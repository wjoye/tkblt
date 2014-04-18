/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1991-2004 George A Howlett.
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
#include "bltInt.h"
#include "bltGraph.h"
}

#include "bltGraphOp.h"

#include "bltGrAxis.h"
#include "bltGrAxisOp.h"
#include "bltGrXAxisOp.h"
#include "bltGrPen.h"
#include "bltGrPenBar.h"
#include "bltGrPenLine.h"
#include "bltGrPenOp.h"
#include "bltGrElem.h"
#include "bltGrElemBar.h"
#include "bltGrElemLine.h"
#include "bltGrElemOp.h"
#include "bltGrMarker.h"
#include "bltGrMarkerOp.h"
#include "bltGrLegd.h"
#include "bltGrLegdOp.h"
#include "bltGrHairs.h"
#include "bltGrHairsOp.h"
#include "bltGrDef.h"

using namespace Blt;

#define MARKER_ABOVE	0
#define MARKER_UNDER	1

extern int Blt_CreatePageSetup(Graph* graphPtr);
extern void Blt_DestroyPageSetup(Graph* graphPtr);
extern void Blt_LayoutGraph(Graph* graphPtr);
extern int PostScriptPreamble(Graph* graphPtr, const char *fileName, Blt_Ps ps);

static Blt_BindPickProc PickEntry;

// OptionSpecs

static const char* barmodeObjOption[] = 
  {"normal", "stacked", "aligned", "overlap", NULL};
