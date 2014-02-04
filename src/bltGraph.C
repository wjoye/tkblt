/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGraph.c --
 *
 * This module implements a graph widget for the BLT toolkit.
 *
 * The graph widget was created by Sani Nassif and George Howlett.
 *
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

#include <assert.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bltInt.h"
#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltGrElem.h"
#include "bltSwitch.h"

typedef int (GraphCmdProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			   Tcl_Obj* const objv[]);

#define PointInRegion(e,x,y)			\
  (((x) <= (e)->right) && ((x) >= (e)->left) && \
   ((y) <= (e)->bottom) && ((y) >= (e)->top))

/* 
 * Objects in the graph have their own class names.  These class names are
 * used for the resource database and bindings.  Example.
 *
 *	option add *X.title "X Axis Title" widgetDefault
 *	.g marker bind BitmapMarker <Enter> { ... }
 *
 * The option database trick is performed by creating a temporary window when
 * an object is initially configured.  The class name of the temporary window
 * will be from the list below.
 */
static const char* objectClassNames[] = {
  "unknown",
  "XAxis", 
  "YAxis",
  "BarElement", 
  "ContourElement",
  "LineElement", 
  "StripElement", 
  "BitmapMarker", 
  "ImageMarker", 
  "LineMarker", 
  "PolygonMarker",
  "TextMarker", 
  "WindowMarker",
};

extern Blt_CustomOption bltLinePenOption;
extern Blt_CustomOption bltBarPenOption;
extern Blt_CustomOption bltBarModeOption;

#define DEF_GRAPH_ASPECT_RATIO		"0.0"
#define DEF_GRAPH_BAR_BASELINE		"0.0"
#define DEF_GRAPH_BAR_MODE		"normal"
#define DEF_GRAPH_BAR_WIDTH		"0.9"
#define DEF_GRAPH_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_GRAPH_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_GRAPH_BUFFER_ELEMENTS	"yes"
#define DEF_GRAPH_BUFFER_GRAPH		"1"
#define DEF_GRAPH_CURSOR		"crosshair"
#define DEF_GRAPH_FONT			STD_FONT_MEDIUM
#define DEF_GRAPH_HALO			"2m"
#define DEF_GRAPH_HALO_BAR		"0.1i"
#define DEF_GRAPH_HEIGHT		"4i"
#define DEF_GRAPH_HIGHLIGHT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_GRAPH_HIGHLIGHT_COLOR	black
#define DEF_GRAPH_HIGHLIGHT_WIDTH	"2"
#define DEF_GRAPH_INVERT_XY		"0"
#define DEF_GRAPH_JUSTIFY		"center"
#define DEF_GRAPH_MARGIN		"0"
#define DEF_GRAPH_MARGIN_VAR		NULL
#define DEF_GRAPH_PLOT_BACKGROUND	"white"
#define DEF_GRAPH_PLOT_BORDERWIDTH	"1"
#define DEF_GRAPH_PLOT_HEIGHT           "0"
#define DEF_GRAPH_PLOT_PADX		"0"
#define DEF_GRAPH_PLOT_PADY		"0"
#define DEF_GRAPH_PLOT_RELIEF		"solid"
#define DEF_GRAPH_PLOT_WIDTH            "0"
#define DEF_GRAPH_RELIEF		"flat"
#define DEF_GRAPH_SHOW_VALUES		"no"
#define DEF_GRAPH_STACK_AXES		"no"
#define DEF_GRAPH_TAKE_FOCUS		""
#define DEF_GRAPH_TITLE			NULL
#define DEF_GRAPH_TITLE_COLOR		STD_NORMAL_FOREGROUND
#define DEF_GRAPH_UNMAP_HIDDEN_ELEMENTS	"0"
#define DEF_GRAPH_WIDTH			"5i"

// Background

static Tk_CustomOptionSetProc BackgroundSetProc;
static Tk_CustomOptionGetProc BackgroundGetProc;
static Tk_ObjCustomOption backgroundObjOption =
  {
    "background", BackgroundSetProc, BackgroundGetProc, NULL, NULL, NULL
  };


static int BackgroundSetProc(ClientData clientData, Tcl_Interp *interp,
			     Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			     int offset, char* save, int flags)
{
  Blt_Background* backgroundPtr = (Blt_Background*)(widgRec + offset);

  if (*backgroundPtr)
    Blt_FreeBackground(*backgroundPtr);
  *backgroundPtr = NULL;

  int length;
  const char* string = Tcl_GetStringFromObj(*objPtr, &length);
  if (string)
    *backgroundPtr = Blt_GetBackground(interp, tkwin, string);
  else
    return TCL_ERROR;

  return TCL_OK;
}

static Tcl_Obj* BackgroundGetProc(ClientData clientData, Tk_Window tkwin, 
				  char *widgRec, int offset)
{
  Blt_Background* backgroundPtr = (Blt_Background*)(widgRec + offset);
  if (*backgroundPtr) {
    const char* string = Blt_NameOfBackground(*backgroundPtr);
    return Tcl_NewStringObj(string, -1);
  }
  else
    return Tcl_NewStringObj("", -1);
}

// BarMode

