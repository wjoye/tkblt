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

using namespace Blt;

#define MARKER_ABOVE	0
#define MARKER_UNDER	1

extern void GraphConfigure(Graph* graphPtr);
extern void GraphDestroy(Graph* graphPtr);

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
   "0", -1, Tk_Offset(Graph, aspect), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   STD_NORMAL_BACKGROUND, -1, Tk_Offset(Graph, normalBg), 0, NULL, CACHE_DIRTY},
  {TK_OPTION_STRING_TABLE, "-barmode", "barMode", "BarMode", 
   "normal", -1, Tk_Offset(Graph, barMode), 0, &barmodeObjOption, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_DOUBLE, "-barwidth", "barWidth", "BarWidth", 
   ".9", -1, Tk_Offset(Graph, barWidth), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_DOUBLE, "-baseline", "baseline", "Baseline", 
   "0", -1, Tk_Offset(Graph, baseline), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_SYNONYM, "-bm", NULL, NULL, NULL, -1, 0, 0, "-bottommargin", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(Graph, borderWidth), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-bottommargin", "bottomMargin", "BottomMargin",
   "0", -1, Tk_Offset(Graph, bottomMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-bottomvariable", "bottomVariable", "BottomVariable",
   NULL, -1, Tk_Offset(Graph, bottomMargin.varName), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-bufferelements", "bufferElements", "BufferElements",
   "yes", -1, Tk_Offset(Graph, backingStore), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-buffergraph", "bufferGraph", "BufferGraph",
   "yes", -1, Tk_Offset(Graph, doubleBuffer), 0, NULL, 0},
  {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor", 
   "crosshair", -1, Tk_Offset(Graph, cursor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_MEDIUM, -1, Tk_Offset(Graph, titleTextStyle.font), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Graph, titleTextStyle.color), 0, NULL, 
   CACHE_DIRTY},
  {TK_OPTION_SYNONYM, "-halo", NULL, NULL, NULL, -1, 0, 0, "-searchhalo", 0},
  {TK_OPTION_PIXELS, "-height", "height", "Height", 
   "4i", -1, Tk_Offset(Graph, reqHeight), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
   "HighlightBackground", 
   STD_NORMAL_BACKGROUND, -1, Tk_Offset(Graph, highlightBgColor), 0, NULL, 
   CACHE_DIRTY},
  {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Graph, highlightColor), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
   "HighlightThickness", 
   "2", -1, Tk_Offset(Graph, highlightWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-invertxy", "invertXY", "InvertXY", 
   "no", -1, Tk_Offset(Graph, inverted), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY | RESET_AXES},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify", 
   "center", -1, Tk_Offset(Graph, titleTextStyle.justify), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-leftmargin", "leftMargin", "Margin", 
   "0", -1, Tk_Offset(Graph, leftMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-leftvariable", "leftVariable", "LeftVariable",
   NULL, -1, Tk_Offset(Graph, leftMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-lm", NULL, NULL, NULL, -1, 0, 0, "-leftmargin", 0},
  {TK_OPTION_BORDER, "-plotbackground", "plotbackground", "PlotBackground",
   STD_NORMAL_BACKGROUND, -1, Tk_Offset(Graph, plotBg), 0, NULL,
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotborderwidth", "plotBorderWidth", "PlotBorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(Graph, plotBW), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotpadx", "plotPadX", "PlotPad", 
   "0", -1, Tk_Offset(Graph, xPad), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotpady", "plotPadY", "PlotPad", 
   "0", -1, Tk_Offset(Graph, yPad), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_RELIEF, "-plotrelief", "plotRelief", "Relief", 
   "flat", -1, Tk_Offset(Graph, plotRelief), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief", 
   "flat", -1, Tk_Offset(Graph, relief), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-rightmargin", "rightMargin", "Margin", 
   "0", -1, Tk_Offset(Graph, rightMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-rightvariable", "rightVariable", "RightVariable",
   NULL, -1, Tk_Offset(Graph, rightMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-rm", NULL, NULL, NULL, -1, 0, 0, "-rightmargin", 0},
  {TK_OPTION_PIXELS, "-searchhalo", "searchhalo", "SearchHalo", 
   "2m", -1, Tk_Offset(Graph, search.halo), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-searchmode", "searchMode", "SearchMode",
   "points", -1, Tk_Offset(Graph, search.mode), 0, &searchModeObjOption, 0}, 
  {TK_OPTION_STRING_TABLE, "-searchalong", "searchAlong", "SearchAlong",
   "both", -1, Tk_Offset(Graph, search.along), 0, &searchAlongObjOption, 0},
  {TK_OPTION_BOOLEAN, "-stackaxes", "stackAxes", "StackAxes", 
   "no", -1, Tk_Offset(Graph, stackAxes), 0, NULL, 0},
  {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
   NULL, -1, Tk_Offset(Graph, takeFocus), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title", 
   NULL, -1, Tk_Offset(Graph, title), TK_OPTION_NULL_OK, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_SYNONYM, "-tm", NULL, NULL, NULL, -1, 0, 0, "-topmargin", 0},
  {TK_OPTION_PIXELS, "-topmargin", "topMargin", "TopMargin", 
   "0", -1, Tk_Offset(Graph, topMargin.reqSize), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_STRING, "-topvariable", "topVariable", "TopVariable",
   NULL, -1, Tk_Offset(Graph, topMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-width", "width", "Width", 
   "5i", -1, Tk_Offset(Graph, reqWidth), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotwidth", "plotWidth", "PlotWidth", 
   "0", -1, Tk_Offset(Graph, reqPlotWidth), 0, NULL, RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_PIXELS, "-plotheight", "plotHeight", "PlotHeight", 
   "0", -1, Tk_Offset(Graph, reqPlotHeight), 0, NULL, 
   RESET_WORLD | CACHE_DIRTY},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

int NewGraph(ClientData clientData, Tcl_Interp*interp, 
	     int objc, Tcl_Obj* const objv[], ClassId classId)
{
  Tk_Window tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
					    Tcl_GetString(objv[1]), NULL);
  if (!tkwin)
    return TCL_ERROR;

  switch (classId) {
  case CID_ELEM_LINE:
    Tk_SetClass(tkwin, "Graph");
    break;
  case CID_ELEM_BAR:
    Tk_SetClass(tkwin, "Barchart");
    break;
  default:
    break;
  }

  Tk_OptionTable optionTable = Tk_CreateOptionTable(interp, optionSpecs);

  Graph* graphPtr = (Graph*)calloc(1, sizeof(Graph));
  ((TkWindow*)tkwin)->instanceData = graphPtr;

  graphPtr->tkwin = tkwin;
  graphPtr->display = Tk_Display(tkwin);
  graphPtr->interp = interp;
  graphPtr->cmdToken = Tcl_CreateObjCommand(interp, 
					    Tk_PathName(graphPtr->tkwin), 
					    GraphInstCmdProc, 
					    graphPtr, 
					    GraphInstCmdDeleteProc);
  graphPtr->optionTable = optionTable;
  graphPtr->classId = classId;
  graphPtr->backingStore = 1;
  graphPtr->doubleBuffer = 1;
  graphPtr->borderWidth = 2;
  graphPtr->plotBW = 1;
  graphPtr->highlightWidth = 2;
  graphPtr->plotRelief = TK_RELIEF_SOLID;
  graphPtr->relief = TK_RELIEF_FLAT;
  graphPtr->flags = MAP_WORLD | REDRAW_WORLD;
  graphPtr->nextMarkerId = 1;
  graphPtr->bottomMargin.site = MARGIN_BOTTOM;
  graphPtr->leftMargin.site = MARGIN_LEFT;
  graphPtr->topMargin.site = MARGIN_TOP;
  graphPtr->rightMargin.site = MARGIN_RIGHT;

  Blt_Ts_InitStyle(graphPtr->titleTextStyle);
  graphPtr->titleTextStyle.anchor = TK_ANCHOR_N;

  Tcl_InitHashTable(&graphPtr->axes.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->axes.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->elements.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->elements.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->markers.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->markers.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->dataTables, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->penTable, TCL_STRING_KEYS);
  graphPtr->elements.displayList = Blt_Chain_Create();
  graphPtr->markers.displayList = Blt_Chain_Create();
  graphPtr->axes.displayList = Blt_Chain_Create();
  graphPtr->bindTable = Blt_CreateBindingTable(interp, tkwin, graphPtr, 
					       PickEntry, Blt_GraphTags);
  graphPtr->legend = new Legend(graphPtr);
  graphPtr->crosshairs = new Crosshairs(graphPtr);

  if (Blt_CreatePen(graphPtr, interp, "activeLine", CID_ELEM_LINE, 0, NULL) != 
      TCL_OK)
    goto error;
  if (Blt_CreatePen(graphPtr, interp, "activeBar", CID_ELEM_BAR, 0, NULL) != 
      TCL_OK)
    goto error;
  if (Blt_CreateAxes(graphPtr) != TCL_OK)
    goto error;
  if (Blt_CreatePageSetup(graphPtr) != TCL_OK)
    goto error;

  // Keep a hold of the associated tkwin until we destroy the graph,
  // otherwise Tk might free it while we still need it.
  Tcl_Preserve(graphPtr->tkwin);

  Tk_CreateEventHandler(graphPtr->tkwin, 
			ExposureMask|StructureNotifyMask|FocusChangeMask,
			GraphEventProc, graphPtr);

  if ((Tk_InitOptions(interp, (char*)graphPtr, optionTable, tkwin) != TCL_OK) ||
      (GraphObjConfigure(interp, graphPtr, objc-2, objv+2) != TCL_OK))
    return TCL_ERROR;

  AdjustAxisPointers(graphPtr);

  Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(graphPtr->tkwin), -1);
  return TCL_OK;

 error:
  GraphDestroy(graphPtr);
  return TCL_ERROR;
}

void GraphDestroy(Graph* graphPtr)
{
  Blt::DestroyMarkers(graphPtr);

  if (graphPtr->crosshairs)
    delete graphPtr->crosshairs;
  Blt_DestroyElements(graphPtr);  // must come before legend and others
  if (graphPtr->legend)
    delete graphPtr->legend;

  Blt_DestroyAxes(graphPtr);
  Blt_DestroyPens(graphPtr);
  Blt_DestroyPageSetup(graphPtr);
  Blt_DestroyBarSets(graphPtr);

  if (graphPtr->bindTable)
    Blt_DestroyBindingTable(graphPtr->bindTable);

  if (graphPtr->drawGC)
    Tk_FreeGC(graphPtr->display, graphPtr->drawGC);

  Blt_Ts_FreeStyle(graphPtr->display, &graphPtr->titleTextStyle);
  if (graphPtr->cache != None)
    Tk_FreePixmap(graphPtr->display, graphPtr->cache);

  Tk_FreeConfigOptions((char*)graphPtr, graphPtr->optionTable, graphPtr->tkwin);
  Tcl_Release(graphPtr->tkwin);
  graphPtr->tkwin = NULL;
  free(graphPtr);
}

void GraphConfigure(Graph* graphPtr)	
{
  // Don't allow negative bar widths. Reset to an arbitrary value (0.1)
  if (graphPtr->barWidth <= 0.0f) {
    graphPtr->barWidth = 0.8f;
  }
  graphPtr->inset = graphPtr->borderWidth + graphPtr->highlightWidth;
  if ((graphPtr->reqHeight != Tk_ReqHeight(graphPtr->tkwin)) ||
      (graphPtr->reqWidth != Tk_ReqWidth(graphPtr->tkwin))) {
    Tk_GeometryRequest(graphPtr->tkwin, graphPtr->reqWidth,
		       graphPtr->reqHeight);
  }
  Tk_SetInternalBorder(graphPtr->tkwin, graphPtr->borderWidth);
  XColor* colorPtr = Tk_3DBorderColor(graphPtr->normalBg);

  graphPtr->titleWidth = graphPtr->titleHeight = 0;
  if (graphPtr->title != NULL) {
    unsigned int w, h;

    Blt_Ts_GetExtents(&graphPtr->titleTextStyle, graphPtr->title, &w, &h);
    graphPtr->titleHeight = h;
  }

  // Create GCs for interior and exterior regions, and a background GC for
  // clearing the margins with XFillRectangle
  // Margin
  XGCValues gcValues;
  gcValues.foreground = graphPtr->titleTextStyle.color->pixel;
  gcValues.background = colorPtr->pixel;
  unsigned long gcMask = (GCForeground | GCBackground);
  GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (graphPtr->drawGC != NULL) {
    Tk_FreeGC(graphPtr->display, graphPtr->drawGC);
  }
  graphPtr->drawGC = newGC;

  // If the -inverted option changed, we need to readjust the pointers
  // to the axes and recompute the their scales.
  if (graphPtr->flags & RESET_AXES)
    AdjustAxisPointers(graphPtr);

  // Free the pixmap if we're not buffering the display of elements anymore.
  if ((!graphPtr->backingStore) && (graphPtr->cache != None)) {
    Tk_FreePixmap(graphPtr->display, graphPtr->cache);
    graphPtr->cache = None;
  }
}

void GraphDisplay(Graph* graphPtr)
{
  Tk_Window tkwin = graphPtr->tkwin;

  graphPtr->flags &= ~REDRAW_PENDING;
  if ((graphPtr->flags & GRAPH_DELETED) || !Tk_IsMapped(tkwin))
    return;

  if ((Tk_Width(tkwin) <= 1) || (Tk_Height(tkwin) <= 1)) {
    /* Don't bother computing the layout until the size of the window is
     * something reasonable. */
    return;
  }
  graphPtr->width = Tk_Width(tkwin);
  graphPtr->height = Tk_Height(tkwin);
  Blt_MapGraph(graphPtr);
  if (!Tk_IsMapped(tkwin)) {
    /* The graph's window isn't displayed, so don't bother drawing
     * anything.  By getting this far, we've at least computed the
     * coordinates of the graph's new layout.  */
    return;
  }
  /* Create a pixmap the size of the window for double buffering. */
  Pixmap drawable;
  if (graphPtr->doubleBuffer)
    drawable = Tk_GetPixmap(graphPtr->display, Tk_WindowId(tkwin), 
			    graphPtr->width, graphPtr->height, Tk_Depth(tkwin));
  else
    drawable = Tk_WindowId(tkwin);

  if (graphPtr->backingStore) {
    if ((graphPtr->cache == None) || 
	(graphPtr->cacheWidth != graphPtr->width) ||
	(graphPtr->cacheHeight != graphPtr->height)) {
      if (graphPtr->cache != None) {
	Tk_FreePixmap(graphPtr->display, graphPtr->cache);
      }


      graphPtr->cache = Tk_GetPixmap(graphPtr->display, 
				     Tk_WindowId(tkwin), 
				     graphPtr->width, graphPtr->height, 
				     Tk_Depth(tkwin));
      graphPtr->cacheWidth  = graphPtr->width;
      graphPtr->cacheHeight = graphPtr->height;
      graphPtr->flags |= CACHE_DIRTY;
    }
  }
  if (graphPtr->backingStore) {
    if (graphPtr->flags & CACHE_DIRTY) {
      /* The backing store is new or out-of-date. */
      DrawPlot(graphPtr, graphPtr->cache);
      graphPtr->flags &= ~CACHE_DIRTY;
    }
    /* Copy the pixmap to the one used for drawing the entire graph. */
    XCopyArea(graphPtr->display, graphPtr->cache, drawable,
	      graphPtr->drawGC, 0, 0, Tk_Width(graphPtr->tkwin),
	      Tk_Height(graphPtr->tkwin), 0, 0);
  }
  else
    DrawPlot(graphPtr, drawable);

  // Draw markers above elements
  Blt::DrawMarkers(graphPtr, drawable, MARKER_ABOVE);
  Blt_DrawActiveElements(graphPtr, drawable);

  // Don't draw legend in the plot area.
  if (graphPtr->legend->isRaised()) {
    switch (graphPtr->legend->position()) {
    case Legend::PLOT:
    case Legend::XY:
      graphPtr->legend->draw(drawable);
      break;
    default:
      break;
    }
  }

  // Draw 3D border just inside of the focus highlight ring
  if ((graphPtr->borderWidth > 0) && (graphPtr->relief != TK_RELIEF_FLAT)) {
    Tk_Draw3DRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
		       graphPtr->highlightWidth, graphPtr->highlightWidth, 
		       graphPtr->width - 2*graphPtr->highlightWidth, 
		       graphPtr->height - 2*graphPtr->highlightWidth, 
		       graphPtr->borderWidth, graphPtr->relief);
  }
  /* Draw focus highlight ring. */
  if ((graphPtr->highlightWidth > 0) && (graphPtr->flags & FOCUS)) {
    GC gc;

    gc = Tk_GCForColor(graphPtr->highlightColor, drawable);
    Tk_DrawFocusHighlight(graphPtr->tkwin, gc, graphPtr->highlightWidth,
			  drawable);
  }
  /* Disable crosshairs before redisplaying to the screen */
  Blt_DisableCrosshairs(graphPtr);
  XCopyArea(graphPtr->display, drawable, Tk_WindowId(tkwin),
	    graphPtr->drawGC, 0, 0, graphPtr->width, graphPtr->height, 0, 0);
  Blt_EnableCrosshairs(graphPtr);
  if (graphPtr->doubleBuffer) {
    Tk_FreePixmap(graphPtr->display, drawable);
  }

  graphPtr->flags &= ~MAP_WORLD;
  graphPtr->flags &= ~REDRAW_WORLD;
  UpdateMarginTraces(graphPtr);
}

// Support

void Blt_EventuallyRedrawGraph(Graph* graphPtr) 
{
  if ((graphPtr->flags & GRAPH_DELETED) || !Tk_IsMapped(graphPtr->tkwin))
    return;

  if (!(graphPtr->flags & REDRAW_PENDING)) {
    graphPtr->flags |= REDRAW_PENDING;
    Tcl_DoWhenIdle(DisplayGraph, graphPtr);
  }
}

static void AdjustAxisPointers(Graph* graphPtr) 
{
  if (graphPtr->inverted) {
    graphPtr->leftMargin.axes   = graphPtr->axisChain[0];
    graphPtr->bottomMargin.axes = graphPtr->axisChain[1];
    graphPtr->rightMargin.axes  = graphPtr->axisChain[2];
    graphPtr->topMargin.axes    = graphPtr->axisChain[3];
  }
  else {
    graphPtr->leftMargin.axes   = graphPtr->axisChain[1];
    graphPtr->bottomMargin.axes = graphPtr->axisChain[0];
    graphPtr->rightMargin.axes  = graphPtr->axisChain[3];
    graphPtr->topMargin.axes    = graphPtr->axisChain[2];
  }
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

  ClosestSearch* searchPtr = &graphPtr->search;
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
  XRectangle rects[4];

  /*
   * Draw the four outer rectangles which encompass the plotting
   * surface. This clears the surrounding area and clips the plot.
   */
  rects[0].x = rects[0].y = rects[3].x = rects[1].x = 0;
  rects[0].width = rects[3].width = (short int)graphPtr->width;
  rects[0].height = (short int)graphPtr->top;
  rects[3].y = graphPtr->bottom;
  rects[3].height = graphPtr->height - graphPtr->bottom;
  rects[2].y = rects[1].y = graphPtr->top;
  rects[1].width = graphPtr->left;
  rects[2].height = rects[1].height = graphPtr->bottom - graphPtr->top;
  rects[2].x = graphPtr->right;
  rects[2].width = graphPtr->width - graphPtr->right;

  Tk_Fill3DRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
		     rects[0].x, rects[0].y, rects[0].width, rects[0].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
		     rects[1].x, rects[1].y, rects[1].width, rects[1].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
		     rects[2].x, rects[2].y, rects[2].width, rects[2].height, 
		     0, TK_RELIEF_FLAT);
  Tk_Fill3DRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
		     rects[3].x, rects[3].y, rects[3].width, rects[3].height, 
		     0, TK_RELIEF_FLAT);

  /* Draw 3D border around the plotting area */

  if (graphPtr->plotBW > 0) {
    int x, y, w, h;

    x = graphPtr->left - graphPtr->plotBW;
    y = graphPtr->top - graphPtr->plotBW;
    w = (graphPtr->right - graphPtr->left) + (2*graphPtr->plotBW);
    h = (graphPtr->bottom - graphPtr->top) + (2*graphPtr->plotBW);
    Tk_Draw3DRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
		       x, y, w, h, graphPtr->plotBW, graphPtr->plotRelief);
  }
  
  switch (graphPtr->legend->position()) {
  case Legend::TOP:
  case Legend::BOTTOM:
  case Legend::RIGHT:
  case Legend::LEFT:
    graphPtr->legend->draw(drawable);
    break;
  default:
    break;
  }

  if (graphPtr->title != NULL)
    Blt_DrawText(graphPtr->tkwin, drawable, graphPtr->title,
		 &graphPtr->titleTextStyle, graphPtr->titleX, graphPtr->titleY);

  Blt_DrawAxes(graphPtr, drawable);
  graphPtr->flags &= ~DRAW_MARGINS;
}

static void DrawPlot(Graph* graphPtr, Drawable drawable)
{
  DrawMargins(graphPtr, drawable);

  // Draw the background of the plotting area with 3D border
  Tk_Fill3DRectangle(graphPtr->tkwin, drawable, graphPtr->plotBg,
		     graphPtr->left-graphPtr->plotBW, 
		     graphPtr->top-graphPtr->plotBW, 
		     graphPtr->right - graphPtr->left + 1 +2*graphPtr->plotBW,
		     graphPtr->bottom - graphPtr->top  + 1 +2*graphPtr->plotBW, 
		     graphPtr->plotBW, graphPtr->plotRelief);
  
  // Draw the elements, markers, legend, and axis limits
  Blt_DrawAxes(graphPtr, drawable);
  Blt_DrawGrids(graphPtr, drawable);
  Blt::DrawMarkers(graphPtr, drawable, MARKER_UNDER);

  if (!graphPtr->legend->isRaised()) {
    switch (graphPtr->legend->position()) {
    case Legend::PLOT:
    case Legend::XY:
      graphPtr->legend->draw(drawable);
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

  if ((graphPtr->vRange > 1) && (graphPtr->hRange > 1)) {
    if (graphPtr->flags & MAP_WORLD)
      Blt_MapAxes(graphPtr);

    Blt_MapElements(graphPtr);
    Blt::MapMarkers(graphPtr);
    graphPtr->flags &= ~(MAP_ALL);
  }
}

static void UpdateMarginTraces(Graph* graphPtr)
{
  Margin* marginPtr;
  Margin* endPtr;

  for (marginPtr = graphPtr->margins, endPtr = marginPtr + 4; 
       marginPtr < endPtr; marginPtr++) {
    if (marginPtr->varName != NULL) { /* Trigger variable traces */
      int size;

      if ((marginPtr->site == MARGIN_LEFT) || 
	  (marginPtr->site == MARGIN_RIGHT)) {
	size = marginPtr->width;
      } else {
	size = marginPtr->height;
      }
      Tcl_SetVar(graphPtr->interp, marginPtr->varName, Blt_Itoa(size), 
		 TCL_GLOBAL_ONLY);
    }
  }
}

Graph* Blt_GetGraphFromWindowData(Tk_Window tkwin)
{
  while (tkwin) {
    TkWindow* winPtr = (TkWindow*)tkwin;
    if (winPtr->instanceData != NULL) {
      Graph* graphPtr = (Graph*)winPtr->instanceData;
      if (graphPtr)
	return graphPtr;
    }
    tkwin = Tk_Parent(tkwin);
  }
  return NULL;
}

void Blt_ReconfigureGraph(Graph* graphPtr)	
{
  GraphConfigure(graphPtr);
  graphPtr->legend->configure();
  //  Blt_ConfigureElements(graphPtr);
  Blt_ConfigureAxes(graphPtr);
  Blt::ConfigureMarkers(graphPtr);
}