const char* searchModeObjOption[] = {"points", "traces", "auto", NULL};
const char* searchAlongObjOption[] = {"x", "y", "both", NULL};

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_DOUBLE, "-aspect", "aspect", "Aspect", 
   "0", -1, Tk_Offset(GraphOptions, aspect), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   STD_NORMAL_BACKGROUND, -1, Tk_Offset(GraphOptions, normalBg), 
   0, NULL, CACHE_DIRTY},
  {TK_OPTION_STRING_TABLE, "-barmode", "barMode", "BarMode", 
   "normal", -1, Tk_Offset(GraphOptions, barMode), 
   0, &barmodeObjOption, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_DOUBLE, "-barwidth", "barWidth", "BarWidth", 
   ".9", -1, Tk_Offset(GraphOptions, barWidth), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_DOUBLE, "-baseline", "baseline", "Baseline", 
   "0", -1, Tk_Offset(GraphOptions, baseline), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_SYNONYM, "-bm", NULL, NULL, NULL, -1, 0, 0, "-bottommargin", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(GraphOptions, borderWidth), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-bottommargin", "bottomMargin", "BottomMargin",
   "0", -1, Tk_Offset(GraphOptions, bottomMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-bottomvariable", "bottomVariable", "BottomVariable",
   NULL, -1, Tk_Offset(GraphOptions, bottomMargin.varName), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-bufferelements", "bufferElements", "BufferElements",
   "yes", -1, Tk_Offset(GraphOptions, backingStore), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-buffergraph", "bufferGraph", "BufferGraph",
   "yes", -1, Tk_Offset(GraphOptions, doubleBuffer), 0, NULL, 0},
  {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor", 
   "crosshair", -1, Tk_Offset(GraphOptions, cursor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_MEDIUM, -1, Tk_Offset(GraphOptions, titleTextStyle.font), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(GraphOptions, titleTextStyle.color), 
   0, NULL, 
   CACHE_DIRTY},
  {TK_OPTION_SYNONYM, "-halo", NULL, NULL, NULL, -1, 0, 0, "-searchhalo", 0},
  {TK_OPTION_PIXELS, "-height", "height", "Height", 
   "4i", -1, Tk_Offset(GraphOptions, reqHeight), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
   "HighlightBackground", 
   STD_NORMAL_BACKGROUND, -1, Tk_Offset(GraphOptions, highlightBgColor), 
   0, NULL, 
   CACHE_DIRTY},
  {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(GraphOptions, highlightColor), 
   0, NULL, 0},
  {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
   "HighlightThickness", 
   "2", -1, Tk_Offset(GraphOptions, highlightWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-invertxy", "invertXY", "InvertXY", 
   "no", -1, Tk_Offset(GraphOptions, inverted), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY | RESET_AXES},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify", 
   "center", -1, Tk_Offset(GraphOptions, titleTextStyle.justify), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-leftmargin", "leftMargin", "Margin", 
   "0", -1, Tk_Offset(GraphOptions, leftMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-leftvariable", "leftVariable", "LeftVariable",
   NULL, -1, Tk_Offset(GraphOptions, leftMargin.varName), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-lm", NULL, NULL, NULL, -1, 0, 0, "-leftmargin", 0},
  {TK_OPTION_BORDER, "-plotbackground", "plotbackground", "PlotBackground",
   STD_NORMAL_BACKGROUND, -1, Tk_Offset(GraphOptions, plotBg), 0, NULL,
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotborderwidth", "plotBorderWidth", "PlotBorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(GraphOptions, plotBW), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotpadx", "plotPadX", "PlotPad", 
   "0", -1, Tk_Offset(GraphOptions, xPad), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotpady", "plotPadY", "PlotPad", 
   "0", -1, Tk_Offset(GraphOptions, yPad), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_RELIEF, "-plotrelief", "plotRelief", "Relief", 
   "flat", -1, Tk_Offset(GraphOptions, plotRelief), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief", 
   "flat", -1, Tk_Offset(GraphOptions, relief), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-rightmargin", "rightMargin", "Margin", 
   "0", -1, Tk_Offset(GraphOptions, rightMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-rightvariable", "rightVariable", "RightVariable",
   NULL, -1, Tk_Offset(GraphOptions, rightMargin.varName), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-rm", NULL, NULL, NULL, -1, 0, 0, "-rightmargin", 0},
  {TK_OPTION_PIXELS, "-searchhalo", "searchhalo", "SearchHalo", 
   "2m", -1, Tk_Offset(GraphOptions, search.halo), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-searchmode", "searchMode", "SearchMode",
   "points", -1, Tk_Offset(GraphOptions, search.mode), 
   0, &searchModeObjOption, 0}, 
  {TK_OPTION_STRING_TABLE, "-searchalong", "searchAlong", "SearchAlong",
   "both", -1, Tk_Offset(GraphOptions, search.along), 
   0, &searchAlongObjOption, 0},
  {TK_OPTION_BOOLEAN, "-stackaxes", "stackAxes", "StackAxes", 
   "no", -1, Tk_Offset(GraphOptions, stackAxes), 0, NULL, 0},
  {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
   NULL, -1, Tk_Offset(GraphOptions, takeFocus), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title", 
   NULL, -1, Tk_Offset(GraphOptions, title), 
   TK_OPTION_NULL_OK, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_SYNONYM, "-tm", NULL, NULL, NULL, -1, 0, 0, "-topmargin", 0},
  {TK_OPTION_PIXELS, "-topmargin", "topMargin", "TopMargin", 
   "0", -1, Tk_Offset(GraphOptions, topMargin.reqSize), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-topvariable", "topVariable", "TopVariable",
   NULL, -1, Tk_Offset(GraphOptions, topMargin.varName), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-width", "width", "Width", 
   "5i", -1, Tk_Offset(GraphOptions, reqWidth), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotwidth", "plotWidth", "PlotWidth", 
   "0", -1, Tk_Offset(GraphOptions, reqPlotWidth), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotheight", "plotHeight", "PlotHeight", 
   "0", -1, Tk_Offset(GraphOptions, reqPlotHeight), 
   0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

Graph::Graph(ClientData clientData, Tcl_Interp* interp, 
	     int objc, Tcl_Obj* const objv[], ClassId classId)
{
  valid_ =1;
  interp_ = interp;
  tkwin_ = Tk_CreateWindowFromPath(interp_, Tk_MainWindow(interp_), 
				  Tcl_GetString(objv[1]), NULL);
  if (!tkwin_) {
    valid_ =0;
    return;
  }
  display_ = Tk_Display(tkwin_);
  ((TkWindow*)tkwin_)->instanceData = this;

  cmdToken_ = Tcl_CreateObjCommand(interp_, Tk_PathName(tkwin_),
				   GraphInstCmdProc, this,
				   GraphInstCmdDeleteProc);
  optionTable_ = Tk_CreateOptionTable(interp_, optionSpecs);
  ops_ = (GraphOptions*)calloc(1, sizeof(GraphOptions));
  GraphOptions* ops = (GraphOptions*)ops_;

  switch (classId) {
  case CID_ELEM_LINE:
    Tk_SetClass(tkwin_, "Graph");
    break;
  case CID_ELEM_BAR:
    Tk_SetClass(tkwin_, "Barchart");
    break;
  default:
    break;
  }

  classId_ = classId;
  flags = MAP_WORLD | REDRAW_WORLD;
  nextMarkerId_ = 1;

  legend_ = new Legend(this);
  crosshairs_ = new Crosshairs(this);

  inset_ =0;
  titleX_ =0;
  titleY_ =0;
  titleWidth_ =0;
  titleHeight_ =0;
  width_ =0;
  height_ =0;
  left_ =0;
  right_ =0;
  top_ =0;
  bottom_ =0;
  focusPtr_ =NULL;
  halo_ =0;
  drawGC_ =NULL;
  vRange_ =0;
  hRange_ =0;
  vOffset_ =0;
  hOffset_ =0;
  vScale_ =0;
  hScale_ =0;
  cache_ =None;
  cacheWidth_ =0;
  cacheHeight_ =0;

  barGroups_ =NULL;
  nBarGroups_ =0;
  maxBarSetSize_ =0;

  ops->bottomMargin.site = MARGIN_BOTTOM;
  ops->leftMargin.site = MARGIN_LEFT;
  ops->topMargin.site = MARGIN_TOP;
  ops->rightMargin.site = MARGIN_RIGHT;

  Blt_Ts_InitStyle(ops->titleTextStyle);
  ops->titleTextStyle.anchor = TK_ANCHOR_N;

  Tcl_InitHashTable(&axes_.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&axes_.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&elements_.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&elements_.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&markers_.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&markers_.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&penTable_, TCL_STRING_KEYS);
  Tcl_InitHashTable(&setTable_, sizeof(BarSetKey) / sizeof(int));
  axes_.displayList = Blt_Chain_Create();
  elements_.displayList = Blt_Chain_Create();
  markers_.displayList = Blt_Chain_Create();
  bindTable_ = Blt_CreateBindingTable(interp_, tkwin_, this, 
				      PickEntry, Blt_GraphTags);

  if (createAxes() != TCL_OK) {
    valid_ =0;
    return;
  }

  switch (classId) {
  case CID_ELEM_LINE:
    if (createPen("active", 0, NULL) != TCL_OK) {
      valid_ =0;
      return;
    }
    break;
  case CID_ELEM_BAR:
    if (createPen("active", 0, NULL) != TCL_OK) {
      valid_ =0;
      return;
    }
    break;
  default:
    break;
  }

  if (Blt_CreatePageSetup(this) != TCL_OK) {
    valid_ =0;
    return;
  }

  // Keep a hold of the associated tkwin until we destroy the graph,
  // otherwise Tk might free it while we still need it.
  Tcl_Preserve(tkwin_);

  Tk_CreateEventHandler(tkwin_, 
			ExposureMask|StructureNotifyMask|FocusChangeMask,
			GraphEventProc, this);

  if ((Tk_InitOptions(interp_, (char*)ops_, optionTable_, tkwin_) != TCL_OK) || (GraphObjConfigure(interp_, this, objc-2, objv+2) != TCL_OK)) {
    valid_ =0;
    return;
  }

  adjustAxes();

  Tcl_SetStringObj(Tcl_GetObjResult(interp_), Tk_PathName(tkwin_), -1);
}

Graph::~Graph()
{
  GraphOptions* ops = (GraphOptions*)ops_;

  destroyMarkers();
  destroyElements();  // must come before legend and others

  if (crosshairs_)
    delete crosshairs_;
  if (legend_)
    delete legend_;

  destroyAxes();
  destroyPens();
  Blt_DestroyPageSetup(this);
  Blt_DestroyBarSets(this);

  if (bindTable_)
    Blt_DestroyBindingTable(bindTable_);

  if (drawGC_)
    Tk_FreeGC(display_, drawGC_);

  Blt_Ts_FreeStyle(display_, &ops->titleTextStyle);
  if (cache_ != None)
    Tk_FreePixmap(display_, cache_);

  Tk_FreeConfigOptions((char*)ops_, optionTable_, tkwin_);
  Tcl_Release(tkwin_);
  tkwin_ = NULL;

  if (ops_)
    free (ops_);
}

void Graph::configure()	
{
  GraphOptions* ops = (GraphOptions*)ops_;

  // Don't allow negative bar widths. Reset to an arbitrary value (0.1)
  if (ops->barWidth <= 0.0f)
    ops->barWidth = 0.8f;

  inset_ = ops->borderWidth + ops->highlightWidth;
  if ((ops->reqHeight != Tk_ReqHeight(tkwin_)) ||
      (ops->reqWidth != Tk_ReqWidth(tkwin_)))
    Tk_GeometryRequest(tkwin_, ops->reqWidth, ops->reqHeight);

  Tk_SetInternalBorder(tkwin_, ops->borderWidth);
  XColor* colorPtr = Tk_3DBorderColor(ops->normalBg);

  titleWidth_ =0;
  titleHeight_ =0;
  if (ops->title != NULL) {
    unsigned int w, h;
    Blt_Ts_GetExtents(&ops->titleTextStyle, ops->title, &w, &h);
    titleHeight_ = h;
  }

  // Create GCs for interior and exterior regions, and a background GC for
  // clearing the margins with XFillRectangle
  // Margin
  XGCValues gcValues;
  gcValues.foreground = ops->titleTextStyle.color->pixel;
  gcValues.background = colorPtr->pixel;
  unsigned long gcMask = (GCForeground | GCBackground);
  GC newGC = Tk_GetGC(tkwin_, gcMask, &gcValues);
  if (drawGC_ != NULL)
    Tk_FreeGC(display_, drawGC_);
  drawGC_ = newGC;

  // If the -inverted option changed, we need to readjust the pointers
  // to the axes and recompute the their scales.
  if (flags & RESET_AXES)
    adjustAxes();

  // Free the pixmap if we're not buffering the display of elements anymore.
  if ((!ops->backingStore) && (cache_ != None)) {
    Tk_FreePixmap(display_, cache_);
    cache_ = None;
  }
}

void Graph::map()
{
  if (flags & RESET_AXES)
    resetAxes();

  if (flags & LAYOUT_NEEDED) {
    Blt_LayoutGraph(this);
    flags &= ~LAYOUT_NEEDED;
  }

  if ((vRange_ > 1) && (hRange_ > 1)) {
    if (flags & MAP_WORLD)
      mapAxes();

    mapElements();
    mapMarkers();
    flags &= ~(MAP_ALL);
  }
}

void Graph::draw()
{
  GraphOptions* ops = (GraphOptions*)ops_;

  flags &= ~REDRAW_PENDING;
  if ((flags & GRAPH_DELETED) || !Tk_IsMapped(tkwin_))
    return;

  // Don't bother computing the layout until the size of the window is
  // something reasonable.
  if ((Tk_Width(tkwin_) <= 1) || (Tk_Height(tkwin_) <= 1))
    return;

  width_ = Tk_Width(tkwin_);
  height_ = Tk_Height(tkwin_);
  map();

  // Create a pixmap the size of the window for double buffering
  Pixmap drawable;
  if (ops->doubleBuffer)
    drawable = Tk_GetPixmap(display_, Tk_WindowId(tkwin_), 
			    width_, height_, Tk_Depth(tkwin_));
  else
    drawable = Tk_WindowId(tkwin_);

  if (ops->backingStore) {
    if ((cache_ == None)||(cacheWidth_ != width_)||(cacheHeight_ != height_)) {
      if (cache_ != None)
	Tk_FreePixmap(display_, cache_);

      cache_ = Tk_GetPixmap(display_, Tk_WindowId(tkwin_), width_, height_, 
			    Tk_Depth(tkwin_));
      cacheWidth_  = width_;
      cacheHeight_ = height_;
      flags |= CACHE_DIRTY;
    }
  }

  if (ops->backingStore) {
    if (flags & CACHE_DIRTY) {
      drawPlot(cache_);
      flags &= ~CACHE_DIRTY;
    }
    XCopyArea(display_, cache_, drawable, drawGC_, 0, 0, Tk_Width(tkwin_),
	      Tk_Height(tkwin_), 0, 0);
  }
  else
    drawPlot(drawable);

  // Draw markers above elements
  drawActiveElements(drawable);

  if (legend_->isRaised()) {
    switch (legend_->position()) {
    case Legend::PLOT:
    case Legend::XY:
      legend_->draw(drawable);
      break;
    default:
      break;
    }
  }

  drawMarkers(drawable, MARKER_ABOVE);

  // Draw 3D border just inside of the focus highlight ring
  if ((ops->borderWidth > 0) && (ops->relief != TK_RELIEF_FLAT))
    Tk_Draw3DRectangle(tkwin_, drawable, ops->normalBg, 
		       ops->highlightWidth, ops->highlightWidth, 
		       width_ - 2*ops->highlightWidth, 
		       height_ - 2*ops->highlightWidth, 
		       ops->borderWidth, ops->relief);

  // Draw focus highlight ring
  if ((ops->highlightWidth > 0) && (flags & FOCUS)) {
    GC gc = Tk_GCForColor(ops->highlightColor, drawable);
    Tk_DrawFocusHighlight(tkwin_, gc, ops->highlightWidth,
			  drawable);
  }

  // Disable crosshairs before redisplaying to the screen
  disableCrosshairs();
  XCopyArea(display_, drawable, Tk_WindowId(tkwin_),
	    drawGC_, 0, 0, width_, height_, 0, 0);

  enableCrosshairs();
  if (ops->doubleBuffer)
    Tk_FreePixmap(display_, drawable);

  flags &= ~MAP_WORLD;
  flags &= ~REDRAW_WORLD;
  updateMarginTraces();
}

void Graph::drawPlot(Drawable drawable)
{
  GraphOptions* ops = (GraphOptions*)ops_;

  drawMargins(drawable);

  switch (legend_->position()) {
  case Legend::TOP:
  case Legend::BOTTOM:
  case Legend::RIGHT:
  case Legend::LEFT:
    legend_->draw(drawable);
    break;
  default:
    break;
  }

  // Draw the background of the plotting area with 3D border
  Tk_Fill3DRectangle(tkwin_, drawable, ops->plotBg, 
		     left_-ops->plotBW, top_-ops->plotBW, 
		     right_-left_+1+2*ops->plotBW,bottom_-top_+1+2*ops->plotBW, 
		     ops->plotBW, ops->plotRelief);
  
  drawAxes(drawable);
  drawAxesGrids(drawable);
  drawAxesLimits(drawable);
  drawMarkers(drawable, MARKER_UNDER);

  if (!legend_->isRaised()) {
    switch (legend_->position()) {
    case Legend::PLOT:
    case Legend::XY:
      legend_->draw(drawable);
      break;
    default:
      break;
    }
  }

  drawElements(drawable);
}

int Graph::print(const char *ident, Blt_Ps ps)
{
  GraphOptions* gops = (GraphOptions*)ops_;
  PageSetup *setupPtr = pageSetup_;

  // We need to know how big a graph to print.  If the graph hasn't been drawn
  // yet, the width and height will be 1.  Instead use the requested size of
  // the widget.  The user can still override this with the -width and -height
  // postscript options.
  if (setupPtr->reqWidth > 0)
    width_ = setupPtr->reqWidth;
  else if (width_ < 2)
    width_ = Tk_ReqWidth(tkwin_);

  if (setupPtr->reqHeight > 0)
    height_ = setupPtr->reqHeight;
  else if (height_ < 2)
    height_ = Tk_ReqHeight(tkwin_);

  Blt_Ps_ComputeBoundingBox(setupPtr, width_, height_);
  flags |= LAYOUT_NEEDED | RESET_WORLD;

  /* Turn on PostScript measurements when computing the graph's layout. */
  Blt_Ps_SetPrinting(ps, 1);
  reconfigure();
  map();

  int x = left_ - gops->plotBW;
  int y = top_ - gops->plotBW;

  int w = (right_ - left_ + 1) + (2*gops->plotBW);
  int h = (bottom_ - top_ + 1) + (2*gops->plotBW);

  int result = PostScriptPreamble(this, ident, ps);
  if (result != TCL_OK)
    goto error;

  Blt_Ps_XSetFont(ps, gops->titleTextStyle.font);
  if (pageSetup_->decorations)
    Blt_Ps_XSetBackground(ps, Tk_3DBorderColor(gops->plotBg));
  else
    Blt_Ps_SetClearBackground(ps);

  Blt_Ps_XFillRectangle(ps, x, y, w, h);
  Blt_Ps_Rectangle(ps, x, y, w, h);
  Blt_Ps_Append(ps, "gsave clip\n\n");

  // Start
  printMargins(ps);

  switch (legend_->position()) {
  case Legend::TOP:
  case Legend::BOTTOM:
  case Legend::RIGHT:
  case Legend::LEFT:
    legend_->print(ps);
    break;
  default:
    break;
  }

  printAxes(ps);
  printAxesGrids(ps);
  printAxesLimits(ps);

  if (!legend_->isRaised()) {
    switch (legend_->position()) {
    case Legend::PLOT:
    case Legend::XY:
      legend_->print(ps);
      break;
    default:
      break;
    }
  }

  printMarkers(ps, MARKER_UNDER);
  printElements(ps);
  printActiveElements(ps);

  if (legend_->isRaised()) {
    switch (legend_->position()) {
    case Legend::PLOT:
    case Legend::XY:
      legend_->print(ps);
      break;
    default:
      break;
    }
  }
  printMarkers(ps, MARKER_ABOVE);
  // End

  Blt_Ps_VarAppend(ps, "\n", "% Unset clipping\n", "grestore\n\n", NULL);
  Blt_Ps_VarAppend(ps, "showpage\n", "%Trailer\n", "grestore\n", "end\n", "%EOF\n", NULL);

 error:
  width_ = Tk_Width(tkwin_);
  height_ = Tk_Height(tkwin_);
  flags |= MAP_WORLD;
  Blt_Ps_SetPrinting(ps, 0);
  reconfigure();
  map();

  // Redraw the graph in order to re-calculate the layout as soon as
  // possible. This is in the case the crosshairs are active.
  eventuallyRedraw();

  return result;
}

void Graph::eventuallyRedraw() 
{
  if ((flags & GRAPH_DELETED) || !Tk_IsMapped(tkwin_))
    return;

  if (!(flags & REDRAW_PENDING)) {
    flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(DisplayGraph, this);
  }
}

void Graph::extents(Region2d* regionPtr)
{
  GraphOptions* ops = (GraphOptions*)ops_;

  regionPtr->left = (double)(hOffset_ - ops->xPad);
  regionPtr->top = (double)(vOffset_ - ops->yPad);
  regionPtr->right = (double)(hOffset_ + hRange_ + ops->xPad);
  regionPtr->bottom = (double)(vOffset_ + vRange_ + ops->yPad);
}

void Graph::reconfigure()	
{
  configure();
  legend_->configure();
  configureElements();
  configureAxes();
  configureMarkers();
}

// Margins

void Graph::drawMargins(Drawable drawable)
{
  GraphOptions* ops = (GraphOptions*)ops_;
  XRectangle rects[4];

  // Draw the four outer rectangles which encompass the plotting
  // surface. This clears the surrounding area and clips the plot.
  rects[0].x = rects[0].y = rects[3].x = rects[1].x = 0;
  rects[0].width = rects[3].width = (short int)width_;
  rects[0].height = (short int)top_;
  rects[3].y = bottom_;
  rects[3].height = height_ - bottom_;
  rects[2].y = rects[1].y = top_;
  rects[1].width = left_;
  rects[2].height = rects[1].height = bottom_ - top_;
  rects[2].x = right_;
  rects[2].width = width_ - right_;

  Tk_Fill3DRectangle(tkwin_, drawable, ops->normalBg, 
		     rects[0].x, rects[0].y, rects[0].width, rects[0].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(tkwin_, drawable, ops->normalBg, 
		     rects[1].x, rects[1].y, rects[1].width, rects[1].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(tkwin_, drawable, ops->normalBg, 
		     rects[2].x, rects[2].y, rects[2].width, rects[2].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(tkwin_, drawable, ops->normalBg, 
		     rects[3].x, rects[3].y, rects[3].width, rects[3].height, 
		     0, TK_RELIEF_FLAT);

  // Draw 3D border around the plotting area
  if (ops->plotBW > 0) {
    int x = left_ - ops->plotBW;
    int y = top_ - ops->plotBW;
    int w = (right_ - left_) + (2*ops->plotBW);
    int h = (bottom_ - top_) + (2*ops->plotBW);
    Tk_Draw3DRectangle(tkwin_, drawable, ops->normalBg, 
		       x, y, w, h, ops->plotBW, ops->plotRelief);
  }
  
  if (ops->title)
    Blt_DrawText(tkwin_, drawable, ops->title,
		 &ops->titleTextStyle, titleX_, titleY_);

  flags &= ~DRAW_MARGINS;
}

void Graph::printMargins(Blt_Ps ps)
{
  GraphOptions* gops = (GraphOptions*)ops_;
  PageSetup *setupPtr = pageSetup_;
  XRectangle margin[4];

  margin[0].x = margin[0].y = margin[3].x = margin[1].x = 0;
  margin[0].width = margin[3].width = width_;
  margin[0].height = top_;
  margin[3].y = bottom_;
  margin[3].height = height_ - bottom_;
  margin[2].y = margin[1].y = top_;
  margin[1].width = left_;
  margin[2].height = margin[1].height = bottom_ - top_;
  margin[2].x = right_;
  margin[2].width = width_ - right_;

  // Clear the surrounding margins and clip the plotting surface
  if (setupPtr->decorations)
    Blt_Ps_XSetBackground(ps, Tk_3DBorderColor(gops->normalBg));
  else
    Blt_Ps_SetClearBackground(ps);

  Blt_Ps_Append(ps, "% Margins\n");
  Blt_Ps_XFillRectangles(ps, margin, 4);
    
  Blt_Ps_Append(ps, "% Interior 3D border\n");
  if (gops->plotBW > 0) {
    int x = left_ - gops->plotBW;
    int y = top_ - gops->plotBW;
    int w = (right_ - left_) + (2*gops->plotBW);
    int h = (bottom_ - top_) + (2*gops->plotBW);
    Blt_Ps_Draw3DRectangle(ps, gops->normalBg, (double)x, (double)y, w, h,
			   gops->plotBW, gops->plotRelief);
  }

  if (gops->title) {
    Blt_Ps_Append(ps, "% Graph title\n");
    Blt_Ps_DrawText(ps, gops->title, &gops->titleTextStyle, 
		    (double)titleX_, (double)titleY_);
  }
}

void Graph::updateMarginTraces()
{
  GraphOptions* ops = (GraphOptions*)ops_;

  Margin* marginPtr;
  Margin* endPtr;
  for (marginPtr = ops->margins, endPtr = marginPtr + 4; 
       marginPtr < endPtr; marginPtr++) {
    if (marginPtr->varName) {
      int size;
      if ((marginPtr->site == MARGIN_LEFT) || (marginPtr->site == MARGIN_RIGHT))
	size = marginPtr->width;
      else
	size = marginPtr->height;

      Tcl_SetVar(interp_, marginPtr->varName, Blt_Itoa(size), 
		 TCL_GLOBAL_ONLY);
    }
  }
}

// Crosshairs

void Graph::enableCrosshairs()
{
  crosshairs_->enable();
}

void Graph::disableCrosshairs()
{
  crosshairs_->disable();
}

// Pens

int Graph::createPen(const char* penName, int objc, Tcl_Obj* const objv[])
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&penTable_, penName, &isNew);
  if (!isNew) {
    Tcl_AppendResult(interp_, "pen \"", penName, "\" already exists in \"",
		     Tk_PathName(tkwin_), "\"", (char *)NULL);
    return TCL_ERROR;
  }

  Pen* penPtr;
  switch (classId_) {
  case CID_ELEM_BAR:
    penPtr = new BarPen(this, penName, hPtr);
    break;
  case CID_ELEM_LINE:
    penPtr = new LinePen(this, penName, hPtr);
    break;
  default:
    return TCL_ERROR;
  }
  if (!penPtr)
    return TCL_ERROR;

  Tcl_SetHashValue(hPtr, penPtr);

  if ((Tk_InitOptions(interp_, (char*)penPtr->ops(), penPtr->optionTable(), tkwin_) != TCL_OK) || (PenObjConfigure(interp_, this, penPtr, objc-4, objv+4) != TCL_OK)) {
    delete penPtr;
    return TCL_ERROR;
  }

  flags |= CACHE_DIRTY;
  eventuallyRedraw();

  return TCL_OK;
}

void Graph::destroyPens()
{
  Tcl_HashSearch iter;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&penTable_, &iter);
       hPtr; hPtr = Tcl_NextHashEntry(&iter)) {
    Pen* penPtr = (Pen*)Tcl_GetHashValue(hPtr);
    delete penPtr;
  }
  Tcl_DeleteHashTable(&penTable_);
}

int Graph::getPen(Tcl_Obj* objPtr, Pen** penPtrPtr)
{
  *penPtrPtr = NULL;
  const char *name = Tcl_GetString(objPtr);
  if (!name || !name[0])
    return TCL_ERROR;

  Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&penTable_, name);
  if (!hPtr) {
    Tcl_AppendResult(interp_, "can't find pen \"", name, "\" in \"", 
		     Tk_PathName(tkwin_), "\"", NULL);
    return TCL_ERROR;
  }

  *penPtrPtr = (Pen*)Tcl_GetHashValue(hPtr);
  (*penPtrPtr)->refCount_++;

  return TCL_OK;
}

// Elements

int Graph::createElement(int objc, Tcl_Obj* const objv[])
{
  char *name = Tcl_GetString(objv[3]);
  if (name[0] == '-') {
    Tcl_AppendResult(interp_, "name of element \"", name, 
		     "\" can't start with a '-'", NULL);
    return TCL_ERROR;
  }

  int isNew;
  Tcl_HashEntry* hPtr = 
    Tcl_CreateHashEntry(&elements_.table, name, &isNew);
  if (!isNew) {
    Tcl_AppendResult(interp_, "element \"", name, 
		     "\" already exists in \"", Tcl_GetString(objv[0]), 
		     "\"", NULL);
    return TCL_ERROR;
  }

  Element* elemPtr;
  switch (classId_) {
  case CID_ELEM_BAR:
    elemPtr = new BarElement(this, name, hPtr);
    break;
  case CID_ELEM_LINE:
    elemPtr = new LineElement(this, name, hPtr);
    break;
  default:
    return TCL_ERROR;
  }
  if (!elemPtr)
    return TCL_ERROR;

  Tcl_SetHashValue(hPtr, elemPtr);

  if ((Tk_InitOptions(interp_, (char*)elemPtr->ops(), elemPtr->optionTable(), tkwin_) != TCL_OK) || (ElementObjConfigure(interp_, elemPtr, objc-4, objv+4) != TCL_OK)) {
    Blt_DestroyElement(elemPtr);
    return TCL_ERROR;
  }

  elemPtr->link = Blt_Chain_Append(elements_.displayList, elemPtr);

  return TCL_OK;
}

void Graph::destroyElements()
{
  Tcl_HashSearch iter;
  for (Tcl_HashEntry* hPtr=Tcl_FirstHashEntry(&elements_.table, &iter);
       hPtr; hPtr = Tcl_NextHashEntry(&iter)) {
    Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
    Blt_DestroyElement(elemPtr);
  }
  Tcl_DeleteHashTable(&elements_.table);
  Tcl_DeleteHashTable(&elements_.tagTable);
  Blt_Chain_Destroy(elements_.displayList);
}

void Graph::configureElements()
{
  for (Blt_ChainLink link=Blt_Chain_FirstLink(elements_.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->configure();
  }
}

void Graph::mapElements()
{
  GraphOptions* gops = (GraphOptions*)ops_;
  if (gops->barMode != BARS_INFRONT)
    Blt_ResetBarGroups(this);

  for (Blt_ChainLink link =Blt_Chain_FirstLink(elements_.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);

    if ((flags & MAP_ALL) || (elemPtr->flags & MAP_ITEM)) {
      elemPtr->map();
      elemPtr->flags &= ~MAP_ITEM;
    }
  }
}

void Graph::drawElements(Drawable drawable)
{
  // Draw with respect to the stacking order
  for (Blt_ChainLink link=Blt_Chain_LastLink(elements_.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->draw(drawable);
  }
}

void Graph::drawActiveElements(Drawable drawable)
{
  for (Blt_ChainLink link=Blt_Chain_LastLink(elements_.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->drawActive(drawable);
  }
}

void Graph::printElements(Blt_Ps ps)
{
  for (Blt_ChainLink link=Blt_Chain_LastLink(elements_.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->print(ps);
  }
}

void Graph::printActiveElements(Blt_Ps ps)
{
  for (Blt_ChainLink link=Blt_Chain_LastLink(elements_.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->printActive(ps);
  }
}

int Graph::getElement(Tcl_Obj *objPtr, Element **elemPtrPtr)
{
  *elemPtrPtr =NULL;
  const char* name = Tcl_GetString(objPtr);
  if (!name || !name[0])
    return TCL_ERROR;

  Tcl_HashEntry*hPtr = Tcl_FindHashEntry(&elements_.table, name);
  if (!hPtr) {
    Tcl_AppendResult(interp_, "can't find element \"", name, "\" in \"",
		     Tk_PathName(tkwin_), "\"", NULL);
    return TCL_ERROR;
  }

  *elemPtrPtr = (Element*)Tcl_GetHashValue(hPtr);
  return TCL_OK;
}

ClientData Graph::elementTag(const char *tagName)
{
  int isNew;
  Tcl_HashEntry* hPtr = Tcl_CreateHashEntry(&elements_.tagTable,tagName,&isNew);
  return Tcl_GetHashKey(&elements_.tagTable, hPtr);
}

// Markers

void Graph::destroyMarkers()
{
  Tcl_HashSearch iter;
  for (Tcl_HashEntry* hPtr=Tcl_FirstHashEntry(&markers_.table, &iter); 
       hPtr; hPtr=Tcl_NextHashEntry(&iter)) {
    Marker* markerPtr = (Marker*)Tcl_GetHashValue(hPtr);
    delete markerPtr;
  }
  Tcl_DeleteHashTable(&markers_.table);
  Tcl_DeleteHashTable(&markers_.tagTable);
  Blt_Chain_Destroy(markers_.displayList);
}


void Graph::configureMarkers()
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(markers_.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    markerPtr->configure();
  }
}

void Graph::mapMarkers()
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(markers_.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* mops = (MarkerOptions*)markerPtr->ops();

    if ((markerPtr->flags & DELETE_PENDING) || mops->hide)
      continue;

    if ((flags & MAP_ALL) || (markerPtr->flags & MAP_ITEM)) {
      markerPtr->map();
      markerPtr->flags &= ~MAP_ITEM;
    }
  }
}

void Graph::drawMarkers(Drawable drawable, int under)
{
  for (Blt_ChainLink link = Blt_Chain_LastLink(markers_.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* mops = (MarkerOptions*)markerPtr->ops();

    if ((mops->drawUnder != under) || (markerPtr->clipped_) ||
	(markerPtr->flags & DELETE_PENDING) || (mops->hide))
      continue;

    if (isElementHidden(markerPtr))
      continue;

    markerPtr->draw(drawable);
  }
}

void Graph::printMarkers(Blt_Ps ps, int under)
{
  for (Blt_ChainLink link = Blt_Chain_LastLink(markers_.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* mops = (MarkerOptions*)markerPtr->ops();
    if (mops->drawUnder != under)
      continue;

    if ((markerPtr->flags & DELETE_PENDING) || mops->hide)
      continue;

    if (isElementHidden(markerPtr))
      continue;

    Blt_Ps_VarAppend(ps, "\n% Marker \"", markerPtr->name_, 
		     "\" is a ", markerPtr->className(), ".\n", (char*)NULL);
    markerPtr->postscript(ps);
  }
}

ClientData Graph::markerTag(const char* tagName)
{
  int isNew;
  Tcl_HashEntry* hPtr = Tcl_CreateHashEntry(&markers_.tagTable, tagName,&isNew);
  return Tcl_GetHashKey(&markers_.tagTable, hPtr);
}

Marker* Graph::nearestMarker(int x, int y, int under)
{
  Point2d point;
  point.x = (double)x;
  point.y = (double)y;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(markers_.displayList);
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* mops = (MarkerOptions*)markerPtr->ops();

    if ((markerPtr->flags & (DELETE_PENDING|MAP_ITEM)) || (mops->hide))
      continue;

    if (isElementHidden(markerPtr))
      continue;

    if ((mops->drawUnder == under) && (mops->state == BLT_STATE_NORMAL))
      if (markerPtr->pointIn(&point))
	return markerPtr;
  }
  return NULL;
}

int Graph::isElementHidden(Marker* markerPtr)
{
  MarkerOptions* mops = (MarkerOptions*)markerPtr->ops();

  if (mops->elemName) {
    Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&elements_.table, mops->elemName);
    if (hPtr) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      if (!elemPtr->link || elemPtr->hide_)
	return 1;
    }
  }
  return 0;
}

// Axis

int Graph::createAxes()
{
  for (int ii=0; ii<4; ii++) {
    int isNew;
    Tcl_HashEntry* hPtr = 
      Tcl_CreateHashEntry(&axes_.table, axisNames[ii].name, &isNew);
    Blt_Chain chain = Blt_Chain_Create();

    Axis* axisPtr = new Axis(this, axisNames[ii].name, ii, hPtr);
    if (!axisPtr)
      return TCL_ERROR;
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();

    Tcl_SetHashValue(hPtr, axisPtr);

    axisPtr->refCount_ = 1;
    axisPtr->use_ =1;
    
    axisPtr->setClass(!(ii&1) ? CID_AXIS_X : CID_AXIS_Y);

    if (Tk_InitOptions(interp_, (char*)axisPtr->ops(), 
		       axisPtr->optionTable(), tkwin_) != TCL_OK)
      return TCL_ERROR;

    if (axisPtr->configure() != TCL_OK)
      return TCL_ERROR;

    if ((axisPtr->margin_ == MARGIN_RIGHT) || (axisPtr->margin_ == MARGIN_TOP))
      ops->hide = 1;

    axisChain_[ii] = chain;
    axisPtr->link = Blt_Chain_Append(chain, axisPtr);
    axisPtr->chain = chain;
  }
  return TCL_OK;
}

int Graph::createAxis(int objc, Tcl_Obj* const objv[])
{
  char *string = Tcl_GetString(objv[3]);
  if (string[0] == '-') {
    Tcl_AppendResult(interp_, "name of axis \"", string, 
		     "\" can't start with a '-'", NULL);
    return TCL_ERROR;
  }

  int isNew;
  Tcl_HashEntry* hPtr = Tcl_CreateHashEntry(&axes_.table, string, &isNew);
  if (!isNew) {
    Tcl_AppendResult(interp_, "axis \"", string, "\" already exists in \"",
		     Tcl_GetString(objv[0]), "\"", NULL);
    return TCL_ERROR;
  }

  Axis* axisPtr = new Axis(this, Tcl_GetString(objv[3]), MARGIN_NONE, hPtr);
  if (!axisPtr)
    return TCL_ERROR;

  Tcl_SetHashValue(hPtr, axisPtr);

  if ((Tk_InitOptions(interp_, (char*)axisPtr->ops(), axisPtr->optionTable(), tkwin_) != TCL_OK) || (AxisObjConfigure(interp_, axisPtr, objc-4, objv+4) != TCL_OK)) {
    delete axisPtr;
    return TCL_ERROR;
  }

  return TCL_OK;
}

void Graph::destroyAxes()
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr=Tcl_FirstHashEntry(&axes_.table, &cursor);
       hPtr; hPtr=Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    delete axisPtr;
  }
  Tcl_DeleteHashTable(&axes_.table);

  for (int ii=0; ii<4; ii++)
    Blt_Chain_Destroy(axisChain_[ii]);

  Tcl_DeleteHashTable(&axes_.tagTable);
  Blt_Chain_Destroy(axes_.displayList);
}

void Graph::configureAxes()
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr=Tcl_FirstHashEntry(&axes_.table, &cursor);
       hPtr; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->configure();
  }
}

void Graph::mapAxes()
{
  GraphOptions* gops = (GraphOptions*)ops_;

  for (int ii=0; ii<4; ii++) {
    int count =0;
    int offset =0;
    Blt_Chain chain = gops->margins[ii].axes;
    for (Blt_ChainLink link=Blt_Chain_FirstLink(chain); link; 
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!axisPtr->use_ || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (ops->reqNumMajorTicks <= 0)
	ops->reqNumMajorTicks = 4;

      if (gops->stackAxes)
	axisPtr->mapStacked(count, ii);
      else 
	axisPtr->map(offset, ii);

      if (ops->showGrid)
	axisPtr->mapGridlines();

      offset += axisPtr->isHorizontal() ? axisPtr->height_ : axisPtr->width_;
      count++;
    }
  }
}

void Graph::drawAxes(Drawable drawable)
{
  GraphOptions* gops = (GraphOptions*)ops_;

  for (int ii=0; ii<4; ii++) {
    for (Blt_ChainLink link = Blt_Chain_LastLink(gops->margins[ii].axes); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      axisPtr->draw(drawable);
    }
  }
}

void Graph::drawAxesLimits(Drawable drawable)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry* hPtr=Tcl_FirstHashEntry(&axes_.table, &cursor);
       hPtr; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->drawLimits(drawable);
  }
}

void Graph::drawAxesGrids(Drawable drawable)
{
  GraphOptions* gops = (GraphOptions*)ops_;

  for (int ii=0; ii<4; ii++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(gops->margins[ii].axes);
	 link; link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      axisPtr->drawGrids(drawable);
    }
  }
}