static char* barmodeObjOption[] = {"normal", "stacked", "aligned", "overlap"};

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_DOUBLE, "-aspect", "aspect", "Aspect", 
   DEF_GRAPH_ASPECT_RATIO, 
   -1, Tk_Offset(Graph, aspect), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-background", "background", "Background",
   DEF_GRAPH_BACKGROUND, 
   -1, Tk_Offset(Graph, normalBg), 0, &backgroundObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-barmode", "barMode", "BarMode", 
   DEF_GRAPH_BAR_MODE, 
   -1, Tk_Offset(Graph, mode), 0, &barmodeObjOption, 0},
  {TK_OPTION_DOUBLE, "-barwidth", "barWidth", "BarWidth", 
   DEF_GRAPH_BAR_WIDTH,
   -1, Tk_Offset(Graph, barWidth), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-baseline", "baseline", "Baseline", 
   DEF_GRAPH_BAR_BASELINE, 
   -1, Tk_Offset(Graph, baseline), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL,
   -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL,
   -1, 0, 0, "-background", 0},
  {TK_OPTION_SYNONYM, "-bm", NULL, NULL, NULL, 
   -1, 0, 0, "-bottommargin", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   DEF_GRAPH_BORDERWIDTH, 
   -1, Tk_Offset(Graph, borderWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-bottommargin", "bottomMargin", "BottomMargin",
   DEF_GRAPH_MARGIN, 
   -1, Tk_Offset(Graph, bottomMargin.reqSize), 0, NULL, 0},
  {TK_OPTION_STRING, "-bottomvariable", "bottomVariable", "BottomVariable",
   DEF_GRAPH_MARGIN_VAR, 
   -1, Tk_Offset(Graph, bottomMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-bufferelements", "bufferElements", "BufferElements",
   DEF_GRAPH_BUFFER_ELEMENTS, 
   -1, Tk_Offset(Graph, backingStore), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-buffergraph", "bufferGraph", "BufferGraph",
   DEF_GRAPH_BUFFER_GRAPH, 
   -1, Tk_Offset(Graph, doubleBuffer), 0, NULL, 0},
  {TK_OPTION_CURSOR, "-cursor", "cursor", "Cursor", 
   DEF_GRAPH_CURSOR, 
   -1, Tk_Offset(Graph, cursor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, 
   -1, 0, 0, "-foreground", 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   DEF_GRAPH_FONT,
   -1, Tk_Offset(Graph, titleTextStyle.font), 0, NULL, 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   DEF_GRAPH_TITLE_COLOR, 
   -1, Tk_Offset(Graph, titleTextStyle.color), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-halo", "halo", "Halo", 
   DEF_GRAPH_HALO, 
   -1, Tk_Offset(Graph, halo), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-height", "height", "Height", 
   DEF_GRAPH_HEIGHT, 
   -1, Tk_Offset(Graph, reqHeight), 0, NULL, 0},
  {TK_OPTION_COLOR, "-highlightbackground", "highlightBackground",
   "HighlightBackground", 
   DEF_GRAPH_HIGHLIGHT_BACKGROUND, 
   -1, Tk_Offset(Graph, highlightBgColor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
   DEF_GRAPH_HIGHLIGHT_COLOR, 
   -1, Tk_Offset(Graph, highlightColor), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-highlightthickness", "highlightThickness",
   "HighlightThickness", 
   DEF_GRAPH_HIGHLIGHT_WIDTH, 
   -1, Tk_Offset(Graph, highlightWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-invertxy", "invertXY", "InvertXY", 
   DEF_GRAPH_INVERT_XY,
   -1, Tk_Offset(Graph, inverted), 0, NULL, 0},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify", 
   DEF_GRAPH_JUSTIFY, 
   -1, Tk_Offset(Graph, titleTextStyle.justify), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-leftmargin", "leftMargin", "Margin", 
   DEF_GRAPH_MARGIN,
   -1, Tk_Offset(Graph, leftMargin.reqSize), 0, NULL, 0},
  {TK_OPTION_STRING, "-leftvariable", "leftVariable", "LeftVariable",
   DEF_GRAPH_MARGIN_VAR,
   -1, Tk_Offset(Graph, leftMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-lm", NULL, NULL, NULL, 
   -1, 0, 0, "-leftmargin", 0},
  {TK_OPTION_CUSTOM, "-plotbackground", "plotbackground", "PlotBackground",
   DEF_GRAPH_PLOT_BACKGROUND, 
   -1, Tk_Offset(Graph, plotBg), 0, &backgroundObjOption, 0},
  {TK_OPTION_PIXELS, "-plotborderwidth", "plotBorderWidth", "PlotBorderWidth",
   DEF_GRAPH_PLOT_BORDERWIDTH, 
   -1, Tk_Offset(Graph, plotBW), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-plotpadx", "plotPadX", "PlotPad", 
   DEF_GRAPH_PLOT_PADX, 
   -1, Tk_Offset(Graph, xPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-plotpady", "plotPadY", "PlotPad", 
   DEF_GRAPH_PLOT_PADY, 
   -1, Tk_Offset(Graph, yPad), 0, NULL, 0},
  {TK_OPTION_RELIEF, "-plotrelief", "plotRelief", "Relief", 
   DEF_GRAPH_PLOT_RELIEF, 
   -1, Tk_Offset(Graph, plotRelief), 0, NULL, 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief", 
   DEF_GRAPH_RELIEF, 
   -1, Tk_Offset(Graph, relief), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-rightmargin", "rightMargin", "Margin", 
   DEF_GRAPH_MARGIN,
   -1, Tk_Offset(Graph, rightMargin.reqSize), 0, NULL, 0},
  {TK_OPTION_STRING, "-rightvariable", "rightVariable", "RightVariable",
   DEF_GRAPH_MARGIN_VAR,
   -1, Tk_Offset(Graph, rightMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-rm", NULL, NULL, NULL,
   -1, 0, 0, "-rightmargin", 0},
  {TK_OPTION_BOOLEAN, "-stackaxes", "stackAxes", "StackAxes", 
   DEF_GRAPH_STACK_AXES, 
   -1, Tk_Offset(Graph, stackAxes), 0, NULL, 0},
  {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus",
   DEF_GRAPH_TAKE_FOCUS, 
   -1, Tk_Offset(Graph, takeFocus), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title", 
   DEF_GRAPH_TITLE, 
   -1, Tk_Offset(Graph, title), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-tm", NULL, NULL, NULL,
   -1, 0, 0, "-topmargin", 0},
  {TK_OPTION_PIXELS, "-topmargin", "topMargin", "TopMargin", 
   DEF_GRAPH_MARGIN,
   -1, Tk_Offset(Graph, topMargin.reqSize), 0, NULL, 0},
  {TK_OPTION_STRING, "-topvariable", "topVariable", "TopVariable",
   DEF_GRAPH_MARGIN_VAR, 
   -1, Tk_Offset(Graph, topMargin.varName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-width", "width", "Width", 
   DEF_GRAPH_WIDTH, 
   -1, Tk_Offset(Graph, reqWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-plotwidth", "plotWidth", "PlotWidth", 
   DEF_GRAPH_PLOT_WIDTH,
   -1, Tk_Offset(Graph, reqPlotWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-plotheight", "plotHeight", "PlotHeight", 
   DEF_GRAPH_PLOT_HEIGHT,
   -1, Tk_Offset(Graph, reqPlotHeight), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

Blt_CustomOption bitmaskGraphUnmapOption =
  {
    ObjToBitmaskProc, BitmaskToObjProc, NULL, (ClientData)UNMAP_HIDDEN
  };

static Blt_ConfigSpec configSpecs[] = {
  {BLT_CONFIG_DOUBLE, "-aspect", "aspect", "Aspect", DEF_GRAPH_ASPECT_RATIO, 
   Tk_Offset(Graph, aspect), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-background", "background", "Background",
   DEF_GRAPH_BACKGROUND, Tk_Offset(Graph, normalBg), 0,
   &backgroundOption},
  {BLT_CONFIG_CUSTOM, "-barmode", "barMode", "BarMode", DEF_GRAPH_BAR_MODE, 
   Tk_Offset(Graph, mode), BLT_CONFIG_DONT_SET_DEFAULT, 
   &bltBarModeOption},
  {BLT_CONFIG_DOUBLE, "-barwidth", "barWidth", "BarWidth", 
   DEF_GRAPH_BAR_WIDTH, Tk_Offset(Graph, barWidth), 0},
  {BLT_CONFIG_DOUBLE, "-baseline", "baseline", "Baseline",
   DEF_GRAPH_BAR_BASELINE, Tk_Offset(Graph, baseline), 0},
  {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char*)NULL, (char*)NULL,0, 0},
  {BLT_CONFIG_SYNONYM, "-bg", "background", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_SYNONYM, "-bm", "bottomMargin", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   DEF_GRAPH_BORDERWIDTH, Tk_Offset(Graph, borderWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-bottommargin", "bottomMargin", "Margin",
   DEF_GRAPH_MARGIN, Tk_Offset(Graph, bottomMargin.reqSize), 0},
  {BLT_CONFIG_STRING, "-bottomvariable", "bottomVariable", "BottomVariable",
   DEF_GRAPH_MARGIN_VAR, Tk_Offset(Graph, bottomMargin.varName), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-bufferelements", "bufferElements", "BufferElements",
   DEF_GRAPH_BUFFER_ELEMENTS, Tk_Offset(Graph, backingStore),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-buffergraph", "bufferGraph", "BufferGraph",
   DEF_GRAPH_BUFFER_GRAPH, Tk_Offset(Graph, doubleBuffer),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
   DEF_GRAPH_CURSOR, Tk_Offset(Graph, cursor), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_FONT, "-font", "font", "Font",
   DEF_GRAPH_FONT, Tk_Offset(Graph, titleTextStyle.font), 0},
  {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
   DEF_GRAPH_TITLE_COLOR, Tk_Offset(Graph, titleTextStyle.color), 0},
  {BLT_CONFIG_PIXELS, "-halo", "halo", "Halo", DEF_GRAPH_HALO, 
   Tk_Offset(Graph, halo), 0},
  {BLT_CONFIG_PIXELS, "-height", "height", "Height", DEF_GRAPH_HEIGHT, 
   Tk_Offset(Graph, reqHeight), 0},
  {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
   "HighlightBackground", DEF_GRAPH_HIGHLIGHT_BACKGROUND, 
   Tk_Offset(Graph, highlightBgColor), 0},
  {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
   DEF_GRAPH_HIGHLIGHT_COLOR, Tk_Offset(Graph, highlightColor), 0},
  {BLT_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
   "HighlightThickness", DEF_GRAPH_HIGHLIGHT_WIDTH, 
   Tk_Offset(Graph, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-unmaphiddenelements", "unmapHiddenElements", 
   "UnmapHiddenElements", DEF_GRAPH_UNMAP_HIDDEN_ELEMENTS, 
   Tk_Offset(Graph, flags), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, 
   &bitmaskGraphUnmapOption},
  {BLT_CONFIG_BOOLEAN, "-invertxy", "invertXY", "InvertXY", 
   DEF_GRAPH_INVERT_XY, Tk_Offset(Graph, inverted),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_GRAPH_JUSTIFY, 
   Tk_Offset(Graph, titleTextStyle.justify), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-leftmargin", "leftMargin", "Margin", 
   DEF_GRAPH_MARGIN, Tk_Offset(Graph, leftMargin.reqSize), 
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-leftvariable", "leftVariable", "LeftVariable",
   DEF_GRAPH_MARGIN_VAR, Tk_Offset(Graph, leftMargin.varName), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-lm", "leftMargin", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_CUSTOM, "-plotbackground", "plotBackground", "Background",
   DEF_GRAPH_PLOT_BACKGROUND, Tk_Offset(Graph, plotBg), 0,
   &backgroundOption},
  {BLT_CONFIG_PIXELS, "-plotborderwidth", "plotBorderWidth", 
   "PlotBorderWidth", DEF_GRAPH_PLOT_BORDERWIDTH, 
   Tk_Offset(Graph, plotBW), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-plotpadx", "plotPadX", "PlotPad", DEF_GRAPH_PLOT_PADX, 
   Tk_Offset(Graph, xPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-plotpady", "plotPadY", "PlotPad", DEF_GRAPH_PLOT_PADY, 
   Tk_Offset(Graph, yPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_RELIEF, "-plotrelief", "plotRelief", "Relief", 
   DEF_GRAPH_PLOT_RELIEF, Tk_Offset(Graph, plotRelief),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_GRAPH_RELIEF, 
   Tk_Offset(Graph, relief), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-rightmargin", "rightMargin", "Margin",
   DEF_GRAPH_MARGIN, Tk_Offset(Graph, rightMargin.reqSize),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-rightvariable", "rightVariable", "RightVariable",
   DEF_GRAPH_MARGIN_VAR, Tk_Offset(Graph, rightMargin.varName), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-rm", "rightMargin", (char*)NULL, (char*)NULL, 0,0},
  {BLT_CONFIG_BOOLEAN, "-stackaxes", "stackAxes", "StackAxes", 
   DEF_GRAPH_STACK_AXES, Tk_Offset(Graph, stackAxes),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
   DEF_GRAPH_TAKE_FOCUS, Tk_Offset(Graph, takeFocus), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_STRING, "-title", "title", "Title", DEF_GRAPH_TITLE, 
   Tk_Offset(Graph, title), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-tm", "topMargin", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_PIXELS, "-topmargin", "topMargin", "Margin", 
   DEF_GRAPH_MARGIN, Tk_Offset(Graph, topMargin.reqSize), 
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-topvariable", "topVariable", "TopVariable",
   DEF_GRAPH_MARGIN_VAR, Tk_Offset(Graph, topMargin.varName), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_PIXELS, "-width", "width", "Width", DEF_GRAPH_WIDTH, 
   Tk_Offset(Graph, reqWidth), 0},
  {BLT_CONFIG_PIXELS, "-plotwidth", "plotWidth", "PlotWidth", 
   (char*)NULL, Tk_Offset(Graph, reqPlotWidth), 
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-plotheight", "plotHeight", "PlotHeight", 
   (char*)NULL, Tk_Offset(Graph, reqPlotHeight), 
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Tcl_IdleProc DisplayGraph;
static Tcl_FreeProc DestroyGraph;
static Tk_EventProc GraphEventProc;
Tcl_ObjCmdProc Blt_GraphInstCmdProc;

static Tcl_ObjCmdProc BarchartObjCmd;
static Tcl_CmdDeleteProc BarchartObjDelete;
static Tcl_ObjCmdProc GraphObjCmd;
static Tcl_CmdDeleteProc GraphObjDelete;
static Tcl_CmdDeleteProc GraphInstCmdDeleteProc;

static Blt_BindPickProc PickEntry;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UpdateGraph --
 *
 *	Tells the Tk dispatcher to call the graph display routine at the next
 *	idle point.  This request is made only if the window is displayed and
 *	no other redraw request is pending.
 *
 *---------------------------------------------------------------------------
 */
void Blt_UpdateGraph(ClientData clientData)
{
  Graph* graphPtr = clientData;

  graphPtr->flags |= REDRAW_WORLD;
  if ((graphPtr->tkwin != NULL) && !(graphPtr->flags & REDRAW_PENDING)) {
    Tcl_DoWhenIdle(DisplayGraph, graphPtr);
    graphPtr->flags |= REDRAW_PENDING;
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_EventuallyRedrawGraph --
 *
 *	Tells the Tk dispatcher to call the graph display routine at the next
 *	idle point.  This request is made only if the window is displayed and
 *	no other redraw request is pending.
 *
 *---------------------------------------------------------------------------
 */
void Blt_EventuallyRedrawGraph(Graph* graphPtr) 
{
  if ((graphPtr->tkwin != NULL) && !(graphPtr->flags & REDRAW_PENDING)) {
    Tcl_DoWhenIdle(DisplayGraph, graphPtr);
    graphPtr->flags |= REDRAW_PENDING;
  }
}

const char* Blt_GraphClassName(ClassId classId) 
{
  if ((classId >= CID_NONE) && (classId <= CID_MARKER_WINDOW)) {
    return objectClassNames[classId];
  }
  return NULL;
}

void Blt_GraphSetObjectClass(GraphObj* graphObjPtr, ClassId classId)
{
  graphObjPtr->classId = classId;
  graphObjPtr->className = Blt_GraphClassName(classId);
}

static void GraphEventProc(ClientData clientData, XEvent* eventPtr)
{
  Graph* graphPtr = clientData;

  if (eventPtr->type == Expose) {
    if (eventPtr->xexpose.count == 0) {
      graphPtr->flags |= REDRAW_WORLD;
      Blt_EventuallyRedrawGraph(graphPtr);
    }
  } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
    if (eventPtr->xfocus.detail != NotifyInferior) {
      if (eventPtr->type == FocusIn) {
	graphPtr->flags |= FOCUS;
      } else {
	graphPtr->flags &= ~FOCUS;
      }
      graphPtr->flags |= REDRAW_WORLD;
      Blt_EventuallyRedrawGraph(graphPtr);
    }
  } else if (eventPtr->type == DestroyNotify) {
    if (graphPtr->tkwin != NULL) {
      Tk_FreeConfigOptions((char*)graphPtr, graphPtr->optionTable, 
			   graphPtr->tkwin);
      graphPtr->tkwin = NULL;
      Tcl_DeleteCommandFromToken(graphPtr->interp, graphPtr->cmdToken);
    }
    if (graphPtr->flags & REDRAW_PENDING) {
      Tcl_CancelIdleCall(DisplayGraph, graphPtr);
    }
    Tcl_EventuallyFree(graphPtr, DestroyGraph);
  } else if (eventPtr->type == ConfigureNotify) {
    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
  }
}

static void GraphInstCmdDeleteProc(ClientData clientData)
{
  Graph* graphPtr = clientData;

  // NULL indicates window has already been destroyed.
  if (graphPtr->tkwin != NULL) {
    Tk_Window tkwin = graphPtr->tkwin;
    graphPtr->tkwin = NULL;
    Tk_DestroyWindow(tkwin);
  }
}

static void AdjustAxisPointers(Graph* graphPtr) 
{
  if (graphPtr->inverted) {
    graphPtr->leftMargin.axes   = graphPtr->axisChain[0];
    graphPtr->bottomMargin.axes = graphPtr->axisChain[1];
    graphPtr->rightMargin.axes  = graphPtr->axisChain[2];
    graphPtr->topMargin.axes    = graphPtr->axisChain[3];
  } else {
    graphPtr->leftMargin.axes   = graphPtr->axisChain[1];
    graphPtr->bottomMargin.axes = graphPtr->axisChain[0];
    graphPtr->rightMargin.axes  = graphPtr->axisChain[3];
    graphPtr->topMargin.axes    = graphPtr->axisChain[2];
  }
}

static int InitPens(Graph* graphPtr)
{
  Tcl_InitHashTable(&graphPtr->penTable, TCL_STRING_KEYS);
  if (Blt_CreatePen(graphPtr, "activeLine", CID_ELEM_LINE, 0, NULL) == NULL) {
    return TCL_ERROR;
  }
  if (Blt_CreatePen(graphPtr, "activeBar", CID_ELEM_BAR, 0, NULL) == NULL) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GraphTags --
 *
 *	Sets the binding tags for a graph obj. This routine is called by Tk
 *	when an event occurs in the graph.  It fills an array of pointers with
 *	bind tag addresses.
 *
 *	The object addresses are strings hashed in one of two tag tables: one
 *	for elements and the another for markers.  Note that there's only one
 *	binding table for elements and markers.  [We don't want to trigger
 *	both a marker and element bind command for the same event.]  But we
 *	don't want a marker and element with the same tag name to activate the
 *	others bindings. A tag "all" for markers should mean all markers, not
 *	all markers and elements.  As a result, element and marker tags are
 *	stored in separate hash tables, which means we can't generate the same
 *	tag address for both an elements and marker, even if they have the
 *	same name.
 *
 *---------------------------------------------------------------------------
 */

void Blt_GraphTags(Blt_BindTable table, ClientData object, ClientData context,
		   Blt_List list)
{
  Graph* graphPtr = (Graph*)Blt_GetBindingData(table);

  /* 
   * All graph objects (markers, elements, axes, etc) have the same starting
   * fields in their structures, such as "classId", "name", "className", and
   * "tags".
   */
  GraphObj* graphObjPtr = (GraphObj*)object;

  MakeTagProc* tagProc;
  switch (graphObjPtr->classId) {
  case CID_ELEM_BAR:		
  case CID_ELEM_LINE: 
    tagProc = Blt_MakeElementTag;
    break;
  case CID_AXIS_X:
  case CID_AXIS_Y:
    tagProc = Blt_MakeAxisTag;
    break;
  case CID_MARKER_BITMAP:
  case CID_MARKER_IMAGE:
  case CID_MARKER_LINE:
  case CID_MARKER_POLYGON:
  case CID_MARKER_TEXT:
  case CID_MARKER_WINDOW:
    tagProc = Blt_MakeMarkerTag;
    break;
  case CID_NONE:
    Blt_Panic("unknown object type");
    tagProc = NULL;
    break;
  default:
    Blt_Panic("bogus object type");
    tagProc = NULL;
    break;
  }
  assert(graphObjPtr->name != NULL);

  /* Always add the name of the object to the tag array. */
  Blt_List_Append(list, (*tagProc)(graphPtr, graphObjPtr->name), 0);
  Blt_List_Append(list, (*tagProc)(graphPtr, graphObjPtr->className), 0);
  if (graphObjPtr->tags != NULL) {
    const char **p;

    for (p = graphObjPtr->tags; *p != NULL; p++) {
      Blt_List_Append(list, (*tagProc) (graphPtr, *p), 0);
    }
  }
}

/*
 *	Find the closest point from the set of displayed elements, searching
 *	the display list from back to front.  That way, if the points from
 *	two different elements overlay each other exactly, the one that's on
 *	top (visible) is picked.
 */

static ClientData PickEntry(ClientData clientData, int x, int y, 
			    ClientData* contextPtr)
{
  Graph* graphPtr = clientData;

  if (graphPtr->flags & MAP_ALL) {
    return NULL;			/* Don't pick anything until the next
					 * redraw occurs. */
  }
  Region2d exts;
  Blt_GraphExtents(graphPtr, &exts);

  if ((x >= exts.right) || (x < exts.left) || 
      (y >= exts.bottom) || (y < exts.top)) {
    /* 
     * Sample coordinate is in one of the graph margins.  Can only pick an
     * axis.
     */
    return Blt_NearestAxis(graphPtr, x, y);
  }
  /* 
   * From top-to-bottom check:
   *	1. markers drawn on top (-under false).
   *	2. elements using its display list back to front.
   *  3. markers drawn under element (-under true).
   */
  Marker* markerPtr = Blt_NearestMarker(graphPtr, x, y, FALSE);
  if (markerPtr != NULL) {
    return markerPtr;		/* Found a marker (-under false). */
  }

  ClosestSearch search;

  search.along = SEARCH_BOTH;
  search.halo = graphPtr->halo;
  search.index = -1;
  search.x = x;
  search.y = y;
  search.dist = (double)(search.halo + 1);
  search.mode = SEARCH_AUTO;
	
  Blt_ChainLink link;
  Element* elemPtr;
  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    elemPtr = Blt_Chain_GetValue(link);
    if (elemPtr->flags & (HIDE|MAP_ITEM)) {
      continue;
    }
    if (elemPtr->state == BLT_STATE_NORMAL) {
      (*elemPtr->procsPtr->closestProc) (graphPtr, elemPtr, &search);
    }
  }
  if (search.dist <= (double)search.halo) {
    return search.elemPtr;// Found an element within the minimum halo distance.
  }

  markerPtr = Blt_NearestMarker(graphPtr, x, y, TRUE);
  if (markerPtr != NULL) {
    return markerPtr;		/* Found a marker (-under true) */
  }
  return NULL;			/* Nothing found. */
}

static void ConfigureGraph(Graph* graphPtr)	
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
  XColor* colorPtr = Blt_BackgroundBorderColor(graphPtr->normalBg);

  graphPtr->titleWidth = graphPtr->titleHeight = 0;
  if (graphPtr->title != NULL) {
    unsigned int w, h;

    Blt_Ts_GetExtents(&graphPtr->titleTextStyle, graphPtr->title, &w, &h);
    graphPtr->titleHeight = h;
  }

  /*
   * Create GCs for interior and exterior regions, and a background GC for
   * clearing the margins with XFillRectangle
   */

  /* Margin GC */
  XGCValues gcValues;
  gcValues.foreground = 
    Blt_Ts_GetForeground(graphPtr->titleTextStyle)->pixel;
  gcValues.background = colorPtr->pixel;
  unsigned long gcMask = (GCForeground | GCBackground);
  GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (graphPtr->drawGC != NULL) {
    Tk_FreeGC(graphPtr->display, graphPtr->drawGC);
  }
  graphPtr->drawGC = newGC;

  if (graphPtr->plotBg != NULL) {
    Blt_SetBackgroundChangedProc(graphPtr->plotBg, Blt_UpdateGraph, 
				 graphPtr);
  }
  if (graphPtr->normalBg != NULL) {
    Blt_SetBackgroundChangedProc(graphPtr->normalBg, Blt_UpdateGraph, 
				 graphPtr);
  }
  if (Blt_ConfigModified(configSpecs, "-invertxy", (char*)NULL)) {

    /*
     * If the -inverted option changed, we need to readjust the pointers
     * to the axes and recompute the their scales.
     */

    AdjustAxisPointers(graphPtr);
    graphPtr->flags |= RESET_AXES;
  }
  if ((!graphPtr->backingStore) && (graphPtr->cache != None)) {
    /*
     * Free the pixmap if we're not buffering the display of elements
     * anymore.
     */
    Tk_FreePixmap(graphPtr->display, graphPtr->cache);
    graphPtr->cache = None;
  }
  /*
   * Reconfigure the crosshairs, just in case the background color of the
   * plotarea has been changed.
   */
  Blt_ConfigureCrosshairs(graphPtr);

  /*
   *  Update the layout of the graph (and redraw the elements) if any of the
   *  following graph options which affect the size of * the plotting area
   *  has changed.
   *
   *	    -aspect
   *      -borderwidth, -plotborderwidth
   *	    -font, -title
   *	    -width, -height
   *	    -invertxy
   *	    -bottommargin, -leftmargin, -rightmargin, -topmargin,
   *	    -barmode, -barwidth
   */
  if (Blt_ConfigModified(configSpecs, "-invertxy", "-title", "-font",
			 "-*margin", "-*width", "-height", "-barmode", "-*pad*", 
			 "-aspect", "-*borderwidth", "-plot*", "-*width", "-*height",
			 "-unmaphiddenelements", (char*)NULL)) {
    graphPtr->flags |= RESET_WORLD | CACHE_DIRTY;
  }
  if (Blt_ConfigModified(configSpecs, "-plot*", "-*background",
			 (char*)NULL)) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  graphPtr->flags |= REDRAW_WORLD;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyGraph --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *	clean up the internal structure of a graph at a safe time (when no-one
 *	is using it anymore).
 *
 *---------------------------------------------------------------------------
 */
static void DestroyGraph(char* dataPtr)
{
  Graph* graphPtr = (Graph*)dataPtr;

  Blt_FreeOptions(configSpecs, (char*)graphPtr, graphPtr->display, 0);
  /*
   * Destroy the individual components of the graph: elements, markers,
   * axes, legend, display lists etc.  Be careful to remove them in
   * order. For example, axes are used by elements and markers, so they have
   * to be removed after the markers and elements. Same it true with the
   * legend and pens (they use elements), so can't be removed until the
   * elements are destroyed.
   */
  Blt_DestroyMarkers(graphPtr);
  Blt_DestroyElements(graphPtr);
  Blt_DestroyLegend(graphPtr);
  Blt_DestroyAxes(graphPtr);
  Blt_DestroyPens(graphPtr);
  Blt_DestroyCrosshairs(graphPtr);
  Blt_DestroyPageSetup(graphPtr);
  Blt_DestroyBarSets(graphPtr);
  if (graphPtr->bindTable != NULL) {
    Blt_DestroyBindingTable(graphPtr->bindTable);
  }

  /* Release allocated X resources and memory. */
  if (graphPtr->drawGC != NULL) {
    Tk_FreeGC(graphPtr->display, graphPtr->drawGC);
  }
  Blt_Ts_FreeStyle(graphPtr->display, &graphPtr->titleTextStyle);
  if (graphPtr->cache != None) {
    Tk_FreePixmap(graphPtr->display, graphPtr->cache);
  }
  free(graphPtr);
}

static int GraphObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			     int objc, Tcl_Obj* const objv[])
{
  Tk_SavedOptions savedOptions;
  int mask, error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)graphPtr, graphPtr->optionTable, 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    //    ConfigureGraph(graphPtr);

    break; // All ok
  }

  if (!error) {
    Tk_FreeSavedOptions(&savedOptions);
    return TCL_OK;
  }
  else {
    Tcl_SetObjResult(interp, errorResult);
    Tcl_DecrRefCount(errorResult);
    return TCL_ERROR;
  }
}

static Graph* CreateGraph(ClientData clientData, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[], ClassId classId)
{
  Tk_OptionTable optionTable = (Tk_OptionTable)clientData;
  if (!optionTable) {
    optionTable = Tk_CreateOptionTable(interp, optionSpecs);
    char* name = Tcl_GetString(objv[0]);
    Tcl_CmdInfo info;
    Tcl_GetCommandInfo(interp, name, &info);
    info.objClientData = (ClientData)optionTable;
    Tcl_SetCommandInfo(interp, name, &info);
  }

  Tk_Window tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
					    Tcl_GetString(objv[1]), 
					    (char*)NULL);
  if (tkwin == NULL)
    return NULL;

  Graph* graphPtr = calloc(1, sizeof(Graph));

  /* Initialize the graph data structure. */

  graphPtr->interp = interp;
  graphPtr->tkwin = tkwin;
  graphPtr->display = Tk_Display(tkwin);
  graphPtr->optionTable = optionTable;
  graphPtr->classId = classId;
  graphPtr->backingStore = TRUE;
  graphPtr->doubleBuffer = TRUE;
  graphPtr->borderWidth = 2;
  graphPtr->plotBW = 1;
  graphPtr->highlightWidth = 2;
  graphPtr->plotRelief = TK_RELIEF_SOLID;
  graphPtr->relief = TK_RELIEF_FLAT;
  graphPtr->flags = RESET_WORLD;
  graphPtr->nextMarkerId = 1;
  graphPtr->bottomMargin.site = MARGIN_BOTTOM;
  graphPtr->leftMargin.site = MARGIN_LEFT;
  graphPtr->topMargin.site = MARGIN_TOP;
  graphPtr->rightMargin.site = MARGIN_RIGHT;

  Blt_Ts_InitStyle(graphPtr->titleTextStyle);
  Blt_Ts_SetAnchor(graphPtr->titleTextStyle, TK_ANCHOR_N);

  Tcl_InitHashTable(&graphPtr->axes.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->axes.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->elements.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->elements.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->markers.table, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->markers.tagTable, TCL_STRING_KEYS);
  Tcl_InitHashTable(&graphPtr->dataTables, TCL_STRING_KEYS);
  graphPtr->elements.displayList = Blt_Chain_Create();
  graphPtr->markers.displayList = Blt_Chain_Create();
  graphPtr->axes.displayList = Blt_Chain_Create();

  switch (classId) {
  case CID_ELEM_LINE:
    Tk_SetClass(tkwin, "Graph");
    break;
  case CID_ELEM_BAR:
    Tk_SetClass(tkwin, "Barchart");
    break;
  default:
    Tk_SetClass(tkwin, "???");
    break;
  }

  ((TkWindow*)tkwin)->instanceData = graphPtr;

  if (InitPens(graphPtr) != TCL_OK)
    goto error;

  /*
  if (Blt_ConfigureWidgetFromObj(interp, tkwin, configSpecs, objc - 2, 
				 objv + 2, (char*)graphPtr, 0) != TCL_OK)
    goto error;
  */
  if (Tk_InitOptions(interp, (char*)graphPtr, optionTable, tkwin) != TCL_OK)
    goto error;
  if (GraphObjConfigure(interp, graphPtr, objc-2, objv+2) != TCL_OK)
    goto error;

  if (Blt_DefaultAxes(graphPtr) != TCL_OK)
    goto error;

  AdjustAxisPointers(graphPtr);

  if (Blt_CreatePageSetup(graphPtr) != TCL_OK)
    goto error;

  if (Blt_CreateCrosshairs(graphPtr) != TCL_OK)
    goto error;

  if (Blt_CreateLegend(graphPtr) != TCL_OK)
    goto error;

  Tk_CreateEventHandler(graphPtr->tkwin, 
			ExposureMask|StructureNotifyMask|FocusChangeMask,
			GraphEventProc, (ClientData)graphPtr);

  graphPtr->cmdToken = Tcl_CreateObjCommand(interp, Tcl_GetString(objv[1]), 
					    Blt_GraphInstCmdProc, 
					    (ClientData)graphPtr, 
					    GraphInstCmdDeleteProc);

  ConfigureGraph(graphPtr);

  graphPtr->bindTable = Blt_CreateBindingTable(interp, tkwin, graphPtr, 
					       PickEntry, Blt_GraphTags);

  Tcl_SetObjResult(interp, objv[1]);
  return graphPtr;

 error:
  DestroyGraph((char*)graphPtr);
  return NULL;
}

/* Widget sub-commands */

static int XAxisOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		   Tcl_Obj* const objv[])
{
  int margin = (graphPtr->inverted) ? MARGIN_LEFT : MARGIN_BOTTOM;
  return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int X2AxisOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		    Tcl_Obj* const objv[])
{
  int margin = (graphPtr->inverted) ? MARGIN_RIGHT : MARGIN_TOP;
  return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int YAxisOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		   Tcl_Obj* const objv[])
{
  int margin = (graphPtr->inverted) ? MARGIN_BOTTOM : MARGIN_LEFT;
  return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int Y2AxisOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		    Tcl_Obj* const objv[])
{
  int margin = (graphPtr->inverted) ? MARGIN_TOP : MARGIN_RIGHT;
  return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int BarOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		 Tcl_Obj* const objv[])
{
  return Blt_ElementOp(graphPtr, interp, objc, objv, CID_ELEM_BAR);
}

static int LineOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		  Tcl_Obj* const objv[])
{
  return Blt_ElementOp(graphPtr, interp, objc, objv, CID_ELEM_LINE);
}

static int ElementOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		     Tcl_Obj* const objv[])
{
  return Blt_ElementOp(graphPtr, interp, objc, objv, graphPtr->classId);
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp, int objc,
		       Tcl_Obj* const objv[])
{
  int flags;

  flags = BLT_CONFIG_OBJV_ONLY;
  if (objc == 2) {
    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
				    (char*)graphPtr, (Tcl_Obj*)NULL, flags);
  } else if (objc == 3) {
    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
				    (char*)graphPtr, objv[2], flags);
  } else {
    //    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, objc - 2, objv + 2, (char*)graphPtr, flags) != TCL_OK)
    if (GraphObjConfigure(interp, graphPtr, objc-2, objv+2) != TCL_OK)
      return TCL_ERROR;

    ConfigureGraph(graphPtr);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
  }
}

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		  Tcl_Obj* const objv[])
{
  return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
				   (char*)graphPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtentsOp --
 *
 *	Reports the size of one of several items within the graph.  The
 *	following are valid items:
 *
 *	  "bottommargin"	Height of the bottom margin
 *	  "leftmargin"		Width of the left margin
 *	  "legend"		x y w h of the legend
 *	  "plotarea"		x y w h of the plotarea
 *	  "plotheight"		Height of the plot area
 *	  "rightmargin"		Width of the right margin
 *	  "topmargin"		Height of the top margin
 *        "plotwidth"		Width of the plot area
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */

static int ExtentsOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		     Tcl_Obj* const objv[])
{
  int length;
  const char* string = Tcl_GetStringFromObj(objv[2], &length);
  char c = string[0];
  if ((c == 'p') && (length > 4) && 
      (strncmp("plotheight", string, length) == 0)) {
    int height;

    height = graphPtr->bottom - graphPtr->top + 1;
    Tcl_SetIntObj(Tcl_GetObjResult(interp), height);
  } else if ((c == 'p') && (length > 4) &&
	     (strncmp("plotwidth", string, length) == 0)) {
    int width;

    width = graphPtr->right - graphPtr->left + 1;
    Tcl_SetIntObj(Tcl_GetObjResult(interp), width);
  } else if ((c == 'p') && (length > 4) &&
	     (strncmp("plotarea", string, length) == 0)) {
    Tcl_Obj* listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->left));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->top));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->right - graphPtr->left + 1));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->bottom - graphPtr->top + 1));
    Tcl_SetObjResult(interp, listObjPtr);
  } else if ((c == 'l') && (length > 2) &&
	     (strncmp("legend", string, length) == 0)) {
    Tcl_Obj* listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(Blt_Legend_X(graphPtr)));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(Blt_Legend_Y(graphPtr)));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(Blt_Legend_Width(graphPtr)));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(Blt_Legend_Height(graphPtr)));
    Tcl_SetObjResult(interp, listObjPtr);
  } else if ((c == 'l') && (length > 2) &&
	     (strncmp("leftmargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), graphPtr->leftMargin.width);
  } else if ((c == 'r') && (length > 1) &&
	     (strncmp("rightmargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), graphPtr->rightMargin.width);
  } else if ((c == 't') && (length > 1) &&
	     (strncmp("topmargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), graphPtr->topMargin.height);
  } else if ((c == 'b') && (length > 1) &&
	     (strncmp("bottommargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), graphPtr->bottomMargin.height);
  } else {
    Tcl_AppendResult(interp, "bad extent item \"", objv[2],
		     "\": should be plotheight, plotwidth, leftmargin, rightmargin, \
topmargin, bottommargin, plotarea, or legend", (char*)NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int InsideOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		    Tcl_Obj* const objv[])
{
  int x, y;
  if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK) {
    return TCL_ERROR;
  }
  Region2d exts;
  Blt_GraphExtents(graphPtr, &exts);
  int result = PointInRegion(&exts, x, y);
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), result);

  return TCL_OK;
}

static int InvtransformOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			  Tcl_Obj* const objv[])
{
  double x, y;
  if ((Blt_ExprDoubleFromObj(interp, objv[2], &x) != TCL_OK) ||
      (Blt_ExprDoubleFromObj(interp, objv[3], &y) != TCL_OK)) {
    return TCL_ERROR;
  }
  if (graphPtr->flags & RESET_AXES) {
    Blt_ResetAxes(graphPtr);
  }
  /* Perform the reverse transformation, converting from window coordinates
   * to graph data coordinates.  Note that the point is always mapped to the
   * bottom and left axes (which may not be what the user wants).  */

  /*  Pick the first pair of axes */
  Axis2d axes;
  axes.x = Blt_GetFirstAxis(graphPtr->axisChain[0]);
  axes.y = Blt_GetFirstAxis(graphPtr->axisChain[1]);
  Point2d point = Blt_InvMap2D(graphPtr, x, y, &axes);

  Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(point.x));
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(point.y));
  Tcl_SetObjResult(interp, listObjPtr);

  return TCL_OK;
}

static int TransformOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		       Tcl_Obj* const objv[])
{
  double x, y;
  if ((Blt_ExprDoubleFromObj(interp, objv[2], &x) != TCL_OK) ||
      (Blt_ExprDoubleFromObj(interp, objv[3], &y) != TCL_OK)) {
    return TCL_ERROR;
  }
  if (graphPtr->flags & RESET_AXES) {
    Blt_ResetAxes(graphPtr);
  }
  /*
   * Perform the transformation from window to graph coordinates.  Note that
   * the points are always mapped onto the bottom and left axes (which may
   * not be the what the user wants).
   */
  Axis2d axes;
  axes.x = Blt_GetFirstAxis(graphPtr->axisChain[0]);
  axes.y = Blt_GetFirstAxis(graphPtr->axisChain[1]);

  Point2d point = Blt_Map2D(graphPtr, x, y, &axes);

  Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(ROUND(point.x)));
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(ROUND(point.y)));
  Tcl_SetObjResult(interp, listObjPtr);

  return TCL_OK;
}

