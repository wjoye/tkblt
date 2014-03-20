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

#include "bltGrMarkerBitmap.h"

// OptionSpecs

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

MarkerCreateProc Blt_CreateBitmapProc;
static MarkerConfigProc ConfigureBitmapProc;
static MarkerDrawProc DrawBitmapProc;
static MarkerFreeProc FreeBitmapProc;
static MarkerMapProc MapBitmapProc;
static MarkerPointProc PointInBitmapProc;
static MarkerPostscriptProc BitmapToPostscriptProc;
static MarkerRegionProc RegionInBitmapProc;

static MarkerClass bitmapMarkerClass = {
  optionSpecs,
  ConfigureBitmapProc,
  DrawBitmapProc,
  FreeBitmapProc,
  MapBitmapProc,
  PointInBitmapProc,
  RegionInBitmapProc,
  BitmapToPostscriptProc,
};

Marker* Blt_CreateBitmapProc(Graph* graphPtr)
{
  BitmapMarker* bmPtr = (BitmapMarker*)calloc(1, sizeof(BitmapMarker));
  bmPtr->classPtr = &bitmapMarkerClass;
  bmPtr->ops = (BitmapMarkerOptions*)calloc(1, sizeof(BitmapMarkerOptions));

  bmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Marker*)bmPtr;
}

static int ConfigureBitmapProc(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;

  if (bmPtr->ops->bitmap == None)
    return TCL_OK;

  XGCValues gcValues;
  unsigned long gcMask = 0;
  if (bmPtr->ops->outlineColor) {
    gcMask |= GCForeground;
    gcValues.foreground = bmPtr->ops->outlineColor->pixel;
  }

  if (bmPtr->ops->fillColor) {
    // Opaque bitmap: both foreground and background (fill) colors are used
    gcValues.background = bmPtr->ops->fillColor->pixel;
    gcMask |= GCBackground;
  }
  else {
    // Transparent bitmap: set the clip mask to the current bitmap
    gcValues.clip_mask = bmPtr->ops->bitmap;
    gcMask |= GCClipMask;
  }

  // This is technically a shared GC, but we're going to set/change the clip
  // origin anyways before we draw the bitmap.  This relies on the fact that
  // no other client will be allocated this GC with the GCClipMask set to
  // this particular bitmap.
  GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (bmPtr->gc)
    Tk_FreeGC(graphPtr->display, bmPtr->gc);
  bmPtr->gc = newGC;

  // Create the background GC containing the fill color
  if (bmPtr->ops->fillColor) {
    gcValues.foreground = bmPtr->ops->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (bmPtr->fillGC)
      Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
    bmPtr->fillGC = newGC;
  }

  return TCL_OK;
}

static void MapBitmapProc(Marker* markerPtr)
{
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (bmPtr->ops->bitmap == None)
    return;
 
  if (!bmPtr->ops->worldPts || (bmPtr->ops->worldPts->num < 1))
    return;

  int width, height;
  Tk_SizeOfBitmap(graphPtr->display, bmPtr->ops->bitmap, &width, &height);

  Point2d anchorPt = 
    Blt_MapPoint(bmPtr->ops->worldPts->points, &markerPtr->ops->axes);
  anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, width, height,
			     bmPtr->ops->anchor);
  anchorPt.x += markerPtr->ops->xOffset;
  anchorPt.y += markerPtr->ops->yOffset;

  Region2d extents;
  extents.left = anchorPt.x;
  extents.top = anchorPt.y;
  extents.right = anchorPt.x + width - 1;
  extents.bottom = anchorPt.y + height - 1;
  markerPtr->clipped = Blt_BoxesDontOverlap(graphPtr, &extents);

  if (markerPtr->clipped)
    return;

  bmPtr->width = width;
  bmPtr->height = height;
  bmPtr->anchorPt = anchorPt;

  // Compute a polygon to represent the background area of the bitmap.
  // This is needed for print a background in PostScript.
  double rotWidth, rotHeight;
  Point2d polygon[5];
  Blt_GetBoundingBox(width, height, 0, &rotWidth, &rotHeight, polygon);
	
  // Adjust each point of the polygon. Both scale it to the new size and
  // translate it to the actual screen position of the bitmap.
  double tx = extents.left + width * 0.5;
  double ty = extents.top + height * 0.5;
  for (int ii=0; ii<4; ii++) {
    polygon[ii].x = (polygon[ii].x) + tx;
    polygon[ii].y = (polygon[ii].y) + ty;
  }
  Blt_GraphExtents(graphPtr, &extents);
  int nn = Blt_PolyRectClip(&extents, polygon, 4, bmPtr->outline); 
  if (nn < 3) { 
    memcpy(&bmPtr->outline, polygon, sizeof(Point2d) * 4);
    bmPtr->nOutlinePts = 4;
  }
  else
    bmPtr->nOutlinePts = nn;
}