void Graph::printAxes(Blt_Ps ps) 
{
  GraphOptions* gops = (GraphOptions*)ops_;

  Margin *mp, *mend;
  for (mp = gops->margins, mend = mp + 4; mp < mend; mp++) {
    for (Blt_ChainLink link=Blt_Chain_FirstLink(mp->axes); link; 
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      axisPtr->print(ps);
    }
  }
}

void Graph::printAxesGrids(Blt_Ps ps) 
{
  GraphOptions* gops = (GraphOptions*)ops_;

  for (int ii=0; ii<4; ii++) {
    for (Blt_ChainLink link=Blt_Chain_FirstLink(gops->margins[ii].axes);
	 link; link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      axisPtr->printGrids(ps);
    }
  }
}

void Graph::printAxesLimits(Blt_Ps ps)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry* hPtr=Tcl_FirstHashEntry(&axes_.table, &cursor);
       hPtr; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->printLimits(ps);
  }
}

int Graph::getAxis(Tcl_Obj *objPtr, Axis **axisPtrPtr)
{
  *axisPtrPtr = NULL;
  const char* name = Tcl_GetString(objPtr);
  if (!name || !name[0])
    return TCL_ERROR;

  Tcl_HashEntry* hPtr = Tcl_FindHashEntry(&axes_.table, name);
  if (!hPtr) {
    Tcl_AppendResult(interp_, "can't find axis \"", name, "\" in \"", 
		     Tk_PathName(tkwin_), "\"", NULL);
    return TCL_ERROR;
  }

  *axisPtrPtr = (Axis*)Tcl_GetHashValue(hPtr);
  return TCL_OK;
}

