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

#include "bltGrMarkerPolygon.h"

using namespace Blt;

// OptionSpecs

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Polygon all", -1, Tk_Offset(PolygonMarkerOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-cap", "cap", "Cap", 
   "butt", -1, Tk_Offset(PolygonMarkerOptions, capStyle),
   0, &capStyleObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(PolygonMarkerOptions, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes",
   NULL, -1, Tk_Offset(PolygonMarkerOptions, dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(PolygonMarkerOptions, elemName),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill", 
   NULL, -1, Tk_Offset(PolygonMarkerOptions, fill),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-fillbg", "fillbg", "FillBg", 
   NULL, -1, Tk_Offset(PolygonMarkerOptions, fillBg),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-join", "join", "Join", 
   "miter", -1, Tk_Offset(PolygonMarkerOptions, joinStyle),
   0, &joinStyleObjOption, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(PolygonMarkerOptions, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(PolygonMarkerOptions, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(PolygonMarkerOptions, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(PolygonMarkerOptions, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline", 
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(PolygonMarkerOptions, outline), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-outlinebg", "outlinebg", "OutlineBg", 
   NULL, -1, Tk_Offset(PolygonMarkerOptions, outlineBg), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(PolygonMarkerOptions, state),
   0, &stateObjOption, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple",
   NULL, -1, Tk_Offset(PolygonMarkerOptions, stipple),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(PolygonMarkerOptions, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(PolygonMarkerOptions, xOffset), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-xor", "xor", "Xor",
   "no", -1, Tk_Offset(PolygonMarkerOptions, xorr), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(PolygonMarkerOptions, yOffset), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

PolygonMarker::PolygonMarker(Graph* graphPtr, const char* name, 
			     Tcl_HashEntry* hPtr) 
  : Marker(graphPtr, name, hPtr)
{
  obj.classId = CID_MARKER_POLYGON;
  obj.className = dupstr("PolygonMarker");
  ops_ = (PolygonMarkerOptions*)calloc(1, sizeof(PolygonMarkerOptions));
  optionTable_ = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  screenPts =NULL;
  outlineGC =NULL;
  fillGC =NULL;
  fillPts =NULL;
  nFillPts =0;
  outlinePts =NULL;
  nOutlinePts =0;
  xorState =0;
}

PolygonMarker::~PolygonMarker()
{
  Graph* graphPtr = obj.graphPtr;

  if (fillGC)
    Tk_FreeGC(graphPtr->display, fillGC);
  if (outlineGC)
    Blt_FreePrivateGC(graphPtr->display, outlineGC);
  if (fillPts)
    free(fillPts);
  if (outlinePts)
    free(outlinePts);
  if (screenPts)
    free(screenPts);
}

int PolygonMarker::configure()
{
  Graph* graphPtr = obj.graphPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;
  Drawable drawable;

  drawable = Tk_WindowId(graphPtr->tkwin);
  gcMask = (GCLineWidth | GCLineStyle);
  if (ops->outline) {
    gcMask |= GCForeground;
    gcValues.foreground = ops->outline->pixel;
  }
  if (ops->outlineBg) {
    gcMask |= GCBackground;
    gcValues.background = ops->outlineBg->pixel;
  }
  gcMask |= (GCCapStyle | GCJoinStyle);
  gcValues.cap_style = ops->capStyle;
  gcValues.join_style = ops->joinStyle;
  gcValues.line_style = LineSolid;
  gcValues.dash_offset = 0;
  gcValues.line_width = LineWidth(ops->lineWidth);
  if (LineIsDashed(ops->dashes)) {
    gcValues.line_style = (ops->outlineBg == NULL)
      ? LineOnOffDash : LineDoubleDash;
  }
  if (ops->xorr) {
    unsigned long pixel;
    gcValues.function = GXxor;

    gcMask |= GCFunction;
    pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
    if (gcMask & GCBackground) {
      gcValues.background ^= pixel;
    }
    gcValues.foreground ^= pixel;
    if (drawable != None) {
      draw(drawable);
    }
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(ops->dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &ops->dashes);
  }
  if (outlineGC) {
    Blt_FreePrivateGC(graphPtr->display, outlineGC);
  }
  outlineGC = newGC;

  gcMask = 0;
  if (ops->fill) {
    gcMask |= GCForeground;
    gcValues.foreground = ops->fill->pixel;
  }
  if (ops->fillBg) {
    gcMask |= GCBackground;
    gcValues.background = ops->fillBg->pixel;
  }
  if (ops->stipple != None) {
    gcValues.stipple = ops->stipple;
    gcValues.fill_style = (ops->fillBg)
      ? FillOpaqueStippled : FillStippled;
    gcMask |= (GCStipple | GCFillStyle);
  }
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (fillGC) {
    Tk_FreeGC(graphPtr->display, fillGC);
  }
  fillGC = newGC;

  if ((gcMask == 0) && !(graphPtr->flags & RESET_AXES) && (ops->xorr)) {
    if (drawable != None) {
      map();
      draw(drawable);
    }
    return TCL_OK;
  }

  return TCL_OK;
}

void PolygonMarker::draw(Drawable drawable)
{
  Graph* graphPtr = obj.graphPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  // fill region
  if ((nFillPts > 0) && (ops->fill)) {
    XPoint* points = (XPoint*)malloc(nFillPts * sizeof(XPoint));
    if (!points)
      return;

    XPoint* dp = points;
    Point2d *sp, *send;
    for (sp = fillPts, send = sp + nFillPts; sp < send; sp++) {
      dp->x = (short int)sp->x;
      dp->y = (short int)sp->y;
      dp++;
    }

    XFillPolygon(graphPtr->display, drawable, fillGC, points, 
		 nFillPts, Complex, CoordModeOrigin);
    free(points);
  }

  // outline
  if ((nOutlinePts > 0) && (ops->lineWidth > 0) && 
      (ops->outline)) {
    Blt_Draw2DSegments(graphPtr->display, drawable, outlineGC,
		       outlinePts, nOutlinePts);
  }
}

void PolygonMarker::map()
{
  Graph* graphPtr = obj.graphPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  if (outlinePts) {
    free(outlinePts);
    outlinePts = NULL;
    nOutlinePts = 0;
  }

  if (fillPts) {
    free(fillPts);
    fillPts = NULL;
    nFillPts = 0;
  }

  if (screenPts) {
    free(screenPts);
    screenPts = NULL;
  }

  if (!ops->worldPts || ops->worldPts->num < 3)
    return;

  // Allocate and fill a temporary array to hold the screen coordinates of
  // the polygon.

  int nScreenPts = ops->worldPts->num + 1;
  Point2d* lscreenPts = (Point2d*)malloc((nScreenPts + 1) * sizeof(Point2d));
  {
    Point2d *sp, *dp, *send;

    dp = lscreenPts;
    for (sp = ops->worldPts->points, send = sp + ops->worldPts->num; 
	 sp < send; sp++) {
      *dp = mapPoint(sp, &ops->axes);
      dp->x += ops->xOffset;
      dp->y += ops->yOffset;
      dp++;
    }
    *dp = lscreenPts[0];
  }
  Region2d extents;
  Blt_GraphExtents(graphPtr, &extents);
  clipped_ = TRUE;
  if (ops->fill) {
    Point2d* lfillPts = (Point2d*)malloc(sizeof(Point2d) * nScreenPts * 3);
    int n = 
      Blt_PolyRectClip(&extents, lscreenPts, ops->worldPts->num,lfillPts);
    if (n < 3)
      free(lfillPts);
    else {
      nFillPts = n;
      fillPts = lfillPts;
      clipped_ = FALSE;
    }
  }
  if ((ops->outline) && (ops->lineWidth > 0)) { 
    Segment2d *segPtr;
    Point2d *sp, *send;

    /* 
     * Generate line segments representing the polygon outline.  The
     * resulting outline may or may not be closed from viewport clipping.
     */
    Segment2d* loutlinePts = (Segment2d*)malloc(nScreenPts * sizeof(Segment2d));
    if (!loutlinePts)
      return;

    // Note that this assumes that the point array contains an extra point
    // that closes the polygon.
    segPtr = loutlinePts;
    for (sp = lscreenPts, send = sp + (nScreenPts - 1); sp < send; sp++) {
      segPtr->p = sp[0];
      segPtr->q = sp[1];
      if (Blt_LineRectClip(&extents, &segPtr->p, &segPtr->q)) {
	segPtr++;
      }
    }
    nOutlinePts = segPtr - loutlinePts;
    outlinePts = loutlinePts;
    if (nOutlinePts > 0) {
      clipped_ = FALSE;
    }
  }

  screenPts = lscreenPts;
}

int PolygonMarker::pointIn(Point2d *samplePtr)
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  if (ops->worldPts && 
      (ops->worldPts->num >= 3) && 
      (screenPts))
    return Blt_PointInPolygon(samplePtr, screenPts, 
			      ops->worldPts->num + 1);

  return FALSE;
}

int PolygonMarker::regionIn(Region2d *extsPtr, int enclosed)
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;
    
  if (ops->worldPts && 
      (ops->worldPts->num >= 3) && 
      (screenPts))
    return Blt_RegionInPolygon(extsPtr, screenPts, 
			       ops->worldPts->num, enclosed);

  return FALSE;
}

void PolygonMarker::postscript(Blt_Ps ps)
{
  Graph* graphPtr = obj.graphPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  if (ops->fill) {
    Blt_Ps_Polyline(ps, fillPts, nFillPts);
    if (ops->fillBg) {
      Blt_Ps_XSetBackground(ps, ops->fillBg);
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    }
    Blt_Ps_XSetForeground(ps, ops->fill);
    if (ops->stipple != None) {
      Blt_Ps_XSetStipple(ps, graphPtr->display, ops->stipple);
    } else {
      Blt_Ps_Append(ps, "fill\n");
    }
  }

  if ((ops->lineWidth > 0) && (ops->outline)) {

    Blt_Ps_XSetLineAttributes(ps, ops->outline, ops->lineWidth, &ops->dashes,
			      ops->capStyle, ops->joinStyle);

    if ((ops->outlineBg) && (LineIsDashed(ops->dashes))) {
      Blt_Ps_Append(ps, "/DashesProc {\ngsave\n    ");
      Blt_Ps_XSetBackground(ps, ops->outlineBg);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
      Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
    } else {
      Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
    Blt_Ps_Draw2DSegments(ps, outlinePts, nOutlinePts);
  }
}

