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

#include "bltGrMarkerBitmap.h"
#include "bltGrMarkerOption.h"
#include "bltGrMisc.h"

using namespace Blt;

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor", 
   "center", -1, Tk_Offset(BitmapMarkerOptions, anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-background", "background", "Background",
   NULL, -1, Tk_Offset(BitmapMarkerOptions, fillColor),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Bitmap all", -1, Tk_Offset(BitmapMarkerOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_BITMAP, "-bitmap", "bitmap", "Bitmap", 
   NULL, -1, Tk_Offset(BitmapMarkerOptions, bitmap),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(BitmapMarkerOptions, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(BitmapMarkerOptions, elemName),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BitmapMarkerOptions, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(BitmapMarkerOptions, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(BitmapMarkerOptions, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(BitmapMarkerOptions, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(BitmapMarkerOptions, state), 0, &stateObjOption, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(BitmapMarkerOptions, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(BitmapMarkerOptions, xOffset), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(BitmapMarkerOptions, yOffset), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

BitmapMarker::BitmapMarker(Graph* graphPtr, const char* name, 
			   Tcl_HashEntry* hPtr)
  : Marker(graphPtr, name, hPtr)
{
  ops_ = (BitmapMarkerOptions*)calloc(1, sizeof(BitmapMarkerOptions));
  optionTable_ = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  anchorPt_.x =0;
  anchorPt_.y =0;
  gc_ =NULL;
  fillGC_ =NULL;
  nOutlinePts_ =0;
  width_ =0;
  height_ =0;
}

BitmapMarker::~BitmapMarker()
{
  if (gc_)
    Tk_FreeGC(graphPtr_->display, gc_);
  if (fillGC_)
    Tk_FreeGC(graphPtr_->display, fillGC_);
}

int BitmapMarker::configure()
{
  BitmapMarkerOptions* ops = (BitmapMarkerOptions*)ops_;

  if (ops->bitmap == None)
    return TCL_OK;

  XGCValues gcValues;
  unsigned long gcMask = 0;
  if (ops->outlineColor) {
    gcMask |= GCForeground;
    gcValues.foreground = ops->outlineColor->pixel;
  }

  if (ops->fillColor) {
    // Opaque bitmap: both foreground and background (fill) colors are used
    gcValues.background = ops->fillColor->pixel;
    gcMask |= GCBackground;
  }
  else {
    // Transparent bitmap: set the clip mask to the current bitmap
    gcValues.clip_mask = ops->bitmap;
    gcMask |= GCClipMask;
  }

  // This is technically a shared GC, but we're going to set/change the clip
  // origin anyways before we draw the bitmap.  This relies on the fact that
  // no other client will be allocated this GC with the GCClipMask set to
  // this particular bitmap.
  GC newGC = Tk_GetGC(graphPtr_->tkwin, gcMask, &gcValues);
  if (gc_)
    Tk_FreeGC(graphPtr_->display, gc_);
  gc_ = newGC;

  // Create the background GC containing the fill color
  if (ops->fillColor) {
    gcValues.foreground = ops->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr_->tkwin, gcMask, &gcValues);
    if (fillGC_)
      Tk_FreeGC(graphPtr_->display, fillGC_);
    fillGC_ = newGC;
  }

  return TCL_OK;
}

void BitmapMarker::draw(Drawable drawable)
{
  BitmapMarkerOptions* ops = (BitmapMarkerOptions*)ops_;

  if ((ops->bitmap == None) || (width_ < 1) || (height_ < 1))
    return;

  if (ops->fillColor == NULL) {
    XSetClipMask(graphPtr_->display, gc_, ops->bitmap);
    XSetClipOrigin(graphPtr_->display, gc_, anchorPt_.x, anchorPt_.y);
  }
  else {
    XSetClipMask(graphPtr_->display, gc_, None);
    XSetClipOrigin(graphPtr_->display, gc_, 0, 0);
  }
  XCopyPlane(graphPtr_->display, ops->bitmap, drawable, gc_, 0, 0,
	     width_, height_, anchorPt_.x, anchorPt_.y, 1);
}

void BitmapMarker::map()
{
  BitmapMarkerOptions* ops = (BitmapMarkerOptions*)ops_;

  if (ops->bitmap == None)
    return;
 
  if (!ops->worldPts || (ops->worldPts->num < 1))
    return;

  int lwidth;
  int lheight;
  Tk_SizeOfBitmap(graphPtr_->display, ops->bitmap, &lwidth, &lheight);

  Point2d lanchorPt = mapPoint(ops->worldPts->points, &ops->axes);
  lanchorPt = 
    Blt_AnchorPoint(lanchorPt.x, lanchorPt.y, lwidth, lheight, ops->anchor);
  lanchorPt.x += ops->xOffset;
  lanchorPt.y += ops->yOffset;

  Region2d extents;
  extents.left = lanchorPt.x;
  extents.top = lanchorPt.y;
  extents.right = lanchorPt.x + lwidth - 1;
  extents.bottom = lanchorPt.y + lheight - 1;
  clipped_ = boxesDontOverlap(graphPtr_, &extents);

  if (clipped_)
    return;

  width_ = lwidth;
  height_ = lheight;
  anchorPt_ = lanchorPt;

  // Compute a polygon to represent the background area of the bitmap.
  // This is needed for print a background in PostScript.
  double rotWidth, rotHeight;
  Point2d polygon[5];
  Blt_GetBoundingBox(width_, height_, 0, &rotWidth, &rotHeight, polygon);
	
  // Adjust each point of the polygon. Both scale it to the new size and
  // translate it to the actual screen position of the bitmap.
  double tx = extents.left + width_ * 0.5;
  double ty = extents.top + height_ * 0.5;
  for (int ii=0; ii<4; ii++) {
    polygon[ii].x = (polygon[ii].x) + tx;
    polygon[ii].y = (polygon[ii].y) + ty;
  }
  Blt_GraphExtents(graphPtr_, &extents);
  int nn = Blt_PolyRectClip(&extents, polygon, 4, outline_); 
  if (nn < 3) { 
    memcpy(&outline_, polygon, sizeof(Point2d) * 4);
    nOutlinePts_ = 4;
  }
  else
    nOutlinePts_ = nn;
}

int BitmapMarker::pointIn(Point2d *samplePtr)
{
  BitmapMarkerOptions* ops = (BitmapMarkerOptions*)ops_;

  if (ops->bitmap == None)
    return 0;

  return ((samplePtr->x >= anchorPt_.x) && 
	  (samplePtr->x < (anchorPt_.x + width_)) &&
	  (samplePtr->y >= anchorPt_.y) && 
	  (samplePtr->y < (anchorPt_.y + height_)));
}

int BitmapMarker::regionIn(Region2d *extsPtr, int enclosed)
{
  if (enclosed) {
    return ((anchorPt_.x >= extsPtr->left) &&
	    (anchorPt_.y >= extsPtr->top) && 
	    ((anchorPt_.x + width_) <= extsPtr->right) &&
	    ((anchorPt_.y + height_) <= extsPtr->bottom));
  }

  return !((anchorPt_.x >= extsPtr->right) ||
	   (anchorPt_.y >= extsPtr->bottom) ||
	   ((anchorPt_.x + width_) <= extsPtr->left) ||
	   ((anchorPt_.y + height_) <= extsPtr->top));
}

void BitmapMarker::postscript(Blt_Ps ps)
{
  BitmapMarkerOptions* ops = (BitmapMarkerOptions*)ops_;

  if ((ops->bitmap == None) || (width_ < 1) || (height_ < 1))
    return;

  if (ops->fillColor) {
    Blt_Ps_XSetBackground(ps, ops->fillColor);
    Blt_Ps_XFillPolygon(ps, outline_, 4);
  }
  Blt_Ps_XSetForeground(ps, ops->outlineColor);

  Blt_Ps_Format(ps, "  gsave\n    %g %g translate\n    %d %d scale\n", 
		anchorPt_.x, anchorPt_.y + height_, width_, -height_);
  Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {",
		width_, height_, width_, -height_, height_);
  Blt_Ps_XSetBitmapData(ps, graphPtr_->display, ops->bitmap, width_, height_);
  Blt_Ps_VarAppend(ps, "    } imagemask\n", "grestore\n", (char*)NULL);
}