ClientData Graph::axisTag(const char *tagName)
{
  int isNew;
  Tcl_HashEntry *hPtr = Tcl_CreateHashEntry(&axes_.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&axes_.tagTable, hPtr);
}

void Graph::adjustAxes() 
{
  GraphOptions* ops = (GraphOptions*)ops_;

  if (ops->inverted) {
    ops->leftMargin.axes   = axisChain_[0];
    ops->bottomMargin.axes = axisChain_[1];
    ops->rightMargin.axes  = axisChain_[2];
    ops->topMargin.axes    = axisChain_[3];
  }
  else {
    ops->leftMargin.axes   = axisChain_[1];
    ops->bottomMargin.axes = axisChain_[0];
    ops->rightMargin.axes  = axisChain_[3];
    ops->topMargin.axes    = axisChain_[2];
  }
}

Point2d Graph::map2D(double x, double y, Axis2d* axesPtr)
{
  GraphOptions* ops = (GraphOptions*)ops_;

  Point2d point;
  if (ops->inverted) {
    point.x = axesPtr->y->hMap(y);
    point.y = axesPtr->x->vMap(x);
  }
  else {
    point.x = axesPtr->x->hMap(x);
    point.y = axesPtr->y->vMap(y);
  }
  return point;
}

Point2d Graph::invMap2D(double x, double y, Axis2d* axesPtr)
{
  GraphOptions* ops = (GraphOptions*)ops_;

  Point2d point;
  if (ops->inverted) {
    point.x = axesPtr->x->invVMap(y);
    point.y = axesPtr->y->invHMap(x);
  }
  else {
    point.x = axesPtr->x->invHMap(x);
    point.y = axesPtr->y->invVMap(y);
  }
  return point;
}