static Blt_OpSpec graphOps[] =
  {
    {"axis",         1, Blt_VirtualAxisOp, 2, 0, "oper ?args?",},
    {"bar",          2, BarOp,             2, 0, "oper ?args?",},
    {"cget",         2, CgetOp,            3, 3, "option",},
    {"configure",    2, ConfigureOp,       2, 0, "?option value?...",},
    {"crosshairs",   2, Blt_CrosshairsOp,  2, 0, "oper ?args?",},
    {"element",      2, ElementOp,         2, 0, "oper ?args?",},
    {"extents",      2, ExtentsOp,         3, 3, "item",},
    {"inside",       3, InsideOp,          4, 4, "winX winY",},
    {"invtransform", 3, InvtransformOp,    4, 4, "winX winY",},
    {"legend",       2, Blt_LegendOp,      2, 0, "oper ?args?",},
    {"line",         2, LineOp,            2, 0, "oper ?args?",},
    {"marker",       2, Blt_MarkerOp,      2, 0, "oper ?args?",},
    {"pen",          2, Blt_PenOp,         2, 0, "oper ?args?",},
    {"postscript",   2, Blt_PostScriptOp,  2, 0, "oper ?args?",},
    {"transform",    1, TransformOp,       4, 4, "x y",},
    {"x2axis",       2, X2AxisOp,          2, 0, "oper ?args?",},
    {"xaxis",        2, XAxisOp,           2, 0, "oper ?args?",},
    {"y2axis",       2, Y2AxisOp,          2, 0, "oper ?args?",},
    {"yaxis",        2, YAxisOp,           2, 0, "oper ?args?",},
  };
