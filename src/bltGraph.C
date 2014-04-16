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
#include "bltGrPenOp.h"
#include "bltGrElem.h"
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

static Blt_BindPickProc PickEntry;

static void AdjustAxisPointers(Graph* graphPtr);
static void DrawPlot(Graph* graphPtr, Drawable drawable);
static void UpdateMarginTraces(Graph* graphPtr);

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

  barGroups =NULL;
  nBarGroups =0;
  maxBarSetSize =0;

  ops->bottomMargin.site = MARGIN_BOTTOM;
  ops->leftMargin.site = MARGIN_LEFT;
  ops->topMargin.site = MARGIN_TOP;
  ops->rightMargin.site = MARGIN_RIGHT;

  Blt_Ts_InitStyle(ops->titleTextStyle);
  ops->titleTextStyle.anchor = TK_ANCHOR_N;

  Tcl_InitHashTable(&axes.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&axes.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&elements.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&elements.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&markers.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&markers.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&dataTables, TCL_STRING_KEYS);
  Tcl_InitHashTable(&penTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&setTable, sizeof(BarSetKey) / sizeof(int));
  elements.displayList = Blt_Chain_Create();
  markers.displayList = Blt_Chain_Create();
  axes.displayList = Blt_Chain_Create();
  bindTable = Blt_CreateBindingTable(interp_, tkwin_, this, 
				     PickEntry, Blt_GraphTags);

  if (Blt_CreatePen(this, interp_, "activeLine", CID_ELEM_LINE, 0, NULL) != 
      TCL_OK) {
    valid_ =0;
    return;
  }
  if (Blt_CreatePen(this, interp_, "activeBar", CID_ELEM_BAR, 0, NULL) != 
      TCL_OK) {
    valid_ =0;
    return;
  }
  if (Blt_CreateAxes(this) != TCL_OK) {
    valid_ =0;
    return;
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

  AdjustAxisPointers(this);

  Tcl_SetStringObj(Tcl_GetObjResult(interp_), Tk_PathName(tkwin_), -1);
}

Graph::~Graph()
{
  GraphOptions* ops = (GraphOptions*)ops_;

  Blt::DestroyMarkers(this);
  Blt_DestroyElements(this);  // must come before legend and others

  if (crosshairs_)
    delete crosshairs_;
  if (legend_)
    delete legend_;

  Blt_DestroyAxes(this);
  Blt_DestroyPens(this);
  Blt_DestroyPageSetup(this);
  Blt_DestroyBarSets(this);

  if (bindTable)
    Blt_DestroyBindingTable(bindTable);

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
    AdjustAxisPointers(this);

  // Free the pixmap if we're not buffering the display of elements anymore.
  if ((!ops->backingStore) && (cache_ != None)) {
    Tk_FreePixmap(display_, cache_);
    cache_ = None;
  }
}