void Graph::resetAxes()
{
  GraphOptions* gops = (GraphOptions*)ops_;
  
  /* FIXME: This should be called whenever the display list of
   *	      elements change. Maybe yet another flag INIT_STACKS to
   *	      indicate that the element display list has changed.
   *	      Needs to be done before the axis limits are set.
   */
  Blt_InitBarSetTable(this);
  if ((gops->barMode == BARS_STACKED) && (nBarGroups_ > 0))
    Blt_ComputeBarStacks(this);

  /*
   * Step 1:  Reset all axes. Initialize the data limits of the axis to
   *		impossible values.
   */
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry* hPtr = Tcl_FirstHashEntry(&axes_.table, &cursor);
       hPtr; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->min_ = axisPtr->valueRange_.min = DBL_MAX;
    axisPtr->max_ = axisPtr->valueRange_.max = -DBL_MAX;
  }

  /*
   * Step 2:  For each element that's to be displayed, get the smallest
   *		and largest data values mapped to each X and Y-axis.  This
   *		will be the axis limits if the user doesn't override them 
   *		with -min and -max options.
   */
  for (Blt_ChainLink link = Blt_Chain_FirstLink(elements_.displayList);
       link; link = Blt_Chain_NextLink(link)) {
    Region2d exts;

    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemops = (ElementOptions*)elemPtr->ops();
    elemPtr->extents(&exts);
    elemops->axes.x->getDataLimits(exts.left, exts.right);
    elemops->axes.y->getDataLimits(exts.top, exts.bottom);
  }
  /*
   * Step 3:  Now that we know the range of data values for each axis,
   *		set axis limits and compute a sweep to generate tick values.
   */
  for (Tcl_HashEntry* hPtr = Tcl_FirstHashEntry(&axes_.table, &cursor);
       hPtr; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    axisPtr->fixRange();

    /* Calculate min/max tick (major/minor) layouts */
    double min = axisPtr->min_;
    double max = axisPtr->max_;
    if ((!isnan(axisPtr->scrollMin_)) && (min < axisPtr->scrollMin_))
      min = axisPtr->scrollMin_;

    if ((!isnan(axisPtr->scrollMax_)) && (max > axisPtr->scrollMax_))
      max = axisPtr->scrollMax_;

    if (ops->logScale)
      axisPtr->logScale(min, max);
    else
      axisPtr->linearScale(min, max);

    if (axisPtr->use_ && (axisPtr->flags & DIRTY))
      flags |= CACHE_DIRTY;
  }

  flags &= ~RESET_AXES;

  // When any axis changes, we need to layout the entire graph.
  flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | MAP_ALL | REDRAW_WORLD);
}