static int nGraphOps = sizeof(graphOps) / sizeof(Blt_OpSpec);

int Blt_GraphInstCmdProc(ClientData clientData, Tcl_Interp* interp, int objc,
		     Tcl_Obj* const objv[])
{
  Graph* graphPtr = clientData;
  GraphCmdProc* proc = Blt_GetOpFromObj(interp, nGraphOps, graphOps, 
					BLT_OP_ARG1, objc, objv, 0);
  if (proc == NULL) {
    return TCL_ERROR;
  }
  Tcl_Preserve(graphPtr);
  int result = (*proc) (graphPtr, interp, objc, objv);
  Tcl_Release(graphPtr);
  return result;
}

static int NewGraph(ClientData clientData, Tcl_Interp*interp, 
		    int objc, Tcl_Obj* const objv[], ClassId classId)
{
  if (objc < 2) {
    Tcl_AppendResult(interp, "wrong # args: should be \"", 
		     Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		     (char*)NULL);
    return TCL_ERROR;
  }

  if (!CreateGraph(clientData, interp, objc, objv, classId))
    return TCL_ERROR;

  return TCL_OK;
}

static void DeleteGraph(ClientData clientData)
{
  Tk_OptionTable optionTable = (Tk_OptionTable)clientData;
  if (clientData)
    Tk_DeleteOptionTable(optionTable);
}

