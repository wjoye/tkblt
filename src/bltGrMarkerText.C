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

TextMarker::TextMarker(Graph* graphPtr, const char* name, Tcl_HashEntry* hPtr) 
  : Marker(graphPtr, name, hPtr)
{
  obj.classId = CID_MARKER_TEXT;
  obj.className = dupstr("TextMarker");
  ops_ = (TextMarkerOptions*)calloc(1, sizeof(TextMarkerOptions));
  Blt_Ts_InitStyle(((TextMarkerOptions*)ops_)->style);
  optionTable_ = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  anchorPt.x =0;
  anchorPt.y =0;
  width =0;
  height =0;
  fillGC =NULL;
}

TextMarker::~TextMarker()
{
  Graph* graphPtr = obj.graphPtr;

  Blt_Ts_FreeStyle(graphPtr->display, &((TextMarkerOptions*)ops_)->style);
}

int TextMarker::configure()
{
  Graph* graphPtr = obj.graphPtr;
  TextMarkerOptions* ops = (TextMarkerOptions*)ops_;

  ops->style.angle = (float)fmod(ops->style.angle, 360.0);
  if (ops->style.angle < 0.0f)
    ops->style.angle += 360.0f;

  GC newGC = NULL;
  XGCValues gcValues;
  unsigned long gcMask;
  if (ops->fillColor) {
    gcMask = GCForeground;
    gcValues.foreground = ops->fillColor->pixel;
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
  TextMarkerOptions* ops = (TextMarkerOptions*)ops_;

  if (!ops->string)
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
  ops->style.flags |= UPDATE_GC;
  Blt_Ts_DrawText(graphPtr->tkwin, drawable, ops->string, -1,
		  &ops->style, anchorPt.x, anchorPt.y);
}

void TextMarker::map()
{
  Graph* graphPtr = obj.graphPtr;
  TextMarkerOptions* ops = (TextMarkerOptions*)ops_;

  if (!ops->string)
    return;

  if (!ops->worldPts || (ops->worldPts->num < 1))
    return;

  width =0;
  height =0;

  unsigned int w;
  unsigned int h;
  Blt_Ts_GetExtents(&ops->style, ops->string, &w, &h);

  double rw;
  double rh;
  Blt_GetBoundingBox(w, h, ops->style.angle, &rw, &rh, outline);
  width = rw;
  height = rh;
  for (int ii=0; ii<4; ii++) {
    outline[ii].x += rw * 0.5;
    outline[ii].y += rh * 0.5;
  }
  outline[4].x = outline[0].x;
  outline[4].y = outline[0].y;

  Point2d lanchorPtr = mapPoint(ops->worldPts->points, &ops->axes);
  lanchorPtr = Blt_AnchorPoint(lanchorPtr.x, lanchorPtr.y, width, 
			     height, ops->anchor);
  lanchorPtr.x += ops->xOffset;
  lanchorPtr.y += ops->yOffset;

  Region2d extents;
  extents.left = lanchorPtr.x;
  extents.top = lanchorPtr.y;
  extents.right = lanchorPtr.x + width - 1;
  extents.bottom = lanchorPtr.y + height - 1;
  clipped_ = boxesDontOverlap(graphPtr, &extents);

  anchorPt = lanchorPtr;
}

int TextMarker::pointIn(Point2d *samplePtr)
{
  TextMarkerOptions* ops = (TextMarkerOptions*)ops_;

  if (!ops->string)
    return 0;

  if (ops->style.angle != 0.0f) {
    Point2d points[5];

    // Figure out the bounding polygon (isolateral) for the text and see
    // if the point is inside of it.
    for (int ii=0; ii<5; ii++) {
      points[ii].x = outline[ii].x + anchorPt.x;
      points[ii].y = outline[ii].y + anchorPt.y;
    }
    return Blt_PointInPolygon(samplePtr, points, 5);
  } 

  return ((samplePtr->x >= anchorPt.x) && 
	  (samplePtr->x < (anchorPt.x + width)) &&
	  (samplePtr->y >= anchorPt.y) && 
	  (samplePtr->y < (anchorPt.y + height)));
}

int TextMarker::regionIn(Region2d *extsPtr, int enclosed)
{
  TextMarkerOptions* ops = (TextMarkerOptions*)ops_;

  if (ops->style.angle != 0.0f) {
    // Generate the bounding polygon (isolateral) for the bitmap and see
    // if the point is inside of it.
    Point2d points[5];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = outline[ii].x + anchorPt.x;
      points[ii].y = outline[ii].y + anchorPt.y;
    }
    return Blt_RegionInPolygon(extsPtr, points, 4, enclosed);
  } 

  if (enclosed)
    return ((anchorPt.x >= extsPtr->left) &&
	    (anchorPt.y >= extsPtr->top) && 
	    ((anchorPt.x + width) <= extsPtr->right) &&
	    ((anchorPt.y + height) <= extsPtr->bottom));

  return !((anchorPt.x >= extsPtr->right) ||
	   (anchorPt.y >= extsPtr->bottom) ||
	   ((anchorPt.x + width) <= extsPtr->left) ||
	   ((anchorPt.y + height) <= extsPtr->top));
}

void TextMarker::postscript(Blt_Ps ps)
{
  TextMarkerOptions* ops = (TextMarkerOptions*)ops_;

  if (!ops->string)
    return;

  if (fillGC) {
    // Simulate the rotated background of the bitmap by filling a bounding
    // polygon with the background color.
    Point2d points[4];
    for (int ii=0; ii<4; ii++) {
      points[ii].x = outline[ii].x + anchorPt.x;
      points[ii].y = outline[ii].y + anchorPt.y;
    }
    Blt_Ps_XSetBackground(ps, ops->fillColor);
    Blt_Ps_XFillPolygon(ps, points, 4);
  }

  Blt_Ps_DrawText(ps, ops->string, &ops->style,  anchorPt.x, anchorPt.y);
}