Axis* Graph::nearestAxis(int x, int y)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry* hPtr=Tcl_FirstHashEntry(&axes_.table, &cursor); 
       hPtr; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    if (ops->hide || !axisPtr->use_ || (axisPtr->flags & DELETE_PENDING))
      continue;

    if (ops->showTicks) {
      for (Blt_ChainLink link=Blt_Chain_FirstLink(axisPtr->tickLabels_);
	   link; link = Blt_Chain_NextLink(link)) {	
	TickLabel *labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
	double rw, rh;
	Point2d bbox[5];
	Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			   ops->tickAngle, &rw, &rh, bbox);
	Point2d t;
	t = Blt_AnchorPoint(labelPtr->anchorPos.x, labelPtr->anchorPos.y,
			    rw, rh, axisPtr->tickAnchor_);
	t.x = x - t.x - (rw * 0.5);
	t.y = y - t.y - (rh * 0.5);

	bbox[4] = bbox[0];
	if (Blt_PointInPolygon(&t, bbox, 5)) {
	  axisPtr->detail_ = "label";
	  return axisPtr;
	}
      }
    }

    if (ops->title) {
      unsigned int w, h;
      double rw, rh;
      Point2d bbox[5];
      Blt_GetTextExtents(ops->titleFont, 0, ops->title,-1,&w,&h);
      Blt_GetBoundingBox(w, h, axisPtr->titleAngle_, &rw, &rh, bbox);
      Point2d t;
      t = Blt_AnchorPoint(axisPtr->titlePos_.x, axisPtr->titlePos_.y, 
			  rw, rh, axisPtr->titleAnchor_);
      // Translate the point so that the 0,0 is the upper left 
      // corner of the bounding box
      t.x = x - t.x - (rw * 0.5);
      t.y = y - t.y - (rh * 0.5);
	    
      bbox[4] = bbox[0];
      if (Blt_PointInPolygon(&t, bbox, 5)) {
	axisPtr->detail_ = "title";
	return axisPtr;
      }
    }
    if (ops->lineWidth > 0) {
      if ((x <= axisPtr->right_) && (x >= axisPtr->left_) && 
	  (y <= axisPtr->bottom_) && (y >= axisPtr->top_)) {
	axisPtr->detail_ = "line";
	return axisPtr;
      }
    }
  }

  return NULL;
}
 