static int GraphObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, 
		       Tcl_Obj* const objv[])
{
  return NewGraph(clientData, interp, objc, objv, CID_ELEM_LINE);
}

static void GraphObjDelete(ClientData clientData)
{
  DeleteGraph(clientData);
}

static int BarchartObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, 
			  Tcl_Obj* const objv[])
{
  return NewGraph(clientData, interp, objc, objv, CID_ELEM_BAR);
}

static void BarchartObjDelete(ClientData clientData)
{
  DeleteGraph(clientData);
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

  Blt_FillBackgroundRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
			      rects[0].x, rects[0].y, rects[0].width, rects[0].height, 
			      0, TK_RELIEF_FLAT);
  Blt_FillBackgroundRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
			      rects[1].x, rects[1].y, rects[1].width, rects[1].height, 
			      0, TK_RELIEF_FLAT);
  Blt_FillBackgroundRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
			      rects[2].x, rects[2].y, rects[2].width, rects[2].height, 
			      0, TK_RELIEF_FLAT);
  Blt_FillBackgroundRectangle(graphPtr->tkwin, drawable, graphPtr->normalBg, 
			      rects[3].x, rects[3].y, rects[3].width, rects[3].height, 
			      0, TK_RELIEF_FLAT);

  /* Draw 3D border around the plotting area */

  if (graphPtr->plotBW > 0) {
    int x, y, w, h;

    x = graphPtr->left - graphPtr->plotBW;
    y = graphPtr->top - graphPtr->plotBW;
    w = (graphPtr->right - graphPtr->left) + (2*graphPtr->plotBW);
    h = (graphPtr->bottom - graphPtr->top) + (2*graphPtr->plotBW);
    Blt_DrawBackgroundRectangle(graphPtr->tkwin, drawable, 
				graphPtr->normalBg, x, y, w, h, graphPtr->plotBW, 
				graphPtr->plotRelief);
  }
  int site = Blt_Legend_Site(graphPtr);
  if (site & LEGEND_MARGIN_MASK) {
    /* Legend is drawn on one of the graph margins */
    Blt_DrawLegend(graphPtr, drawable);
  } else if (site == LEGEND_WINDOW) {
    Blt_Legend_EventuallyRedraw(graphPtr);
  }
  if (graphPtr->title != NULL) {
    Blt_DrawText(graphPtr->tkwin, drawable, graphPtr->title,
		 &graphPtr->titleTextStyle, graphPtr->titleX, graphPtr->titleY);
  }
  Blt_DrawAxes(graphPtr, drawable);
  graphPtr->flags &= ~DRAW_MARGINS;
}

