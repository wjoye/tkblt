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

#include "bltGrMarkerLine.h"

using namespace Blt;

// OptionSpecs

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "Line all", -1, Tk_Offset(LineMarkerOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_CUSTOM, "-cap", "cap", "Cap", 
   "butt", -1, Tk_Offset(LineMarkerOptions, capStyle),
   0, &capStyleObjOption, 0},
  {TK_OPTION_CUSTOM, "-coords", "coords", "Coords",
   NULL, -1, Tk_Offset(LineMarkerOptions, worldPts), 
   TK_OPTION_NULL_OK, &coordsObjOption, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes",
   NULL, -1, Tk_Offset(LineMarkerOptions, dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_PIXELS, "-dashoffset", "dashOffset", "DashOffset",
   "0", -1, Tk_Offset(LineMarkerOptions, dashes.offset), 0, NULL, 0},
  {TK_OPTION_STRING, "-element", "element", "Element", 
   NULL, -1, Tk_Offset(LineMarkerOptions, elemName),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill",
   NULL, -1, Tk_Offset(LineMarkerOptions, fillColor),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-join", "join", "Join", 
   "miter", -1, Tk_Offset(LineMarkerOptions, joinStyle),
   0, &joinStyleObjOption, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(LineMarkerOptions, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(LineMarkerOptions, hide), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(LineMarkerOptions, axes.x), 0, &xAxisObjOption, 0},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY", 
   "y", -1, Tk_Offset(LineMarkerOptions, axes.y), 0, &yAxisObjOption, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LineMarkerOptions, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-state", "state", "State", 
   "normal", -1, Tk_Offset(LineMarkerOptions, state), 0, &stateObjOption, 0},
  {TK_OPTION_BOOLEAN, "-under", "under", "Under",
   "no", -1, Tk_Offset(LineMarkerOptions, drawUnder), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-xoffset", "xOffset", "XOffset",
   "0", -1, Tk_Offset(LineMarkerOptions, xOffset), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-xor", "xor", "Xor",
   "no", -1, Tk_Offset(LineMarkerOptions, xorr), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-yoffset", "yOffset", "YOffset",
   "0", -1, Tk_Offset(LineMarkerOptions, yOffset), 0, NULL, 0},
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

LineMarker::LineMarker(Graph* graphPtr, const char* name) 
  : Marker(graphPtr, name)
{
  obj.classId = CID_MARKER_LINE;
  obj.className = dupstr("LineMarker");
  classPtr = &lineMarkerClass;
  ops = (LineMarkerOptions*)calloc(1, sizeof(LineMarkerOptions));
  ((LineMarkerOptions*)ops)->xorr = FALSE;
  optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  gc =NULL;
  segments =NULL;
  nSegments =0;
  xorState =0;
}

LineMarker::~LineMarker()
{
  Graph* graphPtr = obj.graphPtr;

  if (gc)
    Blt_FreePrivateGC(graphPtr->display, gc);
  if (segments)
    free(segments);
}

Marker* Blt_CreateLineProc(Graph* graphPtr)
{
  LineMarker* lmPtr = (LineMarker*)calloc(1, sizeof(LineMarker));

  lmPtr->classPtr = &lineMarkerClass;
  lmPtr->ops = (LineMarkerOptions*)calloc(1, sizeof(LineMarkerOptions));
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  ops->xorr = FALSE;
  lmPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Marker*)lmPtr;
}

static int PointInLineProc(Marker* markerPtr, Point2d *samplePtr)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;

  return Blt_PointInSegments(samplePtr, lmPtr->segments, lmPtr->nSegments, 
			     (double)markerPtr->obj.graphPtr->search.halo);
}

static int RegionInLineProc(Marker* markerPtr, Region2d *extsPtr, int enclosed)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  if (!ops->worldPts || ops->worldPts->num < 2)
    return FALSE;

  if (enclosed) {
    Point2d *pp, *pend;

    for (pp = ops->worldPts->points, pend = pp + ops->worldPts->num; 
	 pp < pend; pp++) {
      Point2d p = Blt_MapPoint(pp, &ops->axes);
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
    for (pp = ops->worldPts->points, pend = pp + (ops->worldPts->num - 1); 
	 pp < pend; pp++) {
      Point2d p = Blt_MapPoint(pp, &ops->axes);
      Point2d q = Blt_MapPoint(pp + 1, &ops->axes);
      if (Blt_LineRectClip(extsPtr, &p, &q))
	count++;
    }
    return (count > 0);		/* At least 1 segment passes through
				 * region. */
  }
}

