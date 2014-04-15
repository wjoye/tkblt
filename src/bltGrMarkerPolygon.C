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

#include "bltGrMarkerPolygon.h"
#include "bltGrMarkerOption.h"
#include "bltGrMisc.h"

using namespace Blt;

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
  ops_ = (PolygonMarkerOptions*)calloc(1, sizeof(PolygonMarkerOptions));
  optionTable_ = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  screenPts_ =NULL;
  outlineGC_ =NULL;
  fillGC_ =NULL;
  fillPts_ =NULL;
  nFillPts_ =0;
  outlinePts_ =NULL;
  nOutlinePts_ =0;
}

PolygonMarker::~PolygonMarker()
{
  if (fillGC_)
    Tk_FreeGC(graphPtr_->display, fillGC_);
  if (outlineGC_)
    Blt_FreePrivateGC(graphPtr_->display, outlineGC_);
  if (fillPts_)
    delete [] fillPts_;
  if (outlinePts_)
    delete [] outlinePts_;
  if (screenPts_)
    delete [] screenPts_;
}

int PolygonMarker::configure()
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  Drawable drawable = Tk_WindowId(graphPtr_->tkwin);
  unsigned long gcMask = (GCLineWidth | GCLineStyle);
  XGCValues gcValues;
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
    pixel = Tk_3DBorderColor(graphPtr_->plotBg)->pixel;
    if (gcMask & GCBackground)
      gcValues.background ^= pixel;

    gcValues.foreground ^= pixel;
    if (drawable != None)
      draw(drawable);
  }

  // outlineGC
  GC newGC = Blt_GetPrivateGC(graphPtr_->tkwin, gcMask, &gcValues);
  if (LineIsDashed(ops->dashes))
    Blt_SetDashes(graphPtr_->display, newGC, &ops->dashes);
  if (outlineGC_)
    Blt_FreePrivateGC(graphPtr_->display, outlineGC_);
  outlineGC_ = newGC;

  // fillGC
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
  newGC = Tk_GetGC(graphPtr_->tkwin, gcMask, &gcValues);
  if (fillGC_)
    Tk_FreeGC(graphPtr_->display, fillGC_);
  fillGC_ = newGC;

  if ((gcMask == 0) && !(graphPtr_->flags & RESET_AXES) && ops->xorr) {
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
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  // fill region
  if ((nFillPts_ > 0) && (ops->fill)) {
    XPoint* points = new XPoint[nFillPts_];
    if (!points)
      return;

    XPoint* dp = points;
    Point2d *sp, *send;
    for (sp = fillPts_, send = sp + nFillPts_; sp < send; sp++) {
      dp->x = (short int)sp->x;
      dp->y = (short int)sp->y;
      dp++;
    }

    XFillPolygon(graphPtr_->display, drawable, fillGC_, points, 
		 nFillPts_, Complex, CoordModeOrigin);
    delete [] points;
  }

  // outline
  if ((nOutlinePts_ > 0) && (ops->lineWidth > 0) && 
      (ops->outline)) {
    Blt_Draw2DSegments(graphPtr_->display, drawable, outlineGC_,
		       outlinePts_, nOutlinePts_);
  }
}

void PolygonMarker::map()
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  if (outlinePts_) {
    delete [] outlinePts_;
    outlinePts_ = NULL;
    nOutlinePts_ = 0;
  }

  if (fillPts_) {
    delete [] fillPts_;
    fillPts_ = NULL;
    nFillPts_ = 0;
  }

  if (screenPts_) {
    delete [] screenPts_;
    screenPts_ = NULL;
  }

  if (!ops->worldPts || ops->worldPts->num < 3)
    return;

  // Allocate and fill a temporary array to hold the screen coordinates of
  // the polygon.

  int nScreenPts = ops->worldPts->num + 1;
  Point2d* screenPts = new Point2d[nScreenPts + 1];
  {
    Point2d* dp = screenPts;
    Point2d *sp, *send;
    for (sp = ops->worldPts->points, send = sp + ops->worldPts->num; 
	 sp < send; sp++) {
      *dp = mapPoint(sp, &ops->axes);
      dp->x += ops->xOffset;
      dp->y += ops->yOffset;
      dp++;
    }
    *dp = screenPts[0];
  }
  Region2d extents;
  Blt_GraphExtents(graphPtr_, &extents);
  clipped_ = 1;
  if (ops->fill) {
    Point2d* lfillPts = new Point2d[nScreenPts * 3];
    int n = 
      Blt_PolyRectClip(&extents, screenPts, ops->worldPts->num,lfillPts);
    if (n < 3)
      delete [] lfillPts;
    else {
      nFillPts_ = n;
      fillPts_ = lfillPts;
      clipped_ = 0;
    }
  }
  if ((ops->outline) && (ops->lineWidth > 0)) { 
    // Generate line segments representing the polygon outline.  The
    // resulting outline may or may not be closed from viewport clipping.
    Segment2d* outlinePts = new Segment2d[nScreenPts];
    if (!outlinePts)
      return;

    // Note that this assumes that the point array contains an extra point
    // that closes the polygon.
    Segment2d* segPtr = outlinePts;
    Point2d *sp, *send;
    for (sp = screenPts, send = sp + (nScreenPts - 1); sp < send; sp++) {
      segPtr->p = sp[0];
      segPtr->q = sp[1];
      if (Blt_LineRectClip(&extents, &segPtr->p, &segPtr->q)) {
	segPtr++;
      }
    }
    nOutlinePts_ = segPtr - outlinePts;
    outlinePts_ = outlinePts;
    if (nOutlinePts_ > 0)
      clipped_ = 0;
  }

  screenPts_ = screenPts;
}

int PolygonMarker::pointIn(Point2d *samplePtr)
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  if (ops->worldPts && (ops->worldPts->num >= 3) && screenPts_)
    return Blt_PointInPolygon(samplePtr, screenPts_, ops->worldPts->num + 1);

  return 0;
}

int PolygonMarker::regionIn(Region2d *extsPtr, int enclosed)
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;
    
  if (ops->worldPts && (ops->worldPts->num >= 3) && screenPts_)
    return regionInPolygon(extsPtr, screenPts_, ops->worldPts->num, enclosed);

  return 0;
}

void PolygonMarker::postscript(Blt_Ps ps)
{
  PolygonMarkerOptions* ops = (PolygonMarkerOptions*)ops_;

  if (ops->fill) {
    Blt_Ps_Polyline(ps, fillPts_, nFillPts_);
    if (ops->fillBg) {
      Blt_Ps_XSetBackground(ps, ops->fillBg);
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    }
    Blt_Ps_XSetForeground(ps, ops->fill);
    if (ops->stipple != None) {
      Blt_Ps_XSetStipple(ps, graphPtr_->display, ops->stipple);
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
    Blt_Ps_Draw2DSegments(ps, outlinePts_, nOutlinePts_);
  }
}

