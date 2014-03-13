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
#include "bltGrMarkerPolygon.h"

extern Point2d Blt_MapPoint(Point2d *pointPtr, Axis2d *axesPtr);
extern int Blt_BoxesDontOverlap(Graph* graphPtr, Region2d *extsPtr);
extern void Blt_FreeMarker(char*);

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Polygon all", -1, Tk_Offset(PolygonMarker, obj.tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-cap", "cap", "Cap", 
   "butt", -1, Tk_Offset(PolygonMarker, capStyle), 0, &capStyleObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(PolygonMarker, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes",
   NULL, -1, Tk_Offset(PolygonMarker, dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(PolygonMarker, elemName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill", 
   NULL, -1, Tk_Offset(PolygonMarker, fill), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-fillbg", "fillbg", "FillBg", 
   NULL, -1, Tk_Offset(PolygonMarker, fillBg), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-join", "join", "Join", 
   "miter", -1, Tk_Offset(PolygonMarker, joinStyle), 0, &joinStyleObjOption, 0},
  {TK_OPTION_PIXELS, "-polygonwidth", "polygonWidth", "PolygonWidth",
   "1", -1, Tk_Offset(PolygonMarker, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(PolygonMarker, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(PolygonMarker, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(PolygonMarker, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline", 
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(PolygonMarker, outline), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-outlinebg", "outlinebg", "OutlineBg", 
   NULL, -1, Tk_Offset(PolygonMarker, outlineBg), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(PolygonMarker, state), 0, &stateObjOption, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple",
   NULL, -1, Tk_Offset(PolygonMarker, stipple), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(PolygonMarker, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(PolygonMarker, xOffset), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-xor", "xor", "Xor",
   "no", -1, Tk_Offset(PolygonMarker, xorr), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(PolygonMarker, yOffset), 0, NULL, 0},
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

Marker* Blt_CreatePolygonProc(Graph* graphPtr)
{
  PolygonMarker* pmPtr = calloc(1, sizeof(PolygonMarker));
  pmPtr->classPtr = &polygonMarkerClass;
  pmPtr->capStyle = CapButt;
  pmPtr->joinStyle = JoinMiter;
  pmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Marker*)pmPtr;
}

static int PointInPolygonProc(Marker *markerPtr, Point2d *samplePtr)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  if ((markerPtr->nWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
    return Blt_PointInPolygon(samplePtr, pmPtr->screenPts, 
			      markerPtr->nWorldPts + 1);
  }
  return FALSE;
}

static int RegionInPolygonProc(Marker *markerPtr, Region2d *extsPtr, 
			       int enclosed)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    
  if ((markerPtr->nWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
    return Blt_RegionInPolygon(extsPtr, pmPtr->screenPts, 
			       markerPtr->nWorldPts, enclosed);
  }
  return FALSE;
}

static void DrawPolygonProc(Marker *markerPtr, Drawable drawable)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  /* Draw polygon fill region */
  if ((pmPtr->nFillPts > 0) && (pmPtr->fill != NULL)) {
    XPoint *dp, *points;
    Point2d *sp, *send;
	
    points = malloc(pmPtr->nFillPts * sizeof(XPoint));
    if (points == NULL) {
      return;
    }
    dp = points;
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
  if ((pmPtr->nOutlinePts > 0) && (pmPtr->lineWidth > 0) && 
      (pmPtr->outline != NULL)) {
    Blt_Draw2DSegments(graphPtr->display, drawable, pmPtr->outlineGC,
		       pmPtr->outlinePts, pmPtr->nOutlinePts);
  }
}

static void PolygonToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  if (pmPtr->fill != NULL) {

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
    if (pmPtr->fillBg != NULL) {
      /* Draw the solid background as the background layer of the opaque
       * stipple  */
      Blt_Ps_XSetBackground(ps, pmPtr->fillBg);
      /* Retain the path. We'll need it for the foreground layer. */
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    }
    Blt_Ps_XSetForeground(ps, pmPtr->fill);
    if (pmPtr->stipple != None) {
      /* Draw the stipple in the foreground color. */
      Blt_Ps_XSetStipple(ps, graphPtr->display, pmPtr->stipple);
    } else {
      Blt_Ps_Append(ps, "fill\n");
    }
  }

  /* Draw the outline in the foreground color.  */
  if ((pmPtr->lineWidth > 0) && (pmPtr->outline != NULL)) {

    /*  Set up the line attributes.  */
    Blt_Ps_XSetLineAttributes(ps, pmPtr->outline,
			      pmPtr->lineWidth, &pmPtr->dashes, pmPtr->capStyle,
			      pmPtr->joinStyle);

    /*  
     * Define on-the-fly a PostScript macro "DashesProc" that will be
     * executed for each call to the Polygon drawing routine.  If the line
     * isn't dashed, simply make this an empty definition.
     */
    if ((pmPtr->outlineBg != NULL) && (LineIsDashed(pmPtr->dashes))) {
      Blt_Ps_Append(ps, "/DashesProc {\ngsave\n    ");
      Blt_Ps_XSetBackground(ps, pmPtr->outlineBg);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
      Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
    } else {
      Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
    Blt_Ps_Draw2DSegments(ps, pmPtr->outlinePts, pmPtr->nOutlinePts);
  }
}

static int ConfigurePolygonProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;
  Drawable drawable;

  drawable = Tk_WindowId(graphPtr->tkwin);
  gcMask = (GCLineWidth | GCLineStyle);
  if (pmPtr->outline != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = pmPtr->outline->pixel;
  }
  if (pmPtr->outlineBg != NULL) {
    gcMask |= GCBackground;
    gcValues.background = pmPtr->outlineBg->pixel;
  }
  gcMask |= (GCCapStyle | GCJoinStyle);
  gcValues.cap_style = pmPtr->capStyle;
  gcValues.join_style = pmPtr->joinStyle;
  gcValues.line_style = LineSolid;
  gcValues.dash_offset = 0;
  gcValues.line_width = LineWidth(pmPtr->lineWidth);
  if (LineIsDashed(pmPtr->dashes)) {
    gcValues.line_style = (pmPtr->outlineBg == NULL)
      ? LineOnOffDash : LineDoubleDash;
  }
  if (pmPtr->xorr) {
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
  if (LineIsDashed(pmPtr->dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &pmPtr->dashes);
  }
  if (pmPtr->outlineGC != NULL) {
    Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
  }
  pmPtr->outlineGC = newGC;

  gcMask = 0;
  if (pmPtr->fill != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = pmPtr->fill->pixel;
  }
  if (pmPtr->fillBg != NULL) {
    gcMask |= GCBackground;
    gcValues.background = pmPtr->fillBg->pixel;
  }
  if (pmPtr->stipple != None) {
    gcValues.stipple = pmPtr->stipple;
    gcValues.fill_style = (pmPtr->fillBg != NULL)
      ? FillOpaqueStippled : FillStippled;
    gcMask |= (GCStipple | GCFillStyle);
  }
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (pmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
  }
  pmPtr->fillGC = newGC;

  if ((gcMask == 0) && !(graphPtr->flags & RESET_AXES) && (pmPtr->xorr)) {
    if (drawable != None) {
      MapPolygonProc(markerPtr);
      DrawPolygonProc(markerPtr, drawable);
    }
    return TCL_OK;
  }
  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void FreePolygonProc(Marker *markerPtr)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (pmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
  }
  if (pmPtr->outlineGC != NULL) {
    Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
  }
  if (pmPtr->fillPts != NULL) {
    free(pmPtr->fillPts);
  }
  if (pmPtr->outlinePts != NULL) {
    free(pmPtr->outlinePts);
  }
  if (pmPtr->screenPts != NULL) {
    free(pmPtr->screenPts);
  }
}

static void MapPolygonProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  Point2d *screenPts;
  Region2d extents;
  int nScreenPts;

  if (pmPtr->outlinePts != NULL) {
    free(pmPtr->outlinePts);
    pmPtr->outlinePts = NULL;
    pmPtr->nOutlinePts = 0;
  }
  if (pmPtr->fillPts != NULL) {
    free(pmPtr->fillPts);
    pmPtr->fillPts = NULL;
    pmPtr->nFillPts = 0;
  }
  if (pmPtr->screenPts != NULL) {
    free(pmPtr->screenPts);
    pmPtr->screenPts = NULL;
  }
  if (markerPtr->nWorldPts < 3) {
    return;				/* Too few points */
  }

  /* 
   * Allocate and fill a temporary array to hold the screen coordinates of
   * the polygon.
   */
  nScreenPts = markerPtr->nWorldPts + 1;
  screenPts = malloc((nScreenPts + 1) * sizeof(Point2d));
  {
    Point2d *sp, *dp, *send;

    dp = screenPts;
    for (sp = markerPtr->worldPts, send = sp + markerPtr->nWorldPts; 
	 sp < send; sp++) {
      *dp = Blt_MapPoint(sp, &markerPtr->axes);
      dp->x += markerPtr->xOffset;
      dp->y += markerPtr->yOffset;
      dp++;
    }
    *dp = screenPts[0];
  }
  Blt_GraphExtents(graphPtr, &extents);
  markerPtr->clipped = TRUE;
  if (pmPtr->fill != NULL) {	/* Polygon fill required. */
    Point2d *fillPts;
    int n;

    fillPts = malloc(sizeof(Point2d) * nScreenPts * 3);
    n = Blt_PolyRectClip(&extents, screenPts, markerPtr->nWorldPts,fillPts);
    if (n < 3) { 
      free(fillPts);
    } else {
      pmPtr->nFillPts = n;
      pmPtr->fillPts = fillPts;
      markerPtr->clipped = FALSE;
    }
  }
  if ((pmPtr->outline != NULL) && (pmPtr->lineWidth > 0)) { 
    Segment2d *outlinePts;
    Segment2d *segPtr;
    Point2d *sp, *send;

    /* 
     * Generate line segments representing the polygon outline.  The
     * resulting outline may or may not be closed from viewport clipping.
     */
    outlinePts = malloc(nScreenPts * sizeof(Segment2d));
    if (outlinePts == NULL) {
      return;			/* Can't allocate point array */
    }
    /* 
     * Note that this assumes that the point array contains an extra point
     * that closes the polygon.
     */
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
      markerPtr->clipped = FALSE;
    }
  }
  pmPtr->screenPts = screenPts;
}

