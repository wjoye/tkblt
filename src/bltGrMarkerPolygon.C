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

extern "C" {
#include "bltGraph.h"
};

#include "bltGrMarkerPolygon.h"

PolygonMarker::PolygonMarker() : BltMarker()
{
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
}

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

MarkerCreateProc Blt_CreatePolygonProc;
static MarkerConfigProc ConfigurePolygonProc;
static MarkerDrawProc DrawPolygonProc;
static MarkerFreeProc FreePolygonProc;
static MarkerMapProc MapPolygonProc;
static MarkerPointProc PointInPolygonProc;
static MarkerPostscriptProc PolygonToPostscriptProc;
static MarkerRegionProc RegionInPolygonProc;

static MarkerClass polygonMarkerClass = {
  optionSpecs,
  ConfigurePolygonProc,
  DrawPolygonProc,
  FreePolygonProc,
  MapPolygonProc,
  PointInPolygonProc,
  RegionInPolygonProc,
  PolygonToPostscriptProc,
};

BltMarker* Blt_CreatePolygonProc(Graph* graphPtr)
{
  PolygonMarker* pmPtr = (PolygonMarker*)calloc(1, sizeof(PolygonMarker));
  pmPtr->classPtr = &polygonMarkerClass;
  pmPtr->ops = (PolygonMarkerOptions*)calloc(1, sizeof(PolygonMarkerOptions));

  pmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (BltMarker*)pmPtr;
}

static int PointInPolygonProc(BltMarker* markerPtr, Point2d *samplePtr)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;

  if (ops->worldPts && 
      (ops->worldPts->num >= 3) && 
      (pmPtr->screenPts))
    return Blt_PointInPolygon(samplePtr, pmPtr->screenPts, 
			      ops->worldPts->num + 1);

  return FALSE;
}

static int RegionInPolygonProc(BltMarker* markerPtr, Region2d *extsPtr, 
			       int enclosed)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;
    
  if (ops->worldPts && 
      (ops->worldPts->num >= 3) && 
      (pmPtr->screenPts))
    return Blt_RegionInPolygon(extsPtr, pmPtr->screenPts, 
			       ops->worldPts->num, enclosed);

  return FALSE;
}

static void DrawPolygonProc(BltMarker* markerPtr, Drawable drawable)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;

  /* Draw polygon fill region */
  if ((pmPtr->nFillPts > 0) && (ops->fill)) {
    XPoint* points = (XPoint*)malloc(pmPtr->nFillPts * sizeof(XPoint));
    if (!points)
      return;

    XPoint* dp = points;
    Point2d *sp, *send;
    for (sp = pmPtr->fillPts, send = sp + pmPtr->nFillPts; sp < send; 
	 sp++) {
      dp->x = (short int)sp->x;
      dp->y = (short int)sp->y;
      dp++;
    }

    XFillPolygon(graphPtr->display, drawable, pmPtr->fillGC, points, 
		 pmPtr->nFillPts, Complex, CoordModeOrigin);
    free(points);
  }
  /* and then the outline */
  if ((pmPtr->nOutlinePts > 0) && (ops->lineWidth > 0) && 
      (ops->outline)) {
    Blt_Draw2DSegments(graphPtr->display, drawable, pmPtr->outlineGC,
		       pmPtr->outlinePts, pmPtr->nOutlinePts);
  }
}

static void PolygonToPostscriptProc(BltMarker* markerPtr, Blt_Ps ps)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;

  if (ops->fill) {

    /*
     * Options:  fg bg
     *			Draw outline only.
     *	     x          Draw solid or stipple.
     *	     x  x       Draw solid or stipple.
     */

    /* Create a path to use for both the polygon and its outline. */
    Blt_Ps_Polyline(ps, pmPtr->fillPts, pmPtr->nFillPts);

    /* If the background fill color was specified, draw the polygon in a
     * solid fashion with that color.  */
    if (ops->fillBg) {
      /* Draw the solid background as the background layer of the opaque
       * stipple  */
      Blt_Ps_XSetBackground(ps, ops->fillBg);
      /* Retain the path. We'll need it for the foreground layer. */
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    }
    Blt_Ps_XSetForeground(ps, ops->fill);
    if (ops->stipple != None) {
      /* Draw the stipple in the foreground color. */
      Blt_Ps_XSetStipple(ps, graphPtr->display, ops->stipple);
    } else {
      Blt_Ps_Append(ps, "fill\n");
    }
  }

  /* Draw the outline in the foreground color.  */
  if ((ops->lineWidth > 0) && (ops->outline)) {

    /*  Set up the line attributes.  */
    Blt_Ps_XSetLineAttributes(ps, ops->outline,
			      ops->lineWidth,
			      &ops->dashes,
			      ops->capStyle,
			      ops->joinStyle);

    /*  
     * Define on-the-fly a PostScript macro "DashesProc" that will be
     * executed for each call to the Polygon drawing routine.  If the line
     * isn't dashed, simply make this an empty definition.
     */
    if ((ops->outlineBg) && (LineIsDashed(ops->dashes))) {
      Blt_Ps_Append(ps, "/DashesProc {\ngsave\n    ");
      Blt_Ps_XSetBackground(ps, ops->outlineBg);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
      Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
    } else {
      Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
    Blt_Ps_Draw2DSegments(ps, pmPtr->outlinePts, pmPtr->nOutlinePts);
  }
}