static void DrawPlot(Graph* graphPtr, Drawable drawable)
{
  DrawMargins(graphPtr, drawable);

  /* Draw the background of the plotting area with 3D border. */
  Blt_FillBackgroundRectangle(graphPtr->tkwin, drawable, graphPtr->plotBg,
			      graphPtr->left   - graphPtr->plotBW, 
			      graphPtr->top    - graphPtr->plotBW, 
			      graphPtr->right  - graphPtr->left + 1 + 2 * graphPtr->plotBW,
			      graphPtr->bottom - graphPtr->top  + 1 + 2 * graphPtr->plotBW, 
			      graphPtr->plotBW, graphPtr->plotRelief);

  /* Draw the elements, markers, legend, and axis limits. */
  Blt_DrawAxes(graphPtr, drawable);
  Blt_DrawGrids(graphPtr, drawable);
  Blt_DrawMarkers(graphPtr, drawable, MARKER_UNDER);

  int site = Blt_Legend_Site(graphPtr);
  if ((site & LEGEND_PLOTAREA_MASK) && (!Blt_Legend_IsRaised(graphPtr))) {
    Blt_DrawLegend(graphPtr, drawable);
  } else if (site == LEGEND_WINDOW) {
    Blt_Legend_EventuallyRedraw(graphPtr);
  }
  Blt_DrawAxisLimits(graphPtr, drawable);
  Blt_DrawElements(graphPtr, drawable);
  /* Blt_DrawAxes(graphPtr, drawable); */
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
  /* Compute coordinate transformations for graph components */
  if ((graphPtr->vRange > 1) && (graphPtr->hRange > 1)) {
    if (graphPtr->flags & MAP_WORLD) {
      Blt_MapAxes(graphPtr);
    }
    Blt_MapElements(graphPtr);
    Blt_MapMarkers(graphPtr);
    graphPtr->flags &= ~(MAP_ALL);
  }
}