void GraphDisplay(Graph* graphPtr)
{
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
   Tk_Window tkwin = graphPtr->tkwin_;

  graphPtr->flags &= ~REDRAW_PENDING;
  if ((graphPtr->flags & GRAPH_DELETED) || !Tk_IsMapped(graphPtr->tkwin_))
    return;

  if ((Tk_Width(graphPtr->tkwin_) <= 1) || (Tk_Height(graphPtr->tkwin_) <= 1)) {
    /* Don't bother computing the layout until the size of the window is
     * something reasonable. */
    return;
  }
  graphPtr->width_ = Tk_Width(graphPtr->tkwin_);
  graphPtr->height_ = Tk_Height(graphPtr->tkwin_);
  Blt_MapGraph(graphPtr);
  if (!Tk_IsMapped(graphPtr->tkwin_)) {
    /* The graph's window isn't displayed, so don't bother drawing
     * anything.  By getting this far, we've at least computed the
     * coordinates of the graph's new layout.  */
    return;
  }
  /* Create a pixmap the size of the window for double buffering. */
  Pixmap drawable;
  if (ops->doubleBuffer)
    drawable = Tk_GetPixmap(graphPtr->display_, Tk_WindowId(graphPtr->tkwin_), 
			    graphPtr->width_, graphPtr->height_, Tk_Depth(graphPtr->tkwin_));
  else
    drawable = Tk_WindowId(graphPtr->tkwin_);

  if (ops->backingStore) {
    if ((graphPtr->cache_ == None) || 
	(graphPtr->cacheWidth_ != graphPtr->width_) ||
	(graphPtr->cacheHeight_ != graphPtr->height_)) {
      if (graphPtr->cache_ != None)
	Tk_FreePixmap(graphPtr->display_, graphPtr->cache_);

      graphPtr->cache_ = Tk_GetPixmap(graphPtr->display_, 
				     Tk_WindowId(graphPtr->tkwin_), 
				     graphPtr->width_, graphPtr->height_, 
				     Tk_Depth(graphPtr->tkwin_));
      graphPtr->cacheWidth_  = graphPtr->width_;
      graphPtr->cacheHeight_ = graphPtr->height_;
      graphPtr->flags |= CACHE_DIRTY;
    }
  }
  if (ops->backingStore) {
    if (graphPtr->flags & CACHE_DIRTY) {
      DrawPlot(graphPtr, graphPtr->cache_);
      graphPtr->flags &= ~CACHE_DIRTY;
    }

    // Copy the pixmap to the one used for drawing the entire graph
    XCopyArea(graphPtr->display_, graphPtr->cache_, drawable,
	      graphPtr->drawGC_, 0, 0, Tk_Width(graphPtr->tkwin_),
	      Tk_Height(graphPtr->tkwin_), 0, 0);
  }
  else
    DrawPlot(graphPtr, drawable);

  // Draw markers above elements
  Blt::DrawMarkers(graphPtr, drawable, MARKER_ABOVE);
  Blt_DrawActiveElements(graphPtr, drawable);

  // Don't draw legend in the plot area.
  if (graphPtr->legend_->isRaised()) {
    switch (graphPtr->legend_->position()) {
    case Legend::PLOT:
    case Legend::XY:
      graphPtr->legend_->draw(drawable);
      break;
    default:
      break;
    }
  }

  // Draw 3D border just inside of the focus highlight ring
  if ((ops->borderWidth > 0) && (ops->relief != TK_RELIEF_FLAT))
    Tk_Draw3DRectangle(graphPtr->tkwin_, drawable, ops->normalBg, 
		       ops->highlightWidth, ops->highlightWidth, 
		       graphPtr->width_ - 2*ops->highlightWidth, 
		       graphPtr->height_ - 2*ops->highlightWidth, 
		       ops->borderWidth, ops->relief);

  // Draw focus highlight ring
  if ((ops->highlightWidth > 0) && (graphPtr->flags & FOCUS)) {
    GC gc = Tk_GCForColor(ops->highlightColor, drawable);
    Tk_DrawFocusHighlight(graphPtr->tkwin_, gc, ops->highlightWidth,
			  drawable);
  }

  // Disable crosshairs before redisplaying to the screen
  Blt_DisableCrosshairs(graphPtr);
  XCopyArea(graphPtr->display_, drawable, Tk_WindowId(tkwin),
	    graphPtr->drawGC_, 0, 0, graphPtr->width_, graphPtr->height_, 0, 0);
  Blt_EnableCrosshairs(graphPtr);
  if (ops->doubleBuffer)
    Tk_FreePixmap(graphPtr->display_, drawable);

  graphPtr->flags &= ~MAP_WORLD;
  graphPtr->flags &= ~REDRAW_WORLD;
  UpdateMarginTraces(graphPtr);
}

// Support