void Blt_GraphTags(Blt_BindTable table, ClientData object, ClientData context,
		   Blt_List list)
{
  Graph* graphPtr = (Graph*)Blt_GetBindingData(table);
  ClassId classId = (ClassId)(long(context));

  switch (classId) {
  case CID_ELEM_BAR:		
  case CID_ELEM_LINE: 
    {
      Element* ptr = (Element*)object;
      Blt_List_Append(list,(const char*)graphPtr->elementTag(ptr->name_),0);
      Blt_List_Append(list,(const char*)graphPtr->elementTag(ptr->className()),0);
      ElementOptions* ops = (ElementOptions*)ptr->ops();
      for (const char** pp = ops->tags; *pp; pp++)
	Blt_List_Append(list, (const char*)graphPtr->elementTag(*pp),0);
    }
    break;
  case CID_AXIS_X:
  case CID_AXIS_Y:
    {
      Axis* ptr = (Axis*)object;
      Blt_List_Append(list,(const char*)graphPtr->axisTag(ptr->name_),0);
      Blt_List_Append(list,(const char*)graphPtr->axisTag(ptr->className_),0);
      AxisOptions* ops = (AxisOptions*)ptr->ops();
      for (const char** pp = ops->tags; *pp; pp++)
	Blt_List_Append(list, (const char*)graphPtr->axisTag(*pp),0);
    }
    break;
  case CID_MARKER_BITMAP:
  case CID_MARKER_LINE:
  case CID_MARKER_POLYGON:
  case CID_MARKER_TEXT:
  case CID_MARKER_WINDOW:
    {
      Marker* ptr = (Marker*)object;
      Blt_List_Append(list,(const char*)graphPtr->markerTag(ptr->name_),0);
      Blt_List_Append(list,(const char*)graphPtr->markerTag(ptr->className()),0);
      MarkerOptions* ops = (MarkerOptions*)ptr->ops();
      for (const char** pp = ops->tags; *pp; pp++)
	Blt_List_Append(list, (const char*)graphPtr->markerTag(*pp),0);
    }
    break;
  default:
    break;
  }
}

