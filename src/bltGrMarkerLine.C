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

#include "bltGrMarkerLine.h"

// Defs

// OptionSpecs

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Line all", -1, Tk_Offset(LineMarker, obj.tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-cap", "cap", "Cap", 
   "butt", -1, Tk_Offset(LineMarker, capStyle), 0, &capStyleObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(LineMarker, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, MAP_ITEM},
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
   "miter", -1, Tk_Offset(LineMarker, joinStyle), 0, &joinStyleObjOption, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(LineMarker, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(LineMarker, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(LineMarker, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(LineMarker, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LineMarker, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(LineMarker, state), 0, &stateObjOption, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(LineMarker, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(LineMarker, xOffset), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-xor", "xor", "Xor",
   "no", -1, Tk_Offset(LineMarker, xorr), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(LineMarker, yOffset), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
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
  optionSpecs,
  ConfigureLineProc,
  DrawLineProc,
  FreeLineProc,
  MapLineProc,
  PointInLineProc,
  RegionInLineProc,
  LineToPostscriptProc,
};

Marker* Blt_CreateLineProc(Graph* graphPtr)
{
  LineMarker* lmPtr = (LineMarker*)calloc(1, sizeof(LineMarker));
  lmPtr->classPtr = &lineMarkerClass;
  lmPtr->xorr = FALSE;
  lmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Marker*)lmPtr;
}

static int PointInLineProc(Marker *markerPtr, Point2d *samplePtr)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  return Blt_PointInSegments(samplePtr, lmPtr->segments, lmPtr->nSegments, 
			     (double)markerPtr->obj.graphPtr->search.halo);
}

static int RegionInLineProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  if (!lmPtr->worldPts || lmPtr->worldPts->num < 2)
    return FALSE;

  if (enclosed) {
    Point2d *pp, *pend;

    for (pp = lmPtr->worldPts->points, pend = pp + lmPtr->worldPts->num; 
	 pp < pend; pp++) {
      Point2d p = Blt_MapPoint(pp, &markerPtr->axes);
      if ((p.x < extsPtr->left) && (p.x > extsPtr->right) &&
	  (p.y < extsPtr->top) && (p.y > extsPtr->bottom)) {
	return FALSE;
      }
    }
    return TRUE;			/* All points inside bounding box. */
  }
  else {
    Point2d *pp, *pend;
    int count = 0;
    for (pp = lmPtr->worldPts->points, pend = pp + (lmPtr->worldPts->num - 1); 
	 pp < pend; pp++) {
      Point2d p = Blt_MapPoint(pp, &markerPtr->axes);
      Point2d q = Blt_MapPoint(pp + 1, &markerPtr->axes);
      if (Blt_LineRectClip(extsPtr, &p, &q))
	count++;
    }
    return (count > 0);		/* At least 1 segment passes through
				 * region. */
  }
}

static void DrawLineProc(Marker *markerPtr, Drawable drawable)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  if (lmPtr->nSegments > 0) {
    Graph* graphPtr = markerPtr->obj.graphPtr;

    Blt_Draw2DSegments(graphPtr->display, drawable, lmPtr->gc, 
		       lmPtr->segments, lmPtr->nSegments);
    if (lmPtr->xorr)
      lmPtr->xorState = (lmPtr->xorState == 0);
  }
}

static int ConfigureLineProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  Drawable drawable = Tk_WindowId(graphPtr->tkwin);
  unsigned long gcMask = (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle);
  XGCValues gcValues;
  if (lmPtr->outlineColor) {
    gcMask |= GCForeground;
    gcValues.foreground = lmPtr->outlineColor->pixel;
  }
  if (lmPtr->fillColor) {
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
  if (lmPtr->xorr) {
    unsigned long pixel;
    gcValues.function = GXxor;

    gcMask |= GCFunction;
    pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
    if (gcMask & GCBackground)
      gcValues.background ^= pixel;

    gcValues.foreground ^= pixel;
    if (drawable != None)
      DrawLineProc(markerPtr, drawable);
  }

  GC newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lmPtr->gc)
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);

  if (LineIsDashed(lmPtr->dashes))
    Blt_SetDashes(graphPtr->display, newGC, &lmPtr->dashes);

  lmPtr->gc = newGC;
  if (lmPtr->xorr) {
    if (drawable != None) {
      MapLineProc(markerPtr);
      DrawLineProc(markerPtr, drawable);
    }
    return TCL_OK;
  }
  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder)
    graphPtr->flags |= CACHE_DIRTY;

  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void LineToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  if (lmPtr->nSegments > 0) {
    Blt_Ps_XSetLineAttributes(ps, lmPtr->outlineColor, 
			      lmPtr->lineWidth, &lmPtr->dashes, lmPtr->capStyle,
			      lmPtr->joinStyle);
    if ((LineIsDashed(lmPtr->dashes)) && (lmPtr->fillColor)) {
      Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
      Blt_Ps_XSetBackground(ps, lmPtr->fillColor);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes*)NULL);
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
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (lmPtr->gc)
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);

  if (lmPtr->segments)
    free(lmPtr->segments);
}

static void MapLineProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  lmPtr->nSegments = 0;
  if (lmPtr->segments)
    free(lmPtr->segments);

  if (!lmPtr->worldPts || (lmPtr->worldPts->num < 2))
    return;

  Region2d extents;
  Blt_GraphExtents(graphPtr, &extents);

  /* 
   * Allow twice the number of world coordinates. The line will represented
   * as series of line segments, not one continous polyline.  This is
   * because clipping against the plot area may chop the line into several
   * disconnected segments.
   */
  Segment2d* segments = 
    (Segment2d*)malloc(lmPtr->worldPts->num * sizeof(Segment2d));
  Point2d* srcPtr = lmPtr->worldPts->points;
  Point2d p = Blt_MapPoint(srcPtr, &markerPtr->axes);
  p.x += markerPtr->xOffset;
  p.y += markerPtr->yOffset;

  Segment2d* segPtr = segments;
  Point2d* pend;
  for (srcPtr++, pend = lmPtr->worldPts->points + lmPtr->worldPts->num; 
       srcPtr < pend; srcPtr++) {
    Point2d next = Blt_MapPoint(srcPtr, &markerPtr->axes);
    next.x += lmPtr->xOffset;
    next.y += lmPtr->yOffset;
    Point2d q = next;

    if (Blt_LineRectClip(&extents, &p, &q)) {
      segPtr->p = p;
      segPtr->q = q;
      segPtr++;
    }
    p = next;
  }
  lmPtr->nSegments = segPtr - segments;
  lmPtr->segments = segments;
  lmPtr->clipped = (lmPtr->nSegments == 0);
}