void Blt_EventuallyRedrawGraph(Graph* graphPtr) 
{
  if ((graphPtr->flags & GRAPH_DELETED) || !Tk_IsMapped(graphPtr->tkwin_))
    return;

  if (!(graphPtr->flags & REDRAW_PENDING)) {
    graphPtr->flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(DisplayGraph, graphPtr);
  }
}

static void AdjustAxisPointers(Graph* graphPtr) 
{
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
  if (ops->inverted) {
    ops->leftMargin.axes   = graphPtr->axisChain[0];
    ops->bottomMargin.axes = graphPtr->axisChain[1];
    ops->rightMargin.axes  = graphPtr->axisChain[2];
    ops->topMargin.axes    = graphPtr->axisChain[3];
  }
  else {
    ops->leftMargin.axes   = graphPtr->axisChain[1];
    ops->bottomMargin.axes = graphPtr->axisChain[0];
    ops->rightMargin.axes  = graphPtr->axisChain[3];
    ops->topMargin.axes    = graphPtr->axisChain[2];
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawMargins --
 *
 * 	Draws the exterior region of the graph (axes, ticks, titles, etc) 
 * 	onto a pixmap. The interior region is defined by the given rectangle
 * 	structure.
 *
 *	---------------------------------
 *      |                               |
 *      |           rectArr[0]          |
 *      |                               |
 *	---------------------------------
 *      |     |top           right|     |
 *      |     |                   |     |
 *      |     |                   |     |
 *      | [1] |                   | [2] |
 *      |     |                   |     |
 *      |     |                   |     |
 *      |     |                   |     |
 *      |     |                   |     |
 *      |     |                   |     |
 *      |     |left         bottom|     |
 *	---------------------------------
 *      |                               |
 *      |          rectArr[3]           |
 *      |                               |
 *	---------------------------------
 *
 *		X coordinate axis
 *		Y coordinate axis
 *		legend
 *		interior border
 *		exterior border
 *		titles (X and Y axis, graph)
 *
 * Returns:
 *	None.
 *
 * Side Effects:
 *	Exterior of graph is displayed in its window.
 *
 *---------------------------------------------------------------------------
 */
static void DrawMargins(Graph* graphPtr, Drawable drawable)
{
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
  XRectangle rects[4];

  /*
   * Draw the four outer rectangles which encompass the plotting
   * surface. This clears the surrounding area and clips the plot.
   */
  rects[0].x = rects[0].y = rects[3].x = rects[1].x = 0;
  rects[0].width = rects[3].width = (short int)graphPtr->width_;
  rects[0].height = (short int)graphPtr->top_;
  rects[3].y = graphPtr->bottom_;
  rects[3].height = graphPtr->height_ - graphPtr->bottom_;
  rects[2].y = rects[1].y = graphPtr->top_;
  rects[1].width = graphPtr->left_;
  rects[2].height = rects[1].height = graphPtr->bottom_ - graphPtr->top_;
  rects[2].x = graphPtr->right_;
  rects[2].width = graphPtr->width_ - graphPtr->right_;

  Tk_Fill3DRectangle(graphPtr->tkwin_, drawable, ops->normalBg, 
		     rects[0].x, rects[0].y, rects[0].width, rects[0].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(graphPtr->tkwin_, drawable, ops->normalBg, 
		     rects[1].x, rects[1].y, rects[1].width, rects[1].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(graphPtr->tkwin_, drawable, ops->normalBg, 
		     rects[2].x, rects[2].y, rects[2].width, rects[2].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(graphPtr->tkwin_, drawable, ops->normalBg, 
		     rects[3].x, rects[3].y, rects[3].width, rects[3].height, 
		     0, TK_RELIEF_FLAT);

  /* Draw 3D border around the plotting area */

  if (ops->plotBW > 0) {
    int x, y, w, h;

    x = graphPtr->left_ - ops->plotBW;
    y = graphPtr->top_ - ops->plotBW;
    w = (graphPtr->right_ - graphPtr->left_) + (2*ops->plotBW);
    h = (graphPtr->bottom_ - graphPtr->top_) + (2*ops->plotBW);
    Tk_Draw3DRectangle(graphPtr->tkwin_, drawable, ops->normalBg, 
		       x, y, w, h, ops->plotBW, ops->plotRelief);
  }
  
  switch (graphPtr->legend_->position()) {
  case Legend::TOP:
  case Legend::BOTTOM:
  case Legend::RIGHT:
  case Legend::LEFT:
    graphPtr->legend_->draw(drawable);
    break;
  default:
    break;
  }

  if (ops->title != NULL)
    Blt_DrawText(graphPtr->tkwin_, drawable, ops->title,
		 &ops->titleTextStyle, graphPtr->titleX_, graphPtr->titleY_);

  Blt_DrawAxes(graphPtr, drawable);
  graphPtr->flags &= ~DRAW_MARGINS;
}

static void DrawPlot(Graph* graphPtr, Drawable drawable)
{
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
  DrawMargins(graphPtr, drawable);

  // Draw the background of the plotting area with 3D border
  Tk_Fill3DRectangle(graphPtr->tkwin_, drawable, ops->plotBg,
		     graphPtr->left_ - ops->plotBW, 
		     graphPtr->top_ - ops->plotBW, 
		     graphPtr->right_ - graphPtr->left_ + 1 + 2*ops->plotBW,
		     graphPtr->bottom_ - graphPtr->top_  + 1 + 2*ops->plotBW, 
		     ops->plotBW, ops->plotRelief);
  
  // Draw the elements, markers, legend, and axis limits
  Blt_DrawAxes(graphPtr, drawable);
  Blt_DrawGrids(graphPtr, drawable);
  Blt::DrawMarkers(graphPtr, drawable, MARKER_UNDER);

  if (!graphPtr->legend_->isRaised()) {
    switch (graphPtr->legend_->position()) {
    case Legend::PLOT:
    case Legend::XY:
      graphPtr->legend_->draw(drawable);
      break;
    default:
      break;
    }
  }

  Blt_DrawAxisLimits(graphPtr, drawable);
  Blt_DrawElements(graphPtr, drawable);
}

void Blt_MapGraph(Graph* graphPtr)
{
  if (graphPtr->flags & RESET_AXES) {
    Blt_ResetAxes(graphPtr);
  }
  if (graphPtr->flags & LAYOUT_NEEDED) {
    Blt_LayoutGraph(graphPtr);
    graphPtr->flags &= ~LAYOUT_NEEDED;
  }

  if ((graphPtr->vRange_ > 1) && (graphPtr->hRange_ > 1)) {
    if (graphPtr->flags & MAP_WORLD)
      Blt_MapAxes(graphPtr);

    Blt_MapElements(graphPtr);
    Blt::MapMarkers(graphPtr);
    graphPtr->flags &= ~(MAP_ALL);
  }
}

static void UpdateMarginTraces(Graph* graphPtr)
{
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
  Margin* marginPtr;
  Margin* endPtr;

  for (marginPtr = ops->margins, endPtr = marginPtr + 4; 
       marginPtr < endPtr; marginPtr++) {
    if (marginPtr->varName != NULL) { /* Trigger variable traces */
      int size;

      if ((marginPtr->site == MARGIN_LEFT) || 
	  (marginPtr->site == MARGIN_RIGHT)) {
	size = marginPtr->width;
      } else {
	size = marginPtr->height;
      }
      Tcl_SetVar(graphPtr->interp_, marginPtr->varName, Blt_Itoa(size), 
		 TCL_GLOBAL_ONLY);
    }
  }
}

/*
 *---------------------------------------------------------------------------
 * Generates a bounding box representing the plotting area of the
 * graph. This data structure is used to clip the points and line
 * segments of the line element.
 * The clip region is the plotting area plus such arbitrary extra space.
 * The reason we clip with a bounding box larger than the plot area is so
 * that symbols will be drawn even if their center point isn't in the
 * plotting area.
 *---------------------------------------------------------------------------
 */

void Blt_GraphExtents(Graph* graphPtr, Region2d *regionPtr)
{
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
  regionPtr->left = (double)(graphPtr->hOffset_ - ops->xPad);
  regionPtr->top = (double)(graphPtr->vOffset_ - ops->yPad);
  regionPtr->right = (double)(graphPtr->hOffset_ + graphPtr->hRange_ + 
			      ops->xPad);
  regionPtr->bottom = (double)(graphPtr->vOffset_ + graphPtr->vRange_ + 
			       ops->yPad);
}

void Blt_ReconfigureGraph(Graph* graphPtr)	
{
  graphPtr->configure();
  graphPtr->legend_->configure();
  //  Blt_ConfigureElements(graphPtr);
  Blt_ConfigureAxes(graphPtr);
  Blt::ConfigureMarkers(graphPtr);
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
      Element* elemPtr = (Element*)object;
      ElementOptions* ops = (ElementOptions*)elemPtr->ops();
      MakeTagProc* tagProc = Blt_MakeElementTag;
      Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, elemPtr->name()), 0);
      Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, elemPtr->className()), 0);
      if (ops->tags)
	for (const char** p = ops->tags; *p != NULL; p++)
	  Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, *p), 0);
    }
    break;
  case CID_AXIS_X:
  case CID_AXIS_Y:
    {
      Axis* axisPtr = (Axis*)object;
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      MakeTagProc* tagProc = Blt_MakeAxisTag;
      Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, axisPtr->name()), 0);
      Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, axisPtr->className()), 0);
      if (ops->tags)
	for (const char** p = ops->tags; *p != NULL; p++)
	  Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, *p), 0);
    }
    break;
  case CID_MARKER_BITMAP:
  case CID_MARKER_LINE:
  case CID_MARKER_POLYGON:
  case CID_MARKER_TEXT:
  case CID_MARKER_WINDOW:
    {
      Marker* markerPtr = (Marker*)object;
      MarkerOptions* ops = (MarkerOptions*)markerPtr->ops();
      MakeTagProc* tagProc = Blt::MakeMarkerTag;
      Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, markerPtr->name()), 0);
      Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, markerPtr->className()), 0);
      if (ops->tags)
	for (const char** p = ops->tags; *p != NULL; p++)
	  Blt_List_Append(list, (const char*)(*tagProc)(graphPtr, *p), 0);

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
  Blt_GraphExtents(graphPtr, &exts);

  // Sample coordinate is in one of the graph margins. Can only pick an axis.
  if ((x >= exts.right) || (x < exts.left) || 
      (y >= exts.bottom) || (y < exts.top)) {
    Axis* axisPtr = Blt_NearestAxis(graphPtr, x, y);
    if (axisPtr) {
      *contextPtr = (ClientData)axisPtr->classId();
      return axisPtr;
    }
  }

  // From top-to-bottom check:
  // 1. markers drawn on top (-under false).
  // 2. elements using its display list back to front.
  // 3. markers drawn under element (-under true).
  Marker* markerPtr = (Marker*)Blt::NearestMarker(graphPtr, x, y, 0);
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
  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->hide() || (elemPtr->flags & MAP_ITEM))
      continue;

    elemPtr->closest();
  }
  // Found an element within the minimum halo distance.
  if (searchPtr->dist <= (double)searchPtr->halo) {
    *contextPtr = (ClientData)elemPtr->classId();
    return searchPtr->elemPtr;
  }

  markerPtr = (Marker*)Blt::NearestMarker(graphPtr, x, y, 1);
  if (markerPtr) {
    *contextPtr = (ClientData)markerPtr->classId();
    return markerPtr;
  }

  *contextPtr = (ClientData)NULL;
  return NULL;
}

