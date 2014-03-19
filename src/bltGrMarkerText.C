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

extern "C" {
#include "bltGraph.h"
#include "bltMath.h"
};

#include "bltGrMarkerText.h"

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
   TK_OPTION_NULL_OK, &coordsObjOption, MAP_ITEM},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(TextMarker, elemName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_NORMAL, -1, Tk_Offset(TextMarker, style.font), 0, NULL, 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(TextMarker, style.color), 0, NULL, 0},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
   "left", -1, Tk_Offset(TextMarker, style.justify), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(TextMarker, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(TextMarker, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(TextMarker, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
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
  TextMarker* tmPtr = (TextMarker*)calloc(1, sizeof(TextMarker));
  tmPtr->classPtr = &textMarkerClass;
  Blt_Ts_InitStyle(tmPtr->style);
  tmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Marker*)tmPtr;
}

static int ConfigureTextProc(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker* tmPtr = (TextMarker*)markerPtr;

  tmPtr->style.angle = (float)fmod(tmPtr->style.angle, 360.0);
  if (tmPtr->style.angle < 0.0f) {
    tmPtr->style.angle += 360.0f;
  }

  GC newGC = NULL;
  XGCValues gcValues;
  unsigned long gcMask;
  if (tmPtr->fillColor) {
    gcMask = GCForeground;
    gcValues.foreground = tmPtr->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (tmPtr->fillGC)
    Tk_FreeGC(graphPtr->display, tmPtr->fillGC);
  tmPtr->fillGC = newGC;

  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder)
    graphPtr->flags |= CACHE_DIRTY;

  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void MapTextProc(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker* tmPtr = (TextMarker*)markerPtr;

  if (!tmPtr->string)
    return;

  if (!tmPtr->worldPts || (tmPtr->worldPts->num < 1))
    return;

  tmPtr->width =0;
  tmPtr->height =0;

  unsigned int w, h;
  Blt_Ts_GetExtents(&tmPtr->style, tmPtr->string, &w, &h);

  double rw, rh;
  Blt_GetBoundingBox(w, h, tmPtr->style.angle, &rw, &rh, tmPtr->outline);
  tmPtr->width = ROUND(rw);
  tmPtr->height = ROUND(rh);
  for (int ii=0; ii<4; ii++) {
    tmPtr->outline[ii].x += ROUND(rw * 0.5);
    tmPtr->outline[ii].y += ROUND(rh * 0.5);
  }
  tmPtr->outline[4].x = tmPtr->outline[0].x;
  tmPtr->outline[4].y = tmPtr->outline[0].y;

  Point2d anchorPt = Blt_MapPoint(tmPtr->worldPts->points, &markerPtr->axes);
  anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, tmPtr->width, 
			     tmPtr->height, tmPtr->anchor);
  anchorPt.x += markerPtr->xOffset;
  anchorPt.y += markerPtr->yOffset;

  Region2d extents;
  extents.left = anchorPt.x;
  extents.top = anchorPt.y;
  extents.right = anchorPt.x + tmPtr->width - 1;
  extents.bottom = anchorPt.y + tmPtr->height - 1;
  markerPtr->clipped = Blt_BoxesDontOverlap(graphPtr, &extents);
  tmPtr->anchorPt = anchorPt;
}

static int PointInTextProc(Marker* markerPtr, Point2d *samplePtr)
{
  TextMarker* tmPtr = (TextMarker*)markerPtr;

  if (!tmPtr->string)
    return 0;

  if (tmPtr->style.angle != 0.0f) {
    Point2d points[5];

    // Figure out the bounding polygon (isolateral) for the text and see
    // if the point is inside of it.
    for (int ii=0; ii<5; ii++) {
      points[ii].x = tmPtr->outline[ii].x + tmPtr->anchorPt.x;
      points[ii].y = tmPtr->outline[ii].y + tmPtr->anchorPt.y;
    }
    return Blt_PointInPolygon(samplePtr, points, 5);
  } 

  return ((samplePtr->x >= tmPtr->anchorPt.x) && 
	  (samplePtr->x < (tmPtr->anchorPt.x + tmPtr->width)) &&
	  (samplePtr->y >= tmPtr->anchorPt.y) && 
	  (samplePtr->y < (tmPtr->anchorPt.y + tmPtr->height)));
}

static int RegionInTextProc(Marker* markerPtr, Region2d *extsPtr, int enclosed)
{
  TextMarker* tmPtr = (TextMarker*)markerPtr;

  if (tmPtr->style.angle != 0.0f) {
    // Generate the bounding polygon (isolateral) for the bitmap and see
    // if the point is inside of it.
    Point2d points[5];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = tmPtr->outline[ii].x + tmPtr->anchorPt.x;
      points[ii].y = tmPtr->outline[ii].y + tmPtr->anchorPt.y;
    }
    return Blt_RegionInPolygon(extsPtr, points, 4, enclosed);
  } 

  if (enclosed)
    return ((tmPtr->anchorPt.x >= extsPtr->left) &&
	    (tmPtr->anchorPt.y >= extsPtr->top) && 
	    ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->right) &&
	    ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->bottom));

  return !((tmPtr->anchorPt.x >= extsPtr->right) ||
	   (tmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->left) ||
	   ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->top));
}

static void DrawTextProc(Marker* markerPtr, Drawable drawable) 
{
  TextMarker* tmPtr = (TextMarker*)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (!tmPtr->string)
    return;

  if (tmPtr->fillGC) {
    // Simulate the rotated background of the bitmap by filling a bounding
    // polygon with the background color.
    XPoint points[4];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = (short int)(tmPtr->outline[ii].x + tmPtr->anchorPt.x);
      points[ii].y = (short int)(tmPtr->outline[ii].y + tmPtr->anchorPt.y);
    }
    XFillPolygon(graphPtr->display, drawable, tmPtr->fillGC, points, 4,
		 Convex, CoordModeOrigin);
  }

  // be sure to update style->gc, things might have changed
  tmPtr->style.flags |= UPDATE_GC;
  Blt_Ts_DrawText(graphPtr->tkwin, drawable, tmPtr->string, -1,
		  &tmPtr->style, tmPtr->anchorPt.x, tmPtr->anchorPt.y);
}

static void TextToPostscriptProc(Marker* markerPtr, Blt_Ps ps)
{
  TextMarker* tmPtr = (TextMarker*)markerPtr;

  if (!tmPtr->string)
    return;

  if (tmPtr->fillGC) {
    // Simulate the rotated background of the bitmap by filling a bounding
    // polygon with the background color.
    Point2d points[4];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = tmPtr->outline[ii].x + tmPtr->anchorPt.x;
      points[ii].y = tmPtr->outline[ii].y + tmPtr->anchorPt.y;
    }
    Blt_Ps_XSetBackground(ps, tmPtr->fillColor);
    Blt_Ps_XFillPolygon(ps, points, 4);
  }

  Blt_Ps_DrawText(ps, tmPtr->string, &tmPtr->style, tmPtr->anchorPt.x, 
		  tmPtr->anchorPt.y);
}

static void FreeTextProc(Marker* markerPtr)
{
  TextMarker* tmPtr = (TextMarker*)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  Blt_Ts_FreeStyle(graphPtr->display, &tmPtr->style);
}

