/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bltGraph.h"
#include "bltGrMarkerText.h"
#include "bltMath.h"

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor", 
   "center", -1, Tk_Offset(TextMarker, anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-background", "background", "Background",
   NULL, -1, Tk_Offset(TextMarker, fillColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Text all", -1, Tk_Offset(TextMarker, obj.tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(TextMarker, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(TextMarker, elemName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_NORMAL, -1, Tk_Offset(TextMarker, style.font), 0, NULL, 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(TextMarker, style.color), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
   "left", -1, Tk_Offset(TextMarker, style.justify), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(TextMarker, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(TextMarker, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(TextMarker, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_PIXELS, "-padx", "padX", "PadX", 
   "4", -1, Tk_Offset(TextMarker, style.xPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-pady", "padY", "PadY", 
   "4", -1, Tk_Offset(TextMarker, style.yPad), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-rotate", "rotate", "Rotate", 
   "0", -1, Tk_Offset(TextMarker, style.angle), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(TextMarker, state), 0, &stateObjOption, 0},
  {TK_OPTION_STRING, "-text", "text", "Text", 
   NULL, -1, Tk_Offset(TextMarker, string), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(TextMarker, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(TextMarker, xOffset), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(TextMarker, yOffset), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

MarkerCreateProc Blt_CreateTextProc;
static MarkerConfigProc ConfigureTextProc;
static MarkerDrawProc DrawTextProc;
static MarkerFreeProc FreeTextProc;
static MarkerMapProc MapTextProc;
static MarkerPointProc PointInTextProc;
static MarkerPostscriptProc TextToPostscriptProc;
static MarkerRegionProc RegionInTextProc;

static MarkerClass textMarkerClass = {
  optionSpecs,
  ConfigureTextProc,
  DrawTextProc,
  FreeTextProc,
  MapTextProc,
  PointInTextProc,
  RegionInTextProc,
  TextToPostscriptProc,
};

Marker* Blt_CreateTextProc(Graph* graphPtr)
{
  TextMarker* tmPtr = calloc(1, sizeof(TextMarker));
  tmPtr->classPtr = &textMarkerClass;
  Blt_Ts_InitStyle(tmPtr->style);
  tmPtr->style.anchor = TK_ANCHOR_NW;
  tmPtr->style.xPad = 4;
  tmPtr->style.yPad = 4;
  tmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Marker *)tmPtr;
}

static int ConfigureTextProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;

  tmPtr->style.angle = (float)fmod(tmPtr->style.angle, 360.0);
  if (tmPtr->style.angle < 0.0f) {
    tmPtr->style.angle += 360.0f;
  }
  newGC = NULL;
  if (tmPtr->fillColor != NULL) {
    gcMask = GCForeground;
    gcValues.foreground = tmPtr->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (tmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, tmPtr->fillGC);
  }
  tmPtr->fillGC = newGC;

  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void MapTextProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Region2d extents;
  Point2d anchorPt;
  int i;
  unsigned int w, h;
  double rw, rh;

  tmPtr->width = tmPtr->height = 0;
  if (tmPtr->string == NULL) {
    return;
  }
  Blt_Ts_GetExtents(&tmPtr->style, tmPtr->string, &w, &h);
  Blt_GetBoundingBox(w, h, tmPtr->style.angle, &rw, &rh, tmPtr->outline);
  tmPtr->width = ROUND(rw);
  tmPtr->height = ROUND(rh);
  for (i = 0; i < 4; i++) {
    tmPtr->outline[i].x += ROUND(rw * 0.5);
    tmPtr->outline[i].y += ROUND(rh * 0.5);
  }
  tmPtr->outline[4].x = tmPtr->outline[0].x;
  tmPtr->outline[4].y = tmPtr->outline[0].y;
  anchorPt = Blt_MapPoint(markerPtr->worldPts, &markerPtr->axes);
  anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, (double)(tmPtr->width), 
			     (double)(tmPtr->height), tmPtr->anchor);
  anchorPt.x += markerPtr->xOffset;
  anchorPt.y += markerPtr->yOffset;
  /*
   * Determine the bounding box of the text and test to see if it is at
   * least partially contained within the plotting area.
   */
  extents.left = anchorPt.x;
  extents.top = anchorPt.y;
  extents.right = anchorPt.x + tmPtr->width - 1;
  extents.bottom = anchorPt.y + tmPtr->height - 1;
  markerPtr->clipped = Blt_BoxesDontOverlap(graphPtr, &extents);
  tmPtr->anchorPt = anchorPt;

}

static int PointInTextProc(Marker *markerPtr, Point2d *samplePtr)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;

  if (tmPtr->string == NULL) {
    return 0;
  }
  if (tmPtr->style.angle != 0.0f) {
    Point2d points[5];
    int i;

    /* 
     * Figure out the bounding polygon (isolateral) for the text and see
     * if the point is inside of it.
     */
    for (i = 0; i < 5; i++) {
      points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
      points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
    }
    return Blt_PointInPolygon(samplePtr, points, 5);
  } 
  return ((samplePtr->x >= tmPtr->anchorPt.x) && 
	  (samplePtr->x < (tmPtr->anchorPt.x + tmPtr->width)) &&
	  (samplePtr->y >= tmPtr->anchorPt.y) && 
	  (samplePtr->y < (tmPtr->anchorPt.y + tmPtr->height)));
}

static int RegionInTextProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;

  if (markerPtr->nWorldPts < 1) {
    return FALSE;
  }
  if (tmPtr->style.angle != 0.0f) {
    Point2d points[5];
    int i;
	
    /*  
     * Generate the bounding polygon (isolateral) for the bitmap and see
     * if the point is inside of it.
     */
    for (i = 0; i < 4; i++) {
      points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
      points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
    }
    return Blt_RegionInPolygon(extsPtr, points, 4, enclosed);
  } 
  if (enclosed) {
    return ((tmPtr->anchorPt.x >= extsPtr->left) &&
	    (tmPtr->anchorPt.y >= extsPtr->top) && 
	    ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->right) &&
	    ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->bottom));
  }
  return !((tmPtr->anchorPt.x >= extsPtr->right) ||
	   (tmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->left) ||
	   ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->top));
}