static int ConfigurePolygonProc(BltMarker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;

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
      DrawPolygonProc(markerPtr, drawable);
    }
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(ops->dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &ops->dashes);
  }
  if (pmPtr->outlineGC) {
    Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
  }
  pmPtr->outlineGC = newGC;

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
  if (pmPtr->fillGC) {
    Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
  }
  pmPtr->fillGC = newGC;

  if ((gcMask == 0) && !(graphPtr->flags & RESET_AXES) && (ops->xorr)) {
    if (drawable != None) {
      MapPolygonProc(markerPtr);
      DrawPolygonProc(markerPtr, drawable);
    }
    return TCL_OK;
  }

  return TCL_OK;
}

static void FreePolygonProc(BltMarker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;

  if (pmPtr->fillGC)
    Tk_FreeGC(graphPtr->display, pmPtr->fillGC);

  if (pmPtr->outlineGC)
    Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);

  if (pmPtr->fillPts)
    free(pmPtr->fillPts);

  if (pmPtr->outlinePts)
    free(pmPtr->outlinePts);

  if (pmPtr->screenPts)
    free(pmPtr->screenPts);

  if (ops)
    free(ops);
}

static void MapPolygonProc(BltMarker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)pmPtr->ops;

  if (pmPtr->outlinePts) {
    free(pmPtr->outlinePts);
    pmPtr->outlinePts = NULL;
    pmPtr->nOutlinePts = 0;
  }
  if (pmPtr->fillPts) {
    free(pmPtr->fillPts);
    pmPtr->fillPts = NULL;
    pmPtr->nFillPts = 0;
  }
  if (pmPtr->screenPts) {
    free(pmPtr->screenPts);
    pmPtr->screenPts = NULL;
  }
  if (!ops->worldPts || ops->worldPts->num < 3)
    return;

  /* 
   * Allocate and fill a temporary array to hold the screen coordinates of
   * the polygon.
   */
  int nScreenPts = ops->worldPts->num + 1;
  Point2d* screenPts = (Point2d*)malloc((nScreenPts + 1) * sizeof(Point2d));
  {
    Point2d *sp, *dp, *send;

    dp = screenPts;
    for (sp = ops->worldPts->points, send = sp + ops->worldPts->num; 
	 sp < send; sp++) {
      *dp = Blt_MapPoint(sp, &ops->axes);
      dp->x += ops->xOffset;
      dp->y += ops->yOffset;
      dp++;
    }
    *dp = screenPts[0];
  }
  Region2d extents;
  Blt_GraphExtents(graphPtr, &extents);
  pmPtr->clipped = TRUE;
  if (ops->fill) {
    Point2d* fillPts = (Point2d*)malloc(sizeof(Point2d) * nScreenPts * 3);
    int n = 
      Blt_PolyRectClip(&extents, screenPts, ops->worldPts->num,fillPts);
    if (n < 3)
      free(fillPts);
    else {
      pmPtr->nFillPts = n;
      pmPtr->fillPts = fillPts;
      pmPtr->clipped = FALSE;
    }
  }
  if ((ops->outline) && (ops->lineWidth > 0)) { 
    Segment2d *segPtr;
    Point2d *sp, *send;

    /* 
     * Generate line segments representing the polygon outline.  The
     * resulting outline may or may not be closed from viewport clipping.
     */
    Segment2d* outlinePts = (Segment2d*)malloc(nScreenPts * sizeof(Segment2d));
    if (!outlinePts)
      return;

    // Note that this assumes that the point array contains an extra point
    // that closes the polygon.
    segPtr = outlinePts;
    for (sp = screenPts, send = sp + (nScreenPts - 1); sp < send; sp++) {
      segPtr->p = sp[0];
      segPtr->q = sp[1];
      if (Blt_LineRectClip(&extents, &segPtr->p, &segPtr->q)) {
	segPtr++;
      }
    }
    pmPtr->nOutlinePts = segPtr - outlinePts;
    pmPtr->outlinePts = outlinePts;
    if (pmPtr->nOutlinePts > 0) {
      pmPtr->clipped = FALSE;
    }
  }
  pmPtr->screenPts = screenPts;
}

