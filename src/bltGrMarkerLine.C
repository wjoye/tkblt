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
#include "bltGrMarkerLine.h"

static Tk_OptionSpec lineOptionSpecs[] = {
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Line all", -1, Tk_Offset(LineMarker, obj.tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-cap", "cap", "Cap", 
   "butt", -1, Tk_Offset(LineMarker, capStyle), 0, &capStyleObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(LineMarker, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes",
   NULL, -1, Tk_Offset(LineMarker, dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_PIXELS, "-dashoffset", "dashOffset", "DashOffset",
   "0", -1, Tk_Offset(LineMarker, dashes.offset), 0, NULL, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(LineMarker, elemName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill",
   NULL, -1, Tk_Offset(LineMarker, fillColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-join", "join", "Join", 
   "miter", -1, Tk_Offset(LineMarker, joinStyle), 0, &capStyleObjOption, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(LineMarker, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(LineMarker, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(LineMarker, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(LineMarker, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_STRING, "-name", "name", "Name",
   NULL, -1, Tk_Offset(LineMarker, obj.name), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline",
   "black", -1, Tk_Offset(LineMarker, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-state", "state", "State", 
   "normal", -1, Tk_Offset(LineMarker, state), 0, &stateObjOption, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(LineMarker, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(LineMarker, xOffset), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-xor", "xor", "Xor",
   "no", -1, Tk_Offset(LineMarker, xor), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(LineMarker, yOffset), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

static Blt_ConfigSpec lineConfigSpecs[] = {
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", "Line all", 
   Tk_Offset(LineMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", "butt", 
   Tk_Offset(LineMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", NULL, 
   Tk_Offset(LineMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
  {BLT_CONFIG_CUSTOM, "-dashes", "dashes", "Dashes", NULL, 
   Tk_Offset(LineMarker, dashes), BLT_CONFIG_NULL_OK, &dashesOption},
  {BLT_CONFIG_PIXELS, "-dashoffset", "dashOffset", "DashOffset",
   "0", Tk_Offset(LineMarker, dashes.offset),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-element", "element", "Element", NULL, 
   Tk_Offset(LineMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-fill", "fill", "Fill", (char*)NULL, 
   Tk_Offset(LineMarker, fillColor), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", "miter", 
   Tk_Offset(LineMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", Tk_Offset(LineMarker, lineWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", "no", 
   Tk_Offset(LineMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", "x", 
   Tk_Offset(LineMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", "y", 
   Tk_Offset(LineMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, NULL, 
   Tk_Offset(LineMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-outline", "outline", "Outline",
   "black", Tk_Offset(LineMarker, outlineColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", "normal", 
   Tk_Offset(LineMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", "no", 
   Tk_Offset(LineMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", "0", 
   Tk_Offset(LineMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", "no", 
   Tk_Offset(LineMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", "0", 
   Tk_Offset(LineMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

MarkerCreateProc Blt_CreateLineProc;
static MarkerConfigProc ConfigureLineProc;
static MarkerDrawProc DrawLineProc;
static MarkerFreeProc FreeLineProc;
static MarkerMapProc MapLineProc;
static MarkerPointProc PointInLineProc;
static MarkerPostscriptProc LineToPostscriptProc;
static MarkerRegionProc RegionInLineProc;

static MarkerClass lineMarkerClass = {
  lineConfigSpecs,
  ConfigureLineProc,
  DrawLineProc,
  FreeLineProc,
  MapLineProc,
  PointInLineProc,
  RegionInLineProc,
  LineToPostscriptProc,
};

Marker* Blt_CreateLineProc(void)
{
  LineMarker *lmPtr;

  lmPtr = calloc(1, sizeof(LineMarker));
  lmPtr->classPtr = &lineMarkerClass;
  lmPtr->xor = FALSE;
  lmPtr->capStyle = CapButt;
  lmPtr->joinStyle = JoinMiter;
  return (Marker *)lmPtr;
}

static int PointInLineProc(Marker *markerPtr, Point2d *samplePtr)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;

  return Blt_PointInSegments(samplePtr, lmPtr->segments, lmPtr->nSegments, 
			     (double)markerPtr->obj.graphPtr->search.halo);
}

static int RegionInLineProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  if (markerPtr->nWorldPts < 2) {
    return FALSE;
  }
  if (enclosed) {
    Point2d *pp, *pend;

    for (pp = markerPtr->worldPts, pend = pp + markerPtr->nWorldPts; 
	 pp < pend; pp++) {
      Point2d p;

      p = Blt_MapPoint(pp, &markerPtr->axes);
      if ((p.x < extsPtr->left) && (p.x > extsPtr->right) &&
	  (p.y < extsPtr->top) && (p.y > extsPtr->bottom)) {
	return FALSE;
      }
    }
    return TRUE;			/* All points inside bounding box. */
  } else {
    int count;
    Point2d *pp, *pend;

    count = 0;
    for (pp = markerPtr->worldPts, pend = pp + (markerPtr->nWorldPts - 1); 
	 pp < pend; pp++) {
      Point2d p, q;

      p = Blt_MapPoint(pp, &markerPtr->axes);
      q = Blt_MapPoint(pp + 1, &markerPtr->axes);
      if (Blt_LineRectClip(extsPtr, &p, &q)) {
	count++;
      }
    }
    return (count > 0);		/* At least 1 segment passes through
				 * region. */
  }
}

static void DrawLineProc(Marker *markerPtr, Drawable drawable)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;

  if (lmPtr->nSegments > 0) {
    Graph* graphPtr = markerPtr->obj.graphPtr;

    Blt_Draw2DSegments(graphPtr->display, drawable, lmPtr->gc, 
		       lmPtr->segments, lmPtr->nSegments);
    if (lmPtr->xor) {		/* Toggle the drawing state */
      lmPtr->xorState = (lmPtr->xorState == 0);
    }
  }
}

static int ConfigureLineProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;
  Drawable drawable;

  drawable = Tk_WindowId(graphPtr->tkwin);
  gcMask = (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle);
  if (lmPtr->outlineColor != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = lmPtr->outlineColor->pixel;
  }
  if (lmPtr->fillColor != NULL) {
    gcMask |= GCBackground;
    gcValues.background = lmPtr->fillColor->pixel;
  }
  gcValues.cap_style = lmPtr->capStyle;
  gcValues.join_style = lmPtr->joinStyle;
  gcValues.line_width = LineWidth(lmPtr->lineWidth);
  gcValues.line_style = LineSolid;
  if (LineIsDashed(lmPtr->dashes)) {
    gcValues.line_style = 
      (gcMask & GCBackground) ? LineDoubleDash : LineOnOffDash;
  }
  if (lmPtr->xor) {
    unsigned long pixel;
    gcValues.function = GXxor;

    gcMask |= GCFunction;
    pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
    if (gcMask & GCBackground) {
      gcValues.background ^= pixel;
    }
    gcValues.foreground ^= pixel;
    if (drawable != None) {
      DrawLineProc(markerPtr, drawable);
    }
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lmPtr->gc != NULL) {
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);
  }
  if (LineIsDashed(lmPtr->dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &lmPtr->dashes);
  }
  lmPtr->gc = newGC;
  if (lmPtr->xor) {
    if (drawable != None) {
      MapLineProc(markerPtr);
      DrawLineProc(markerPtr, drawable);
    }
    return TCL_OK;
  }
  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void LineToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;

  if (lmPtr->nSegments > 0) {
    Blt_Ps_XSetLineAttributes(ps, lmPtr->outlineColor, 
			      lmPtr->lineWidth, &lmPtr->dashes, lmPtr->capStyle,
			      lmPtr->joinStyle);
    if ((LineIsDashed(lmPtr->dashes)) && (lmPtr->fillColor != NULL)) {
      Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
      Blt_Ps_XSetBackground(ps, lmPtr->fillColor);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
      Blt_Ps_VarAppend(ps,
		       "stroke\n",
		       "  grestore\n",
		       "} def\n", (char*)NULL);
    } else {
      Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
    Blt_Ps_Draw2DSegments(ps, lmPtr->segments, lmPtr->nSegments);
  }
}

static void FreeLineProc(Marker *markerPtr)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (lmPtr->gc != NULL) {
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);
  }
  if (lmPtr->segments != NULL) {
    free(lmPtr->segments);
  }
}

static void MapLineProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker *)markerPtr;
  Point2d *srcPtr, *pend;
  Segment2d *segments, *segPtr;
  Point2d p, q;
  Region2d extents;

  lmPtr->nSegments = 0;
  if (lmPtr->segments != NULL) {
    free(lmPtr->segments);
  }
  if (markerPtr->nWorldPts < 2) {
    return;				/* Too few points */
  }
  Blt_GraphExtents(graphPtr, &extents);

  /* 
   * Allow twice the number of world coordinates. The line will represented
   * as series of line segments, not one continous polyline.  This is
   * because clipping against the plot area may chop the line into several
   * disconnected segments.
   */
  segments = malloc(markerPtr->nWorldPts * sizeof(Segment2d));
  srcPtr = markerPtr->worldPts;
  p = Blt_MapPoint(srcPtr, &markerPtr->axes);
  p.x += markerPtr->xOffset;
  p.y += markerPtr->yOffset;

  segPtr = segments;
  for (srcPtr++, pend = markerPtr->worldPts + markerPtr->nWorldPts; 
       srcPtr < pend; srcPtr++) {
    Point2d next;

    next = Blt_MapPoint(srcPtr, &markerPtr->axes);
    next.x += markerPtr->xOffset;
    next.y += markerPtr->yOffset;
    q = next;
    if (Blt_LineRectClip(&extents, &p, &q)) {
      segPtr->p = p;
      segPtr->q = q;
      segPtr++;
    }
    p = next;
  }
  lmPtr->nSegments = segPtr - segments;
  lmPtr->segments = segments;
  markerPtr->clipped = (lmPtr->nSegments == 0);
}