static void DrawTextProc(Marker *markerPtr, Drawable drawable) 
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (tmPtr->string == NULL) {
    return;
  }
  if (tmPtr->fillGC != NULL) {
    XPoint points[4];
    int i;

    /*
     * Simulate the rotated background of the bitmap by filling a bounding
     * polygon with the background color.
     */
    for (i = 0; i < 4; i++) {
      points[i].x = (short int)(tmPtr->outline[i].x + tmPtr->anchorPt.x);
      points[i].y = (short int)(tmPtr->outline[i].y + tmPtr->anchorPt.y);
    }
    XFillPolygon(graphPtr->display, drawable, tmPtr->fillGC, points, 4,
		 Convex, CoordModeOrigin);
  }
  if (tmPtr->style.color != NULL) {
    Blt_Ts_DrawText(graphPtr->tkwin, drawable, tmPtr->string, -1,
		    &tmPtr->style, (int)tmPtr->anchorPt.x, (int)tmPtr->anchorPt.y);
  }
}

static void TextToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;

  if (tmPtr->string == NULL) {
    return;
  }
  if (tmPtr->fillGC != NULL) {
    Point2d points[4];
    int i;

    /*
     * Simulate the rotated background of the bitmap by filling a bounding
     * polygon with the background color.
     */
    for (i = 0; i < 4; i++) {
      points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
      points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
    }
    Blt_Ps_XSetBackground(ps, tmPtr->fillColor);
    Blt_Ps_XFillPolygon(ps, points, 4);
  }
  Blt_Ps_DrawText(ps, tmPtr->string, &tmPtr->style, tmPtr->anchorPt.x, 
		  tmPtr->anchorPt.y);
}

static void FreeTextProc(Marker *markerPtr)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  Blt_Ts_FreeStyle(graphPtr->display, &tmPtr->style);
}