// Find the closest point from the set of displayed elements, searching
// the display list from back to front.  That way, if the points from
// two different elements overlay each other exactly, the one that's on
// top (visible) is picked.
static ClientData PickEntry(ClientData clientData, int x, int y, 
			    ClientData* contextPtr)
{
  Graph* graphPtr = (Graph*)clientData;
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;

  if (graphPtr->flags & MAP_ALL) {
    *contextPtr = (ClientData)NULL;
    return NULL;
  }

  Region2d exts;
  graphPtr->extents(&exts);

  // Sample coordinate is in one of the graph margins. Can only pick an axis.
  if ((x >= exts.right) || (x < exts.left) || 
      (y >= exts.bottom) || (y < exts.top)) {
    Axis* axisPtr = graphPtr->nearestAxis(x, y);
    if (axisPtr) {
      *contextPtr = (ClientData)axisPtr->classId_;
      return axisPtr;
    }
  }

  // From top-to-bottom check:
  // 1. markers drawn on top (-under false).
  // 2. elements using its display list back to front.
  // 3. markers drawn under element (-under true).
  Marker* markerPtr = graphPtr->nearestMarker(x, y, 0);
  if (markerPtr) {
    *contextPtr = (ClientData)markerPtr->classId();
    return markerPtr;
  }

  ClosestSearch* searchPtr = &ops->search;
  searchPtr->index = -1;
  searchPtr->x = x;
  searchPtr->y = y;
  searchPtr->dist = (double)(searchPtr->halo + 1);
	
  Blt_ChainLink link;
  Element* elemPtr;
  for (link = Blt_Chain_LastLink(graphPtr->elements_.displayList);
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->hide_ || (elemPtr->flags & MAP_ITEM))
      continue;

    elemPtr->closest();
  }
  // Found an element within the minimum halo distance.
  if (searchPtr->dist <= (double)searchPtr->halo) {
    *contextPtr = (ClientData)elemPtr->classId();
    return searchPtr->elemPtr;
  }

  markerPtr = graphPtr->nearestMarker(x, y, 1);
  if (markerPtr) {
    *contextPtr = (ClientData)markerPtr->classId();
    return markerPtr;
  }

  *contextPtr = (ClientData)NULL;
  return NULL;
}