static int PointInBitmapProc(Marker* markerPtr, Point2d *samplePtr)
{
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;

  if (bmPtr->ops->bitmap == None)
    return 0;

  return ((samplePtr->x >= bmPtr->anchorPt.x) && 
	  (samplePtr->x < (bmPtr->anchorPt.x + bmPtr->width)) &&
	  (samplePtr->y >= bmPtr->anchorPt.y) && 
	  (samplePtr->y < (bmPtr->anchorPt.y + bmPtr->height)));
}

static int RegionInBitmapProc(Marker* markerPtr, Region2d *extsPtr, 
			      int enclosed)
{
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;

  if (enclosed) {
    return ((bmPtr->anchorPt.x >= extsPtr->left) &&
	    (bmPtr->anchorPt.y >= extsPtr->top) && 
	    ((bmPtr->anchorPt.x + bmPtr->width) <= extsPtr->right) &&
	    ((bmPtr->anchorPt.y + bmPtr->height) <= extsPtr->bottom));
  }

  return !((bmPtr->anchorPt.x >= extsPtr->right) ||
	   (bmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((bmPtr->anchorPt.x + bmPtr->width) <= extsPtr->left) ||
	   ((bmPtr->anchorPt.y + bmPtr->height) <= extsPtr->top));
}

static void DrawBitmapProc(Marker* markerPtr, Drawable drawable)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;
  if ((bmPtr->ops->bitmap == None) || (bmPtr->width < 1) || (bmPtr->height < 1))
    return;

  if (bmPtr->ops->fillColor == NULL) {
    XSetClipMask(graphPtr->display, bmPtr->gc, bmPtr->ops->bitmap);
    XSetClipOrigin(graphPtr->display, bmPtr->gc, bmPtr->anchorPt.x, 
		   bmPtr->anchorPt.y);
  }
  else {
    XSetClipMask(graphPtr->display, bmPtr->gc, None);
    XSetClipOrigin(graphPtr->display, bmPtr->gc, 0, 0);
  }
  XCopyPlane(graphPtr->display, bmPtr->ops->bitmap, drawable, bmPtr->gc, 0, 0,
	     bmPtr->width, bmPtr->height, bmPtr->anchorPt.x, 
	     bmPtr->anchorPt.y, 1);
}

static void BitmapToPostscriptProc(Marker* markerPtr, Blt_Ps ps)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;
  if ((bmPtr->ops->bitmap == None) || (bmPtr->width < 1) || (bmPtr->height < 1))
    return;

  if (bmPtr->ops->fillColor) {
    Blt_Ps_XSetBackground(ps, bmPtr->ops->fillColor);
    Blt_Ps_XFillPolygon(ps, bmPtr->outline, 4);
  }
  Blt_Ps_XSetForeground(ps, bmPtr->ops->outlineColor);

  Blt_Ps_Format(ps,
		"  gsave\n    %g %g translate\n    %d %d scale\n", 
		bmPtr->anchorPt.x, bmPtr->anchorPt.y + bmPtr->height, 
		bmPtr->width, -bmPtr->height);
  Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {",
		bmPtr->width, bmPtr->height, bmPtr->width, 
		-bmPtr->height, bmPtr->height);
  Blt_Ps_XSetBitmapData(ps, graphPtr->display, bmPtr->ops->bitmap,
			bmPtr->width, bmPtr->height);
  Blt_Ps_VarAppend(ps, 
		   "    } imagemask\n",
		   "grestore\n", (char*)NULL);
}

static void FreeBitmapProc(Marker* markerPtr)
{
  BitmapMarker* bmPtr = (BitmapMarker*)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (bmPtr->gc)
    Tk_FreeGC(graphPtr->display, bmPtr->gc);

  if (bmPtr->fillGC)
    Tk_FreeGC(graphPtr->display, bmPtr->fillGC);

  if (bmPtr->ops)
    free(bmPtr->ops);
}

