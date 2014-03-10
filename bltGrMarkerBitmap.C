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
#include "bltBitmap.h"
#include "bltGrMarkerBitmap.h"

#define GETBITMAP(b) (((b)->destBitmap == None) ? (b)->srcBitmap : (b)->destBitmap)

static Blt_ConfigSpec bitmapConfigSpecs[] = {
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", "center", 
   Tk_Offset(BitmapMarker, anchor), 0},
  {BLT_CONFIG_COLOR, "-background", "background", "Background",
   "white", Tk_Offset(BitmapMarker, fillColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-bg", "background", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", "Bitmap all", 
   Tk_Offset(BitmapMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap", NULL, 
   Tk_Offset(BitmapMarker, srcBitmap), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", NULL, 
   Tk_Offset(BitmapMarker, worldPts), BLT_CONFIG_NULL_OK, 
   &coordsOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element", NULL, 
   Tk_Offset(BitmapMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_SYNONYM, "-fill", "background", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
   "black", Tk_Offset(BitmapMarker, outlineColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", "no", 
   Tk_Offset(BitmapMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", "x", 
   Tk_Offset(BitmapMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", "y", 
   Tk_Offset(BitmapMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, NULL, 
   Tk_Offset(BitmapMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-outline", "foreground", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_DOUBLE, "-rotate", "rotate", "Rotate", "0", 
   Tk_Offset(BitmapMarker, reqAngle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", "normal", 
   Tk_Offset(BitmapMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", "no", 
   Tk_Offset(BitmapMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", "0", 
   Tk_Offset(BitmapMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", "0", 
   Tk_Offset(BitmapMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
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
  bitmapConfigSpecs,
  ConfigureBitmapProc,
  DrawBitmapProc,
  FreeBitmapProc,
  MapBitmapProc,
  PointInBitmapProc,
  RegionInBitmapProc,
  BitmapToPostscriptProc,
};

Marker* Blt_CreateBitmapProc(void)
{
  BitmapMarker *bmPtr;

  bmPtr = calloc(1, sizeof(BitmapMarker));
  bmPtr->classPtr = &bitmapMarkerClass;
  return (Marker *)bmPtr;
}

static int ConfigureBitmapProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;

  if (bmPtr->srcBitmap == None) {
    return TCL_OK;
  }
  bmPtr->angle = fmod(bmPtr->reqAngle, 360.0);
  if (bmPtr->angle < 0.0) {
    bmPtr->angle += 360.0;
  }
  gcMask = 0;

  if (bmPtr->outlineColor != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = bmPtr->outlineColor->pixel;
  }

  if (bmPtr->fillColor != NULL) {
    /* Opaque bitmap: both foreground and background (fill) colors
     * are used. */
    gcValues.background = bmPtr->fillColor->pixel;
    gcMask |= GCBackground;
  } else {
    /* Transparent bitmap: set the clip mask to the current bitmap. */
    gcValues.clip_mask = bmPtr->srcBitmap;
    gcMask |= GCClipMask;
  }

  /* 
   * This is technically a shared GC, but we're going to set/change the clip
   * origin anyways before we draw the bitmap.  This relies on the fact that
   * no other client will be allocated this GC with the GCClipMask set to
   * this particular bitmap.
   */
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (bmPtr->gc != NULL) {
    Tk_FreeGC(graphPtr->display, bmPtr->gc);
  }
  bmPtr->gc = newGC;

  /* Create the background GC containing the fill color. */

  if (bmPtr->fillColor != NULL) {
    gcValues.foreground = bmPtr->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (bmPtr->fillGC != NULL) {
      Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
    }
    bmPtr->fillGC = newGC;
  }

  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }

  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void MapBitmapProc(Marker *markerPtr)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  Region2d extents;
  Graph* graphPtr = markerPtr->obj.graphPtr;
  Point2d anchorPt;
  Point2d corner1, corner2;
  int destWidth, destHeight;
  int srcWidth, srcHeight;
  int i;

  if (bmPtr->srcBitmap == None) {
    return;
  }
  if (bmPtr->destBitmap != None) {
    Tk_FreePixmap(graphPtr->display, bmPtr->destBitmap);
    bmPtr->destBitmap = None;
  }
  /* 
   * Collect the coordinates.  The number of coordinates will determine the
   * calculations to be made.
   * 
   *	   x1 y1	A single pair of X-Y coordinates.  They represent
   *			the anchor position of the bitmap.  
   *
   *	x1 y1 x2 y2	Two pairs of X-Y coordinates.  They represent
   *			two opposite corners of a bounding rectangle. The
   *			bitmap is possibly rotated and scaled to fit into
   *			this box.
   *
   */   
  Tk_SizeOfBitmap(graphPtr->display, bmPtr->srcBitmap, &srcWidth, 
		  &srcHeight);
  corner1 = Blt_MapPoint(markerPtr->worldPts, &markerPtr->axes);
  if (markerPtr->nWorldPts > 1) {
    double hold;

    corner2 = Blt_MapPoint(markerPtr->worldPts + 1, &markerPtr->axes);
    /* Flip the corners if necessary */
    if (corner1.x > corner2.x) {
      hold = corner1.x, corner1.x = corner2.x, corner2.x = hold;
    }
    if (corner1.y > corner2.y) {
      hold = corner1.y, corner1.y = corner2.y, corner2.y = hold;
    }
  } else {
    corner2.x = corner1.x + srcWidth - 1;
    corner2.y = corner1.y + srcHeight - 1;
  }
  destWidth = (int)(corner2.x - corner1.x) + 1;
  destHeight = (int)(corner2.y - corner1.y) + 1;

  if (markerPtr->nWorldPts == 1) {
    anchorPt = Blt_AnchorPoint(corner1.x, corner1.y, (double)destWidth, 
			       (double)destHeight, bmPtr->anchor);
  } else {
    anchorPt = corner1;
  }
  anchorPt.x += markerPtr->xOffset;
  anchorPt.y += markerPtr->yOffset;

  /* Check if the bitmap sits at least partially in the plot area. */
  extents.left   = anchorPt.x;
  extents.top    = anchorPt.y;
  extents.right  = anchorPt.x + destWidth - 1;
  extents.bottom = anchorPt.y + destHeight - 1;
  markerPtr->clipped = Blt_BoxesDontOverlap(graphPtr, &extents);
  if (markerPtr->clipped) {
    return;				/* Bitmap is offscreen. Don't generate
					 * rotated or scaled bitmaps. */
  }

  /*  
   * Scale the bitmap if necessary. It's a little tricky because we only
   * want to scale what's visible on the screen, not the entire bitmap.
   */
  if ((bmPtr->angle != 0.0f) || (destWidth != srcWidth) || 
      (destHeight != srcHeight)) {
    int regionX, regionY, regionWidth, regionHeight; 
    double left, right, top, bottom;

    /* Ignore parts of the bitmap outside of the plot area. */
    left   = MAX(graphPtr->left, extents.left);
    right  = MIN(graphPtr->right, extents.right);
    top    = MAX(graphPtr->top, extents.top);
    bottom = MIN(graphPtr->bottom, extents.bottom);

    /* Determine the portion of the scaled bitmap to display. */
    regionX = regionY = 0;
    if (graphPtr->left > extents.left) {
      regionX = (int)(graphPtr->left - extents.left);
    }
    if (graphPtr->top > extents.top) {
      regionY = (int)(graphPtr->top - extents.top);
    }	    
    regionWidth = (int)(right - left) + 1;
    regionHeight = (int)(bottom - top) + 1;
	
    anchorPt.x = left;
    anchorPt.y = top;
    bmPtr->destBitmap = Blt_ScaleRotateBitmapArea(graphPtr->tkwin, 
						  bmPtr->srcBitmap, srcWidth, srcHeight, regionX, regionY, 
						  regionWidth, regionHeight, destWidth, destHeight, bmPtr->angle);
    bmPtr->destWidth = regionWidth;
    bmPtr->destHeight = regionHeight;
  } else {
    bmPtr->destWidth = srcWidth;
    bmPtr->destHeight = srcHeight;
    bmPtr->destBitmap = None;
  }
  bmPtr->anchorPt = anchorPt;
  {
    double xScale, yScale;
    double tx, ty;
    double rotWidth, rotHeight;
    Point2d polygon[5];
    int n;

    /* 
     * Compute a polygon to represent the background area of the bitmap.
     * This is needed for backgrounds of arbitrarily rotated bitmaps.  We
     * also use it to print a background in PostScript.
     */
    Blt_GetBoundingBox(srcWidth, srcHeight, bmPtr->angle, &rotWidth, 
		       &rotHeight, polygon);
    xScale = (double)destWidth / rotWidth;
    yScale = (double)destHeight / rotHeight;
	
    /* 
     * Adjust each point of the polygon. Both scale it to the new size and
     * translate it to the actual screen position of the bitmap.
     */
    tx = extents.left + destWidth * 0.5;
    ty = extents.top + destHeight * 0.5;
    for (i = 0; i < 4; i++) {
      polygon[i].x = (polygon[i].x * xScale) + tx;
      polygon[i].y = (polygon[i].y * yScale) + ty;
    }
    Blt_GraphExtents(graphPtr, &extents);
    n = Blt_PolyRectClip(&extents, polygon, 4, bmPtr->outline); 
    if (n < 3) { 
      memcpy(&bmPtr->outline, polygon, sizeof(Point2d) * 4);
      bmPtr->nOutlinePts = 4;
    } else {
      bmPtr->nOutlinePts = n;
    }
  }
}

static int PointInBitmapProc(Marker *markerPtr, Point2d *samplePtr)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;

  if (bmPtr->srcBitmap == None) {
    return 0;
  }
  if (bmPtr->angle != 0.0f) {
    Point2d points[MAX_OUTLINE_POINTS];
    int i;

    /*  
     * Generate the bounding polygon (isolateral) for the bitmap and see
     * if the point is inside of it.
     */
    for (i = 0; i < bmPtr->nOutlinePts; i++) {
      points[i].x = bmPtr->outline[i].x + bmPtr->anchorPt.x;
      points[i].y = bmPtr->outline[i].y + bmPtr->anchorPt.y;
    }
    return Blt_PointInPolygon(samplePtr, points, bmPtr->nOutlinePts);
  }
  return ((samplePtr->x >= bmPtr->anchorPt.x) && 
	  (samplePtr->x < (bmPtr->anchorPt.x + bmPtr->destWidth)) &&
	  (samplePtr->y >= bmPtr->anchorPt.y) && 
	  (samplePtr->y < (bmPtr->anchorPt.y + bmPtr->destHeight)));
}

static int RegionInBitmapProc(Marker *markerPtr, Region2d *extsPtr, 
			      int enclosed)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;

  if (markerPtr->nWorldPts < 1) {
    return FALSE;
  }
  if (bmPtr->angle != 0.0f) {
    Point2d points[MAX_OUTLINE_POINTS];
    int i;
	
    /*  
     * Generate the bounding polygon (isolateral) for the bitmap and see
     * if the point is inside of it.
     */
    for (i = 0; i < bmPtr->nOutlinePts; i++) {
      points[i].x = bmPtr->outline[i].x + bmPtr->anchorPt.x;
      points[i].y = bmPtr->outline[i].y + bmPtr->anchorPt.y;
    }
    return Blt_RegionInPolygon(extsPtr, points, bmPtr->nOutlinePts, 
			       enclosed);
  }
  if (enclosed) {
    return ((bmPtr->anchorPt.x >= extsPtr->left) &&
	    (bmPtr->anchorPt.y >= extsPtr->top) && 
	    ((bmPtr->anchorPt.x + bmPtr->destWidth) <= extsPtr->right) &&
	    ((bmPtr->anchorPt.y + bmPtr->destHeight) <= extsPtr->bottom));
  }
  return !((bmPtr->anchorPt.x >= extsPtr->right) ||
	   (bmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((bmPtr->anchorPt.x + bmPtr->destWidth) <= extsPtr->left) ||
	   ((bmPtr->anchorPt.y + bmPtr->destHeight) <= extsPtr->top));
}

static void DrawBitmapProc(Marker *markerPtr, Drawable drawable)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  double rangle;
  Pixmap bitmap;

  bitmap = GETBITMAP(bmPtr);
  if ((bitmap == None) || (bmPtr->destWidth < 1) || (bmPtr->destHeight < 1)) {
    return;
  }
  rangle = fmod(bmPtr->angle, 90.0);
  if ((bmPtr->fillColor == NULL) || (rangle != 0.0)) {

    /* 
     * If the bitmap is rotated and a filled background is required, then
     * a filled polygon is drawn before the bitmap.
     */
    if (bmPtr->fillColor != NULL) {
      int i;
      XPoint polygon[MAX_OUTLINE_POINTS];

      for (i = 0; i < bmPtr->nOutlinePts; i++) {
	polygon[i].x = (short int)bmPtr->outline[i].x;
	polygon[i].y = (short int)bmPtr->outline[i].y;
      }
      XFillPolygon(graphPtr->display, drawable, bmPtr->fillGC,
		   polygon, bmPtr->nOutlinePts, Convex, CoordModeOrigin);
    }
    XSetClipMask(graphPtr->display, bmPtr->gc, bitmap);
    XSetClipOrigin(graphPtr->display, bmPtr->gc, (int)bmPtr->anchorPt.x, 
		   (int)bmPtr->anchorPt.y);
  } else {
    XSetClipMask(graphPtr->display, bmPtr->gc, None);
    XSetClipOrigin(graphPtr->display, bmPtr->gc, 0, 0);
  }
  XCopyPlane(graphPtr->display, bitmap, drawable, bmPtr->gc, 0, 0,
	     bmPtr->destWidth, bmPtr->destHeight, (int)bmPtr->anchorPt.x, 
	     (int)bmPtr->anchorPt.y, 1);
}

static void BitmapToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  Pixmap bitmap;

  bitmap = GETBITMAP(bmPtr);
  if ((bitmap == None) || (bmPtr->destWidth < 1) || (bmPtr->destHeight < 1)) {
    return;				/* No bitmap to display. */
  }
  if (bmPtr->fillColor != NULL) {
    Blt_Ps_XSetBackground(ps, bmPtr->fillColor);
    Blt_Ps_XFillPolygon(ps, bmPtr->outline, 4);
  }
  Blt_Ps_XSetForeground(ps, bmPtr->outlineColor);

  Blt_Ps_Format(ps,
		"  gsave\n    %g %g translate\n    %d %d scale\n", 
		bmPtr->anchorPt.x, bmPtr->anchorPt.y + bmPtr->destHeight, 
		bmPtr->destWidth, -bmPtr->destHeight);
  Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {",
		bmPtr->destWidth, bmPtr->destHeight, bmPtr->destWidth, 
		-bmPtr->destHeight, bmPtr->destHeight);
  Blt_Ps_XSetBitmapData(ps, graphPtr->display, bitmap,
			bmPtr->destWidth, bmPtr->destHeight);
  Blt_Ps_VarAppend(ps, 
		   "    } imagemask\n",
		   "grestore\n", (char*)NULL);
}

static void FreeBitmapProc(Marker *markerPtr)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (bmPtr->gc != NULL) {
    Tk_FreeGC(graphPtr->display, bmPtr->gc);
  }
  if (bmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
  }
  if (bmPtr->destBitmap != None) {
    Tk_FreePixmap(graphPtr->display, bmPtr->destBitmap);
  }
}

