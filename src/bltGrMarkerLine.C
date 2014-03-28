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

static MarkerClass lineMarkerClass = {
  optionSpecs,
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

int LineMarker::configure()
{
  Graph* graphPtr = obj.graphPtr;
  LineMarkerOptions* opp = (LineMarkerOptions*)ops;

  Drawable drawable = Tk_WindowId(graphPtr->tkwin);
  unsigned long gcMask = (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle);
  XGCValues gcValues;
  if (opp->outlineColor) {
    gcMask |= GCForeground;
    gcValues.foreground = opp->outlineColor->pixel;
  }
  if (opp->fillColor) {
    gcMask |= GCBackground;
    gcValues.background = opp->fillColor->pixel;
  }
  gcValues.cap_style = opp->capStyle;
  gcValues.join_style = opp->joinStyle;
  gcValues.line_width = LineWidth(opp->lineWidth);
  gcValues.line_style = LineSolid;
  if (LineIsDashed(opp->dashes)) {
    gcValues.line_style = 
      (gcMask & GCBackground) ? LineDoubleDash : LineOnOffDash;
  }
  if (opp->xorr) {
    unsigned long pixel;
    gcValues.function = GXxor;

    gcMask |= GCFunction;
    pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
    if (gcMask & GCBackground)
      gcValues.background ^= pixel;

    gcValues.foreground ^= pixel;
    if (drawable != None)
      draw(drawable);
  }

  GC newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (gc)
    Blt_FreePrivateGC(graphPtr->display, gc);

  if (LineIsDashed(opp->dashes))
    Blt_SetDashes(graphPtr->display, newGC, &opp->dashes);

  gc = newGC;
  if (opp->xorr) {
    if (drawable != None) {
      map();
      draw(drawable);
    }
    return TCL_OK;
  }

  return TCL_OK;
}

void LineMarker::draw(Drawable drawable)
{
  Graph* graphPtr = obj.graphPtr;
  LineMarkerOptions* opp = (LineMarkerOptions*)ops;

  if (nSegments > 0) {
    Blt_Draw2DSegments(graphPtr->display, drawable, gc, segments, nSegments);
    if (opp->xorr)
      xorState = (xorState == 0);
  }
}

void LineMarker::map()
{
  Graph* graphPtr = obj.graphPtr;
  LineMarkerOptions* opp = (LineMarkerOptions*)ops;

  nSegments = 0;
  if (segments)
    free(segments);

  if (!opp->worldPts || (opp->worldPts->num < 2))
    return;

  Region2d extents;
  Blt_GraphExtents(graphPtr, &extents);

  // Allow twice the number of world coordinates. The line will represented
  // as series of line segments, not one continous polyline.  This is
  // because clipping against the plot area may chop the line into several
  // disconnected segments.
  Segment2d* lsegments = (Segment2d*)malloc(opp->worldPts->num * sizeof(Segment2d));
  Point2d* srcPtr = opp->worldPts->points;
  Point2d p = mapPoint(srcPtr, &opp->axes);
  p.x += opp->xOffset;
  p.y += opp->yOffset;

  Segment2d* segPtr = lsegments;
  Point2d* pend;
  for (srcPtr++, pend = opp->worldPts->points + opp->worldPts->num; 
       srcPtr < pend; srcPtr++) {
    Point2d next = mapPoint(srcPtr, &opp->axes);
    next.x += opp->xOffset;
    next.y += opp->yOffset;
    Point2d q = next;

    if (Blt_LineRectClip(&extents, &p, &q)) {
      segPtr->p = p;
      segPtr->q = q;
      segPtr++;
    }
    p = next;
  }
  nSegments = segPtr - lsegments;
  segments = lsegments;
  clipped = (nSegments == 0);
}

int LineMarker::pointIn(Point2d *samplePtr)
{
  return Blt_PointInSegments(samplePtr, segments, nSegments, 
			     (double)obj.graphPtr->search.halo);
}

int LineMarker::regionIn(Region2d *extsPtr, int enclosed)
{
  LineMarkerOptions* opp = (LineMarkerOptions*)ops;

  if (!opp->worldPts || opp->worldPts->num < 2)
    return FALSE;

  if (enclosed) {
    Point2d *pp, *pend;

    for (pp = opp->worldPts->points, pend = pp + opp->worldPts->num; 
	 pp < pend; pp++) {
      Point2d p = mapPoint(pp, &opp->axes);
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
    for (pp = opp->worldPts->points, pend = pp + (opp->worldPts->num - 1); 
	 pp < pend; pp++) {
      Point2d p = mapPoint(pp, &opp->axes);
      Point2d q = mapPoint(pp + 1, &opp->axes);
      if (Blt_LineRectClip(extsPtr, &p, &q))
	count++;
    }
    return (count > 0);		/* At least 1 segment passes through
				 * region. */
  }
}

void LineMarker::postscript(Blt_Ps ps)
{
  LineMarkerOptions* opp = (LineMarkerOptions*)ops;

  if (nSegments > 0) {
    Blt_Ps_XSetLineAttributes(ps, opp->outlineColor, opp->lineWidth,
			      &opp->dashes, opp->capStyle, opp->joinStyle);
    if ((LineIsDashed(opp->dashes)) && (opp->fillColor)) {
      Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
      Blt_Ps_XSetBackground(ps, opp->fillColor);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes*)NULL);
      Blt_Ps_VarAppend(ps, "stroke\n", "  grestore\n", "} def\n", (char*)NULL);
    } 
    else
      Blt_Ps_Append(ps, "/DashesProc {} def\n");

    Blt_Ps_Draw2DSegments(ps, segments, nSegments);
  }
}


