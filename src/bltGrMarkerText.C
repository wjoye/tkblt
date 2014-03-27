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

#include "bltC.h"
#include "bltMath.h"

extern "C" {
#include "bltGraph.h"
};

#include "bltGrMarkerText.h"

using namespace Blt;

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor", 
   "center", -1, Tk_Offset(TextMarkerOptions, anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-background", "background", "Background",
   NULL, -1, Tk_Offset(TextMarkerOptions, fillColor),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Text all", -1, Tk_Offset(TextMarkerOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(TextMarkerOptions, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(TextMarkerOptions, elemName),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_NORMAL, -1, Tk_Offset(TextMarkerOptions, style.font), 0, NULL, 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(TextMarkerOptions, style.color),
   0, NULL, 0},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
   "left", -1, Tk_Offset(TextMarkerOptions, style.justify), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(TextMarkerOptions, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(TextMarkerOptions, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(TextMarkerOptions, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_DOUBLE, "-rotate", "rotate", "Rotate", 
   "0", -1, Tk_Offset(TextMarkerOptions, style.angle), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(TextMarkerOptions, state), 0, &stateObjOption, 0},
  {TK_OPTION_STRING, "-text", "text", "Text", 
   NULL, -1, Tk_Offset(TextMarkerOptions, string), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(TextMarkerOptions, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(TextMarkerOptions, xOffset), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(TextMarkerOptions, yOffset), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

static MarkerMapProc MapTextProc;
static MarkerPointProc PointInTextProc;
static MarkerPostscriptProc TextToPostscriptProc;
static MarkerRegionProc RegionInTextProc;

static MarkerClass textMarkerClass = {
  optionSpecs,
  MapTextProc,
  PointInTextProc,
  RegionInTextProc,
  TextToPostscriptProc,
};

TextMarker::TextMarker(Graph* graphPtr, const char* name) 
  : Marker(graphPtr, name)
{
  obj.classId = CID_MARKER_TEXT;
  obj.className = dupstr("TextMarker");
  classPtr = &textMarkerClass;
  ops = (TextMarkerOptions*)calloc(1, sizeof(TextMarkerOptions));
  Blt_Ts_InitStyle(((TextMarkerOptions*)ops)->style);
  optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  anchorPt.x =0;
  anchorPt.y =0;
  width =0;
  height =0;
  fillGC =NULL;
}

TextMarker::~TextMarker()
{
  Graph* graphPtr = obj.graphPtr;

  Blt_Ts_FreeStyle(graphPtr->display, &((TextMarkerOptions*)ops)->style);
}

int TextMarker::configure()
{
  Graph* graphPtr = obj.graphPtr;
  TextMarkerOptions* opp = (TextMarkerOptions*)ops;

  opp->style.angle = (float)fmod(opp->style.angle, 360.0);
  if (opp->style.angle < 0.0f)
    opp->style.angle += 360.0f;

  GC newGC = NULL;
  XGCValues gcValues;
  unsigned long gcMask;
  if (opp->fillColor) {
    gcMask = GCForeground;
    gcValues.foreground = opp->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (fillGC)
    Tk_FreeGC(graphPtr->display, fillGC);
  fillGC = newGC;

  return TCL_OK;
}

void TextMarker::draw(Drawable drawable) 
{
  Graph* graphPtr = obj.graphPtr;
  TextMarkerOptions* opp = (TextMarkerOptions*)ops;

  if (!opp->string)
    return;

  if (fillGC) {
    // Simulate the rotated background of the bitmap by filling a bounding
    // polygon with the background color.
    XPoint points[4];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = (short int)(outline[ii].x + anchorPt.x);
      points[ii].y = (short int)(outline[ii].y + anchorPt.y);
    }
    XFillPolygon(graphPtr->display, drawable, fillGC, points, 4,
		 Convex, CoordModeOrigin);
  }

  // be sure to update style->gc, things might have changed
  opp->style.flags |= UPDATE_GC;
  Blt_Ts_DrawText(graphPtr->tkwin, drawable, opp->string, -1,
		  &opp->style, anchorPt.x, anchorPt.y);
}

static void MapTextProc(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker* tmPtr = (TextMarker*)markerPtr;
  TextMarkerOptions* ops = (TextMarkerOptions*)tmPtr->ops;

  if (!ops->string)
    return;

  if (!ops->worldPts || (ops->worldPts->num < 1))
    return;

  tmPtr->width =0;
  tmPtr->height =0;

  unsigned int w, h;
  Blt_Ts_GetExtents(&ops->style, ops->string, &w, &h);

  double rw, rh;
  Blt_GetBoundingBox(w, h, ops->style.angle, &rw, &rh, tmPtr->outline);
  tmPtr->width = ROUND(rw);
  tmPtr->height = ROUND(rh);
  for (int ii=0; ii<4; ii++) {
    tmPtr->outline[ii].x += ROUND(rw * 0.5);
    tmPtr->outline[ii].y += ROUND(rh * 0.5);
  }
  tmPtr->outline[4].x = tmPtr->outline[0].x;
  tmPtr->outline[4].y = tmPtr->outline[0].y;

  Point2d anchorPt = 
    Blt_MapPoint(ops->worldPts->points, &ops->axes);
  anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, tmPtr->width, 
			     tmPtr->height, ops->anchor);
  anchorPt.x += ops->xOffset;
  anchorPt.y += ops->yOffset;

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
  TextMarkerOptions* ops = (TextMarkerOptions*)tmPtr->ops;

  if (!ops->string)
    return 0;

  if (ops->style.angle != 0.0f) {
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
  TextMarkerOptions* ops = (TextMarkerOptions*)tmPtr->ops;

  if (ops->style.angle != 0.0f) {
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

static void TextToPostscriptProc(Marker* markerPtr, Blt_Ps ps)
{
  TextMarker* tmPtr = (TextMarker*)markerPtr;
  TextMarkerOptions* ops = (TextMarkerOptions*)tmPtr->ops;

  if (!ops->string)
    return;

  if (tmPtr->fillGC) {
    // Simulate the rotated background of the bitmap by filling a bounding
    // polygon with the background color.
    Point2d points[4];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = tmPtr->outline[ii].x + tmPtr->anchorPt.x;
      points[ii].y = tmPtr->outline[ii].y + tmPtr->anchorPt.y;
    }
    Blt_Ps_XSetBackground(ps, ops->fillColor);
    Blt_Ps_XFillPolygon(ps, points, 4);
  }

  Blt_Ps_DrawText(ps, ops->string, &ops->style,
		  tmPtr->anchorPt.x, tmPtr->anchorPt.y);
}