void Blt_DrawGraph(Graph* graphPtr, Drawable drawable)
{
  DrawPlot(graphPtr, drawable);
  /* Draw markers above elements */
  Blt_DrawMarkers(graphPtr, drawable, MARKER_ABOVE);
  Blt_DrawActiveElements(graphPtr, drawable);

  /* Don't draw legend in the plot area. */
  if ((Blt_Legend_Site(graphPtr) & LEGEND_PLOTAREA_MASK) && 
      (Blt_Legend_IsRaised(graphPtr))) {
    Blt_DrawLegend(graphPtr, drawable);
  }
  /* Draw 3D border just inside of the focus highlight ring. */
  if ((graphPtr->borderWidth > 0) && (graphPtr->relief != TK_RELIEF_FLAT)) {
    Blt_DrawBackgroundRectangle(graphPtr->tkwin, drawable, 
				graphPtr->normalBg, graphPtr->highlightWidth, 
				graphPtr->highlightWidth, 
				graphPtr->width  - 2 * graphPtr->highlightWidth, 
				graphPtr->height - 2 * graphPtr->highlightWidth, 
				graphPtr->borderWidth, graphPtr->relief);
  }
  /* Draw focus highlight ring. */
  if ((graphPtr->highlightWidth > 0) && (graphPtr->flags & FOCUS)) {
    GC gc;

    gc = Tk_GCForColor(graphPtr->highlightColor, drawable);
    Tk_DrawFocusHighlight(graphPtr->tkwin, gc, graphPtr->highlightWidth,
			  drawable);
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

static void DisplayGraph(ClientData clientData)
{
  Graph* graphPtr = clientData;
  Pixmap drawable;
  Tk_Window tkwin;
  int site;

  graphPtr->flags &= ~REDRAW_PENDING;
  if (graphPtr->tkwin == NULL) {
    return;				/* Window has been destroyed (we
					 * should not get here) */
  }
  tkwin = graphPtr->tkwin;
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
  if (graphPtr->doubleBuffer) {
    drawable = Tk_GetPixmap(graphPtr->display, Tk_WindowId(tkwin), 
			    graphPtr->width, graphPtr->height, Tk_Depth(tkwin));
  } else {
    drawable = Tk_WindowId(tkwin);
  }
  if (graphPtr->backingStore) {
    if ((graphPtr->cache == None) || 
	(graphPtr->cacheWidth != graphPtr->width) ||
	(graphPtr->cacheHeight != graphPtr->height)) {
      if (graphPtr->cache != None) {
	Tk_FreePixmap(graphPtr->display, graphPtr->cache);
      }


      graphPtr->cache = Tk_GetPixmap(graphPtr->display, 
				     Tk_WindowId(tkwin), graphPtr->width, graphPtr->height, 
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
  } else {
    DrawPlot(graphPtr, drawable);
  }
  /* Draw markers above elements */
  Blt_DrawMarkers(graphPtr, drawable, MARKER_ABOVE);
  Blt_DrawActiveElements(graphPtr, drawable);
  /* Don't draw legend in the plot area. */
  site = Blt_Legend_Site(graphPtr);
  if ((site & LEGEND_PLOTAREA_MASK) && (Blt_Legend_IsRaised(graphPtr))) {
    Blt_DrawLegend(graphPtr, drawable);
  }
  if (site == LEGEND_WINDOW) {
    Blt_Legend_EventuallyRedraw(graphPtr);
  }
  /* Draw 3D border just inside of the focus highlight ring. */
  if ((graphPtr->borderWidth > 0) && (graphPtr->relief != TK_RELIEF_FLAT)) {
    Blt_DrawBackgroundRectangle(graphPtr->tkwin, drawable, 
				graphPtr->normalBg, graphPtr->highlightWidth, 
				graphPtr->highlightWidth, 
				graphPtr->width - 2 * graphPtr->highlightWidth, 
				graphPtr->height - 2 * graphPtr->highlightWidth, 
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
  graphPtr->flags &= ~RESET_WORLD;
  UpdateMarginTraces(graphPtr);
}

int Blt_GraphCmdInitProc(Tcl_Interp* interp)
{
  static Blt_InitCmdSpec graphSpec = 
    {"graph", GraphObjCmd, GraphObjDelete, NULL};
  static Blt_InitCmdSpec barchartSpec = 
    {"barchart", BarchartObjCmd, BarchartObjDelete, NULL};

  if (Blt_InitCmd(interp, "::blt", &graphSpec) != TCL_OK)
    return TCL_ERROR;
  if (Blt_InitCmd(interp, "::blt", &barchartSpec) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}

Graph* Blt_GetGraphFromWindowData(Tk_Window tkwin)
{
  while (tkwin) {
    TkWindow* winPtr = (TkWindow*)tkwin;
    if (winPtr->instanceData != NULL) {
      Graph* graphPtr = (ClientData)winPtr->instanceData;
      if (graphPtr)
	return graphPtr;
    }
    tkwin = Tk_Parent(tkwin);
  }
  return NULL;
}

int Blt_GraphType(Graph* graphPtr)
{
  switch (graphPtr->classId) {
  case CID_ELEM_LINE:
    return GRAPH;
  case CID_ELEM_BAR:
    return BARCHART;
  default:
    return 0;
  }

  return 0;
}

void Blt_ReconfigureGraph(Graph* graphPtr)	
{
  ConfigureGraph(graphPtr);
  Blt_ConfigureLegend(graphPtr);
  Blt_ConfigureElements(graphPtr);
  Blt_ConfigureAxes(graphPtr);
  Blt_ConfigureMarkers(graphPtr);
}