static void DrawLineProc(Marker* markerPtr, Drawable drawable)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  if (lmPtr->nSegments > 0) {
    Graph* graphPtr = markerPtr->obj.graphPtr;

    Blt_Draw2DSegments(graphPtr->display, drawable, lmPtr->gc, 
		       lmPtr->segments, lmPtr->nSegments);
    if (ops->xorr)
      lmPtr->xorState = (lmPtr->xorState == 0);
  }
}

static int ConfigureLineProc(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  Drawable drawable = Tk_WindowId(graphPtr->tkwin);
  unsigned long gcMask = (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle);
  XGCValues gcValues;
  if (ops->outlineColor) {
    gcMask |= GCForeground;
    gcValues.foreground = ops->outlineColor->pixel;
  }
  if (ops->fillColor) {
    gcMask |= GCBackground;
    gcValues.background = ops->fillColor->pixel;
  }
  gcValues.cap_style = ops->capStyle;
  gcValues.join_style = ops->joinStyle;
  gcValues.line_width = LineWidth(ops->lineWidth);
  gcValues.line_style = LineSolid;
  if (LineIsDashed(ops->dashes)) {
    gcValues.line_style = 
      (gcMask & GCBackground) ? LineDoubleDash : LineOnOffDash;
  }
  if (ops->xorr) {
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

  if (LineIsDashed(ops->dashes))
    Blt_SetDashes(graphPtr->display, newGC, &ops->dashes);

  lmPtr->gc = newGC;
  if (ops->xorr) {
    if (drawable != None) {
      MapLineProc(markerPtr);
      DrawLineProc(markerPtr, drawable);
    }
    return TCL_OK;
  }

  return TCL_OK;
}

static void LineToPostscriptProc(Marker* markerPtr, Blt_Ps ps)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  if (lmPtr->nSegments > 0) {
    Blt_Ps_XSetLineAttributes(ps, ops->outlineColor, 
			      ops->lineWidth,
			      &ops->dashes,
			      ops->capStyle,
			      ops->joinStyle);
    if ((LineIsDashed(ops->dashes)) && (ops->fillColor)) {
      Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
      Blt_Ps_XSetBackground(ps, ops->fillColor);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes*)NULL);
      Blt_Ps_VarAppend(ps,
		       "stroke\n",
		       "  grestore\n",
		       "} def\n", (char*)NULL);
    } 
    else
      Blt_Ps_Append(ps, "/DashesProc {} def\n");

    Blt_Ps_Draw2DSegments(ps, lmPtr->segments, lmPtr->nSegments);
  }
}

static void FreeLineProc(Marker* markerPtr)
{
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  if (lmPtr->gc)
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);

  if (lmPtr->segments)
    free(lmPtr->segments);

  if (ops)
    free(ops);
}

static void MapLineProc(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker*)markerPtr;
  LineMarkerOptions* ops = (LineMarkerOptions*)lmPtr->ops;

  lmPtr->nSegments = 0;
  if (lmPtr->segments)
    free(lmPtr->segments);

  if (!ops->worldPts || (ops->worldPts->num < 2))
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
    (Segment2d*)malloc(ops->worldPts->num * sizeof(Segment2d));
  Point2d* srcPtr = ops->worldPts->points;
  Point2d p = Blt_MapPoint(srcPtr, &ops->axes);
  p.x += ops->xOffset;
  p.y += ops->yOffset;

  Segment2d* segPtr = segments;
  Point2d* pend;
  for (srcPtr++, pend = ops->worldPts->points + ops->worldPts->num; 
       srcPtr < pend; srcPtr++) {
    Point2d next = Blt_MapPoint(srcPtr, &ops->axes);
    next.x += ops->xOffset;
    next.y += ops->yOffset;
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

