
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

/*
 * To do:
 *
 * 2) Update manual pages.
 *
 * 3) Update comments.
 *
 * 5) Surface, contour, and flow graphs
 *
 * 7) Arrows for line markers
 *
 */

#include "bltGraph.h"
#include "bltOp.h"
#include "bltBind.h"
#include "bltGrElem.h"
#include "bltGrLegd.h"
#include "bltSwitch.h"
#include <X11/Xutil.h>
#include "tkDisplay.h"
#include "bltPicture.h"

typedef int (GraphCmdProc)(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

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
static const char *objectClassNames[] = {
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

BLT_EXTERN Blt_CustomOption bltLinePenOption;
BLT_EXTERN Blt_CustomOption bltBarPenOption;
BLT_EXTERN Blt_CustomOption bltBarModeOption;

#define DEF_GRAPH_ASPECT_RATIO		"0.0"
#define DEF_GRAPH_BAR_BASELINE		"0.0"
#define DEF_GRAPH_BAR_MODE		"normal"
#define DEF_GRAPH_BAR_WIDTH		"0.9"
#define DEF_GRAPH_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_GRAPH_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_GRAPH_BUFFER_ELEMENTS	"yes"
#define DEF_GRAPH_BUFFER_GRAPH		"1"
#define DEF_GRAPH_CURSOR		"crosshair"
#define DEF_GRAPH_FONT			"{Sans Serif} 12"
#define DEF_GRAPH_HALO			"2m"
#define DEF_GRAPH_HALO_BAR		"0.1i"
#define DEF_GRAPH_HEIGHT		"4i"
#define DEF_GRAPH_HIGHLIGHT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_GRAPH_HIGHLIGHT_COLOR	RGB_BLACK
#define DEF_GRAPH_HIGHLIGHT_WIDTH	"2"
#define DEF_GRAPH_INVERT_XY		"0"
#define DEF_GRAPH_JUSTIFY		"center"
#define DEF_GRAPH_MARGIN		"0"
#define DEF_GRAPH_MARGIN_VAR		(char *)NULL
#define DEF_GRAPH_PLOT_BACKGROUND	RGB_WHITE
#define DEF_GRAPH_PLOT_BORDERWIDTH	"1"
#define DEF_GRAPH_PLOT_PADX		"0"
#define DEF_GRAPH_PLOT_PADY		"0"
#define DEF_GRAPH_PLOT_RELIEF		"solid"
#define DEF_GRAPH_RELIEF		"flat"
#define DEF_GRAPH_SHOW_VALUES		"no"
#define DEF_GRAPH_STACK_AXES		"no"
#define DEF_GRAPH_TAKE_FOCUS		""
#define DEF_GRAPH_TITLE			(char *)NULL
#define DEF_GRAPH_TITLE_COLOR		STD_NORMAL_FOREGROUND
#define DEF_GRAPH_WIDTH			"5i"
#define DEF_GRAPH_DATA			(char *)NULL
#define DEF_GRAPH_DATA_COMMAND		(char *)NULL
#define DEF_GRAPH_UNMAP_HIDDEN_ELEMENTS	"0"

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_FLOAT, "-aspect", "aspect", "Aspect", DEF_GRAPH_ASPECT_RATIO, 
	Blt_Offset(Graph, aspect), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_GRAPH_BACKGROUND, Blt_Offset(Graph, normalBg), 0},
    {BLT_CONFIG_CUSTOM, "-barmode", "barMode", "BarMode", DEF_GRAPH_BAR_MODE, 
	Blt_Offset(Graph, mode), BLT_CONFIG_DONT_SET_DEFAULT, 
	&bltBarModeOption},
    {BLT_CONFIG_FLOAT, "-barwidth", "barWidth", "BarWidth", 
	DEF_GRAPH_BAR_WIDTH, Blt_Offset(Graph, barWidth), 0},
    {BLT_CONFIG_FLOAT, "-baseline", "baseline", "Baseline",
	DEF_GRAPH_BAR_BASELINE, Blt_Offset(Graph, baseline), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL,0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-bm", "bottomMargin", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_GRAPH_BORDERWIDTH, Blt_Offset(Graph, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-bottommargin", "bottomMargin", "Margin",
	DEF_GRAPH_MARGIN, Blt_Offset(Graph, bottomMargin.reqSize), 0},
    {BLT_CONFIG_STRING, "-bottomvariable", "bottomVariable", "BottomVariable",
	DEF_GRAPH_MARGIN_VAR, Blt_Offset(Graph, bottomMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-bufferelements", "bufferElements", "BufferElements",
	DEF_GRAPH_BUFFER_ELEMENTS, Blt_Offset(Graph, backingStore),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-buffergraph", "bufferGraph", "BufferGraph",
	DEF_GRAPH_BUFFER_GRAPH, Blt_Offset(Graph, doubleBuffer),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_GRAPH_CURSOR, Blt_Offset(Graph, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-data", "data", "Data", 
        (char *)NULL, Blt_Offset(Graph, data), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-datacommand", "dataCommand", "DataCommand", 
        (char *)NULL, Blt_Offset(Graph, dataCmd), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font",
	DEF_GRAPH_FONT, Blt_Offset(Graph, titleTextStyle.font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_GRAPH_TITLE_COLOR, Blt_Offset(Graph, titleTextStyle.color), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-halo", "halo", "Halo", DEF_GRAPH_HALO, 
	Blt_Offset(Graph, halo), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_GRAPH_HEIGHT, 
	Blt_Offset(Graph, reqHeight), 0},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_GRAPH_HIGHLIGHT_BACKGROUND, 
	Blt_Offset(Graph, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_GRAPH_HIGHLIGHT_COLOR, Blt_Offset(Graph, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_GRAPH_HIGHLIGHT_WIDTH, 
	Blt_Offset(Graph, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-unmaphiddenelements", "unmapHiddenElements", 
	"UnmapHiddenElements", DEF_GRAPH_UNMAP_HIDDEN_ELEMENTS, 
	Blt_Offset(Graph, flags), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)UNMAP_HIDDEN},
    {BLT_CONFIG_BOOLEAN, "-invertxy", "invertXY", "InvertXY", 
	DEF_GRAPH_INVERT_XY, Blt_Offset(Graph, inverted),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_GRAPH_JUSTIFY, 
	Blt_Offset(Graph, titleTextStyle.justify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-leftmargin", "leftMargin", "Margin", 
	DEF_GRAPH_MARGIN, Blt_Offset(Graph, leftMargin.reqSize), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-leftvariable", "leftVariable", "LeftVariable",
	DEF_GRAPH_MARGIN_VAR, Blt_Offset(Graph, leftMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-lm", "leftMargin", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-plotbackground", "plotBackground", "Background",
	DEF_GRAPH_PLOT_BACKGROUND, Blt_Offset(Graph, plotBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-plotborderwidth", "plotBorderWidth", 
        "PlotBorderWidth", DEF_GRAPH_PLOT_BORDERWIDTH, 
	Blt_Offset(Graph, plotBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-plotpadx", "plotPadX", "PlotPad", DEF_GRAPH_PLOT_PADX, 
	Blt_Offset(Graph, xPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-plotpady", "plotPadY", "PlotPad", DEF_GRAPH_PLOT_PADY, 
	Blt_Offset(Graph, yPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-plotrelief", "plotRelief", "Relief", 
	DEF_GRAPH_PLOT_RELIEF, Blt_Offset(Graph, plotRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_GRAPH_RELIEF, 
	Blt_Offset(Graph, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-rightmargin", "rightMargin", "Margin",
	DEF_GRAPH_MARGIN, Blt_Offset(Graph, rightMargin.reqSize),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-rightvariable", "rightVariable", "RightVariable",
	DEF_GRAPH_MARGIN_VAR, Blt_Offset(Graph, rightMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-rm", "rightMargin", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_BOOLEAN, "-stackaxes", "stackAxes", "StackAxes", 
	DEF_GRAPH_STACK_AXES, Blt_Offset(Graph, stackAxes),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_GRAPH_TAKE_FOCUS, Blt_Offset(Graph, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-title", "title", "Title", DEF_GRAPH_TITLE, 
	Blt_Offset(Graph, title), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-tm", "topMargin", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-topmargin", "topMargin", "Margin", 
	DEF_GRAPH_MARGIN, Blt_Offset(Graph, topMargin.reqSize), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-topvariable", "topVariable", "TopVariable",
	DEF_GRAPH_MARGIN_VAR, Blt_Offset(Graph, topMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_GRAPH_WIDTH, 
	Blt_Offset(Graph, reqWidth), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-plotwidth", "plotWidth", "PlotWidth", 
	(char *)NULL, Blt_Offset(Graph, reqPlotWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-plotheight", "plotHeight", "PlotHeight", 
	(char *)NULL, Blt_Offset(Graph, reqPlotHeight), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_SwitchParseProc ObjToFormat;
static Blt_SwitchCustom formatSwitch =
{
    ObjToFormat, NULL, (ClientData)0,
};

typedef struct {
    const char *name;
    int width, height;
    int format;
} SnapSwitches;

enum SnapFormats { FORMAT_PICTURE, FORMAT_PHOTO, FORMAT_EMF, FORMAT_WMF };

static Blt_SwitchSpec snapSwitches[] = 
{
    {BLT_SWITCH_INT_POS, "-width",  "width",
	Blt_Offset(SnapSwitches, width),  0},
    {BLT_SWITCH_INT_POS, "-height", "height",
	Blt_Offset(SnapSwitches, height), 0},
    {BLT_SWITCH_CUSTOM,  "-format", "format",
	Blt_Offset(SnapSwitches, format), 0, 0, &formatSwitch},
    {BLT_SWITCH_END}
};

static Tcl_IdleProc DisplayGraph;
static Tcl_FreeProc DestroyGraph;
static Tk_EventProc GraphEventProc;
Tcl_ObjCmdProc Blt_GraphInstCmdProc;

static Blt_BindPickProc PickEntry;
static Tcl_ObjCmdProc StripchartCmd;
static Tcl_ObjCmdProc BarchartCmd;
static Tcl_ObjCmdProc GraphCmd;
static Tcl_CmdDeleteProc GraphInstCmdDeleteProc;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UpdateGraph --
 *
 *	Tells the Tk dispatcher to call the graph display routine at the next
 *	idle point.  This request is made only if the window is displayed and
 *	no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *	The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UpdateGraph(ClientData clientData)
{
    Graph *graphPtr = clientData;

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
 * Results: None.
 *
 * Side effects:
 *	The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_EventuallyRedrawGraph(Graph *graphPtr) 
{
    if ((graphPtr->tkwin != NULL) && !(graphPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayGraph, graphPtr);
	graphPtr->flags |= REDRAW_PENDING;
    }
}

const char *
Blt_GraphClassName(ClassId classId) 
{
    if ((classId >= CID_NONE) && (classId <= CID_MARKER_WINDOW)) {
	return objectClassNames[classId];
    }
    return NULL;
}

void
Blt_GraphSetObjectClass(GraphObj *graphObjPtr, ClassId classId)
{
    graphObjPtr->classId = classId;
    graphObjPtr->className = Blt_GraphClassName(classId);
}

/*
 *---------------------------------------------------------------------------
 *
 * GraphEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events on
 *	graphs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, the graph is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
GraphEventProc(ClientData clientData, XEvent *eventPtr)
{
    Graph *graphPtr = clientData;

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
	    Blt_DeleteWindowInstanceData(graphPtr->tkwin);
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

/*
 *---------------------------------------------------------------------------
 *
 * GraphInstCmdDeleteProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
GraphInstCmdDeleteProc(ClientData clientData) /* Pointer to widget record. */
{
    Graph *graphPtr = clientData;

    if (graphPtr->tkwin != NULL) {	/* NULL indicates window has already
					 * been destroyed. */
	Tk_Window tkwin;

	tkwin = graphPtr->tkwin;
	graphPtr->tkwin = NULL;
	Blt_DeleteWindowInstanceData(tkwin);
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AdjustAxisPointers --
 *
 *	Sets the axis pointers according to whether the axis is inverted on
 *	not.  The axis sites are also reset.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
AdjustAxisPointers(Graph *graphPtr) 
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

static int
InitPens(Graph *graphPtr)
{
    Blt_InitHashTable(&graphPtr->penTable, BLT_STRING_KEYS);
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
 * Results:
 *	None.
 *
 * Side effects:
 *	This information will be used by the binding code in bltUtil.c to
 *	determine what graph objects match the current event.  The tags are
 *	placed in tagArr and *nTagsPtr is set with the number of tags found.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
void
Blt_GraphTags(
    Blt_BindTable table,
    ClientData object,
    ClientData context,		/* Not used. */
    Blt_List list)
{
    GraphObj *graphObjPtr;
    MakeTagProc *tagProc;
    Graph *graphPtr;

    graphPtr = (Graph *)Blt_GetBindingData(table);

    /* 
     * All graph objects (markers, elements, axes, etc) have the same starting
     * fields in their structures, such as "classId", "name", "className", and
     * "tags".
     */
    graphObjPtr = (GraphObj *)object;

    switch (graphObjPtr->classId) {
    case CID_ELEM_BAR:		
    case CID_ELEM_CONTOUR:
    case CID_ELEM_LINE: 
    case CID_ELEM_STRIP: 
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
	panic("unknown object type");
	tagProc = NULL;
	break;
    default:
	panic("bogus object type");
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
/*ARGSUSED*/
static ClientData
PickEntry(ClientData clientData, int x, int y, ClientData *contextPtr)
{
    Graph *graphPtr = clientData;
    Blt_ChainLink link;
    Element *elemPtr;
    Marker *markerPtr;
    Region2d exts;

    if (graphPtr->flags & MAP_ALL) {
	return NULL;			/* Don't pick anything until the next
					 * redraw occurs. */
    }
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
    markerPtr = Blt_NearestMarker(graphPtr, x, y, FALSE);
    if (markerPtr != NULL) {
	return markerPtr;		/* Found a marker (-under false). */
    }
    {
	ClosestSearch search;

	search.along = SEARCH_BOTH;
	search.halo = graphPtr->halo;
	search.index = -1;
	search.x = x;
	search.y = y;
	search.dist = (double)(search.halo + 1);
	search.mode = SEARCH_AUTO;
	
	for (link = Blt_Chain_LastLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_PrevLink(link)) {
	    elemPtr = Blt_Chain_GetValue(link);
	    if (elemPtr->flags & (HIDE|MAP_ITEM)) {
		continue;
	    }
	    if (elemPtr->state == STATE_NORMAL) {
		(*elemPtr->procsPtr->closestProc) (graphPtr, elemPtr, &search);
	    }
	}
	if (search.dist <= (double)search.halo) {
	    return search.elemPtr;	/* Found an element within the minimum
					 * halo distance. */
	}
    }
    markerPtr = Blt_NearestMarker(graphPtr, x, y, TRUE);
    if (markerPtr != NULL) {
	return markerPtr;		/* Found a marker (-under true) */
    }
    return NULL;			/* Nothing found. */
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureGraph --
 *
 *	Allocates resources for the graph.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for graphPtr; old resources get freed, if there were any.  The
 *	graph is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
ConfigureGraph(Graph *graphPtr)	
{
    XColor *colorPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    /* Don't allow negative bar widths. Reset to an arbitrary value (0.1) */
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
    colorPtr = Blt_BackgroundBorderColor(graphPtr->normalBg);

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

    gcValues.foreground = 
	Blt_Ts_GetForeground(graphPtr->titleTextStyle)->pixel;
    gcValues.background = colorPtr->pixel;
    gcMask = (GCForeground | GCBackground);
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
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
    if (Blt_ConfigModified(configSpecs, "-invertxy", (char *)NULL)) {

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
		"-unmaphiddenelements", (char *)NULL)) {
	graphPtr->flags |= RESET_WORLD | CACHE_DIRTY;
    }
    if (Blt_ConfigModified(configSpecs, "-plot*", "-*background",
			   (char *)NULL)) {
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
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyGraph(DestroyData dataPtr)
{
    Graph *graphPtr = (Graph *)dataPtr;

    Blt_FreeOptions(configSpecs, (char *)graphPtr, graphPtr->display, 0);
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
    /* Destroy table clients after elements are destroyed. */
    Blt_DestroyTableClients(graphPtr);
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
    Blt_Free(graphPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateGraph --
 *
 *	This procedure creates and initializes a new widget.
 *
 * Results:
 *	The return value is a pointer to a structure describing the new
 *	widget.  If an error occurred, then the return value is NULL and an
 *	error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated, a Tk_Window is created, etc.
 *
 *---------------------------------------------------------------------------
 */

static Graph *
CreateGraph(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, ClassId classId)
{
    Graph *graphPtr;
    Tk_Window tkwin;

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
	Tcl_GetString(objv[1]), (char *)NULL);
    if (tkwin == NULL) {
	return NULL;
    }
    graphPtr = Blt_AssertCalloc(1, sizeof(Graph));

    /* Initialize the graph data structure. */

    graphPtr->tkwin = tkwin;
    graphPtr->display = Tk_Display(tkwin);
    graphPtr->interp = interp;
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
    graphPtr->padLeft = graphPtr->padRight = 0;
    graphPtr->padTop = graphPtr->padBottom = 0;
    graphPtr->bottomMargin.site = MARGIN_BOTTOM;
    graphPtr->leftMargin.site = MARGIN_LEFT;
    graphPtr->topMargin.site = MARGIN_TOP;
    graphPtr->rightMargin.site = MARGIN_RIGHT;
    Blt_Ts_InitStyle(graphPtr->titleTextStyle);
    Blt_Ts_SetAnchor(graphPtr->titleTextStyle, TK_ANCHOR_N);

    Blt_InitHashTable(&graphPtr->axes.table, BLT_STRING_KEYS);
    Blt_InitHashTable(&graphPtr->axes.tagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&graphPtr->elements.table, BLT_STRING_KEYS);
    Blt_InitHashTable(&graphPtr->elements.tagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&graphPtr->markers.table, BLT_STRING_KEYS);
    Blt_InitHashTable(&graphPtr->markers.tagTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&graphPtr->dataTables, BLT_STRING_KEYS);
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
    case CID_ELEM_STRIP:
	Tk_SetClass(tkwin, "Stripchart");
    default:
	Tk_SetClass(tkwin, "???");
	break;
    }
    Blt_SetWindowInstanceData(tkwin, graphPtr);

    if (InitPens(graphPtr) != TCL_OK) {
	goto error;
    }
    if (Blt_ConfigureWidgetFromObj(interp, tkwin, configSpecs, objc - 2, 
		objv + 2, (char *)graphPtr, 0) != TCL_OK) {
	goto error;
    }
    if (Blt_DefaultAxes(graphPtr) != TCL_OK) {
	goto error;
    }
    AdjustAxisPointers(graphPtr);

    if (Blt_CreatePageSetup(graphPtr) != TCL_OK) {
	goto error;
    }
    if (Blt_CreateCrosshairs(graphPtr) != TCL_OK) {
	goto error;
    }
    if (Blt_CreateLegend(graphPtr) != TCL_OK) {
	goto error;
    }
    Tk_CreateEventHandler(graphPtr->tkwin, 
	ExposureMask | StructureNotifyMask | FocusChangeMask, GraphEventProc, 
	graphPtr);

    graphPtr->cmdToken = Tcl_CreateObjCommand(interp, Tcl_GetString(objv[1]), 
	Blt_GraphInstCmdProc, graphPtr, GraphInstCmdDeleteProc);
    ConfigureGraph(graphPtr);
    graphPtr->bindTable = Blt_CreateBindingTable(interp, tkwin, graphPtr, 
	PickEntry, Blt_GraphTags);

    Tcl_SetObjResult(interp, objv[1]);
    return graphPtr;

 error:
    DestroyGraph((DestroyData)graphPtr);
    return NULL;
}

/* Widget sub-commands */

/*ARGSUSED*/
static int
XAxisOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int margin;

    margin = (graphPtr->inverted) ? MARGIN_LEFT : MARGIN_BOTTOM;
    return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int
X2AxisOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int margin;

    margin = (graphPtr->inverted) ? MARGIN_RIGHT : MARGIN_TOP;
    return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int
YAxisOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int margin;

    margin = (graphPtr->inverted) ? MARGIN_BOTTOM : MARGIN_LEFT;
    return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int
Y2AxisOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int margin;

    margin = (graphPtr->inverted) ? MARGIN_TOP : MARGIN_RIGHT;
    return Blt_AxisOp(interp, graphPtr, margin, objc, objv);
}

static int
BarOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ElementOp(graphPtr, interp, objc, objv, CID_ELEM_BAR);
}

/*ARGSUSED*/
static int
LineOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ElementOp(graphPtr, interp, objc, objv, CID_ELEM_LINE);
}

static int
ElementOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ElementOp(graphPtr, interp, objc, objv, graphPtr->classId);
}

static int
ConfigureOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int flags;

    flags = BLT_CONFIG_OBJV_ONLY;
    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)graphPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)graphPtr, objv[2], flags);
    } else {
	if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
		objc - 2, objv + 2, (char *)graphPtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
	ConfigureGraph(graphPtr);
	Blt_EventuallyRedrawGraph(graphPtr);
	return TCL_OK;
    }
}

/* ARGSUSED*/
static int
CgetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
	(char *)graphPtr, objv[2], 0);
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
/* ARGSUSED*/
static int
ExtentsOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
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
	Tcl_Obj *listObjPtr;

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
	Tcl_Obj *listObjPtr;

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
topmargin, bottommargin, plotarea, or legend", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsideOp --
 *
 *	Returns true of false whether the given point is inside the plotting
 *	area (defined by left,bottom right, top).
 *
 * Results:
 *	Always returns TCL_OK.  interp->result will contain the boolean string
 *	representation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED*/
static int
InsideOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int x, y;
    Region2d exts;
    int result;

    if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_GraphExtents(graphPtr, &exts);
    result = PointInRegion(&exts, x, y);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), result);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvtransformOp --
 *
 *	This procedure returns a list of the graph coordinate values
 *	corresponding with the given window X and Y coordinate positions.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred while parsing the
 *	window positions, TCL_ERROR is returned, and interp->result will
 *	contain the error message.  Otherwise interp->result will contain a
 *	Tcl list of the x and y coordinates.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvtransformOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    double x, y;
    Point2d point;
    Axis2d axes;
    Tcl_Obj *listObjPtr;

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
    axes.x = Blt_GetFirstAxis(graphPtr->axisChain[0]);
    axes.y = Blt_GetFirstAxis(graphPtr->axisChain[1]);
    point = Blt_InvMap2D(graphPtr, x, y, &axes);

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(point.x));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(point.y));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TransformOp --
 *
 *	This procedure returns a list of the window coordinates corresponding
 *	with the given graph x and y coordinates.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains the list of
 *	the graph coordinates. If an error occurred while parsing the window
 *	positions, TCL_ERROR is returned, then interp->result will contain an
 *	error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TransformOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    double x, y;
    Point2d point;
    Axis2d axes;
    Tcl_Obj *listObjPtr;

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
    axes.x = Blt_GetFirstAxis(graphPtr->axisChain[0]);
    axes.y = Blt_GetFirstAxis(graphPtr->axisChain[1]);

    point = Blt_Map2D(graphPtr, x, y, &axes);

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(ROUND(point.x)));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(ROUND(point.y)));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

#ifndef NO_PRINTER

/*
 *---------------------------------------------------------------------------
 *
 * Print1Op --
 *
 *	Prints the equivalent of a screen snapshot of the graph to the
 *	designated printer.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred TCL_ERROR is
 *	returned and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
Print1Op(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    BITMAPINFO info;
    void *data;
    TkWinDCState state;
    TkWinBitmap bd;
    DIBSECTION ds;
    Drawable drawable;
    HBITMAP hBitmap;
    HDC hDC;
    DOCINFO di;
    double pageWidth, pageHeight;
    int result;
    double scale, sx, sy;
    int jobId;

    graphPtr->width = Tk_Width(graphPtr->tkwin);
    graphPtr->height = Tk_Height(graphPtr->tkwin);
    if ((graphPtr->width < 2) && (graphPtr->reqWidth > 0)) {
	graphPtr->width = graphPtr->reqWidth;
    }
    if ((graphPtr->height < 2) && (graphPtr->reqHeight > 0)) {
	graphPtr->height = graphPtr->reqHeight;
    }
    if (objc == 2) {
	result = Blt_PrintDialog(interp, &drawable);
	if (result == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (result == TCL_RETURN) {
	    return TCL_OK;
	}
    } else {
	if (Blt_GetOpenPrinter(interp, Tcl_GetString(objv[2]), &drawable) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /*  
     * This is a taken from Blt_SnapPhoto.  The difference is that here we're
     * using the DIBSection directly, without converting the section into a
     * Picture.
     */
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = graphPtr->width;
    info.bmiHeader.biHeight = graphPtr->height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    hDC = TkWinGetDrawableDC(graphPtr->display, Tk_WindowId(graphPtr->tkwin),
		&state);
    hBitmap = CreateDIBSection(hDC, &info, DIB_RGB_COLORS, &data, NULL, 0);
    TkWinReleaseDrawableDC(Tk_WindowId(graphPtr->tkwin), hDC, &state);
    
    /*
     * Create our own drawable by hand using the DIB we just created.  We'll
     * then draw into it using the standard drawing functions.
     */
    bd.type = TWD_BITMAP;
    bd.handle = hBitmap;
    bd.colormap = DefaultColormap(graphPtr->display, 
	DefaultScreen(graphPtr->display));
    bd.depth = Tk_Depth(graphPtr->tkwin);
    
    graphPtr->flags |= RESET_WORLD;
    Blt_DrawGraph(graphPtr, (Drawable)&bd);

    /*
     * Now that the DIB contains the image of the graph, get the the data bits
     * and write them to the printer device, stretching the image to the fit
     * the printer's resolution.
     */
    result = TCL_ERROR;
    if (GetObject(hBitmap, sizeof(DIBSECTION), &ds) == 0) {
	Tcl_AppendResult(interp, "can't get object: ", Blt_LastError(),
	    (char *)NULL);
	goto done;
    }
    hDC = ((TkWinDC *) drawable)->hdc;
    /* Get the resolution of the printer device. */
    sx = (double)GetDeviceCaps(hDC, HORZRES) / (double)graphPtr->width;
    sy = (double)GetDeviceCaps(hDC, VERTRES) / (double)graphPtr->height;
    scale = MIN(sx, sy);
    pageWidth = scale * graphPtr->width;
    pageHeight = scale * graphPtr->height;

    ZeroMemory(&di, sizeof(di));
    di.cbSize = sizeof(di);
    di.lpszDocName = "Graph Contents";
    jobId = StartDoc(hDC, &di);
    if (jobId <= 0) {
	Tcl_AppendResult(interp, "can't start document: ", Blt_LastError(),
	    (char *)NULL);
	goto done;
    }
    if (StartPage(hDC) <= 0) {
	Tcl_AppendResult(interp, "error starting page: ", Blt_LastError(),
	    (char *)NULL);
	goto done;
    }
    StretchDIBits(hDC, 0, 0, ROUND(pageWidth), ROUND(pageHeight), 0, 0, 
	graphPtr->width, graphPtr->height, ds.dsBm.bmBits, 
	(LPBITMAPINFO)&ds.dsBmih, DIB_RGB_COLORS, SRCCOPY);
    EndPage(hDC);
    EndDoc(hDC);
    result = TCL_OK;
  done:
    DeleteBitmap(hBitmap);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Print2Op --
 *
 *	Prints directly to the designated printer device.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *	returned and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
Print2Op(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Drawable drawable;
    int result;

    graphPtr->width = Tk_Width(graphPtr->tkwin);
    graphPtr->height = Tk_Height(graphPtr->tkwin);
    if ((graphPtr->width < 2) && (graphPtr->reqWidth > 0)) {
	graphPtr->width = graphPtr->reqWidth;
    }
    if ((graphPtr->height < 2) && (graphPtr->reqHeight > 0)) {
	graphPtr->height = graphPtr->reqHeight;
    }
    if (objc == 2) {
	result = Blt_PrintDialog(interp, &drawable);
	if (result == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (result == TCL_RETURN) {
	    return TCL_OK;
	}
    } else {
	result = Blt_GetOpenPrinter(interp, Tcl_GetString(objv[2]), &drawable);
    }
    if (result == TCL_OK) {
	int oldMode;
	HDC hDC;
	double xRatio, yRatio;
	TkWinDC *drawPtr;
	int w, h; 

	drawPtr = (TkWinDC *) drawable;
	hDC = drawPtr->hdc;
	Blt_GetPrinterScale(hDC, &xRatio, &yRatio);
	oldMode = SetMapMode(hDC, MM_ISOTROPIC);
	if (oldMode == 0) {
	    Tcl_AppendResult(interp, "can't set mode for printer DC: ",
		Blt_LastError(), (char *)NULL);
	    return TCL_ERROR;
	}
	w = (int)round(graphPtr->width * xRatio);
	h = (int)round(graphPtr->height * yRatio);
	SetViewportExtEx(hDC, w, h, NULL);
	SetWindowExtEx(hDC, graphPtr->width, graphPtr->height, NULL);

	Blt_StartPrintJob(interp, drawable);
	graphPtr->flags |= RESET_WORLD;
	Blt_DrawGraph(graphPtr, drawable);
	Blt_EndPrintJob(interp, drawable);
    }
    return result;
}

#endif /* NO_PRINTER */

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFormat --
 *
 *	Convert a string represent a node number into its integer value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFormat(
    ClientData clientData,		/* Not used.*/
    Tcl_Interp *interp,			/* Interpreter to send results back to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    int *formatPtr = (int *)(record + offset);
    char c;
    const char *string;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'p') && (strcmp(string, "picture") == 0)) {
	*formatPtr = FORMAT_PHOTO;
    } else if ((c == 'p') && (strcmp(string, "photo") == 0)) {
	*formatPtr = FORMAT_PHOTO;
#ifdef WIN32
    } else if ((c == 'e') && (strcmp(string, "emf") == 0)) {
	*formatPtr = FORMAT_EMF;
    } else if ((c == 'w') && (strcmp(string, "wmf") == 0)) {
	*formatPtr = FORMAT_WMF;
#endif /* WIN32 */
    } else {
#ifdef WIN32
	Tcl_AppendResult(interp, "bad format \"", string, 
		 "\": should be picture, photo, emf, or wmf.", (char *)NULL);
#else
	Tcl_AppendResult(interp, "bad format \"", string, 
		 "\": should be picture or photo.", (char *)NULL);
#endif /* WIN32 */
	return TCL_ERROR;
    }
    return TCL_OK;
}

#ifdef WIN32
static int InitMetaFileHeader(
    Tk_Window tkwin,
    int width, int height,
    APMHEADER *mfhPtr)
{
    unsigned int *p;
    unsigned int sum;
#define MM_INCH		25.4
    int xdpi, ydpi;

    mfhPtr->key = 0x9ac6cdd7L;
    mfhPtr->hmf = 0;
    mfhPtr->inch = 1440;

    Blt_ScreenDPI(tkwin, &xdpi, &ydpi);
    mfhPtr->bbox.Left = mfhPtr->bbox.Top = 0;
    mfhPtr->bbox.Bottom = (SHORT)((width * 1440)/ (float)xdpi);
    mfhPtr->bbox.Right = (SHORT)((height * 1440) / (float)ydpi);
    mfhPtr->reserved = 0;
    sum = 0;
    for (p = (unsigned int *)mfhPtr; 
	 p < (unsigned int *)&(mfhPtr->checksum); p++) {
	sum ^= *p;
    }
    mfhPtr->checksum = sum;
    return TCL_OK;
}

static int
CreateAPMetaFile(Tcl_Interp *interp, HANDLE hMetaFile, HDC hDC, 
		 APMHEADER *mfhPtr, const char *fileName)
{
    HANDLE hFile;
    HANDLE hMem;
    LPVOID buffer;
    int result;
    DWORD count, nBytes;

    result = TCL_ERROR;
    hMem = NULL;
    hFile = CreateFile(
       fileName,			/* File path */
       GENERIC_WRITE,			/* Access mode */
       0,				/* No sharing. */
       NULL,				/* Security attributes */
       CREATE_ALWAYS,			/* Overwrite any existing file */
       FILE_ATTRIBUTE_NORMAL,
       NULL);				/* No template file */
    if (hFile == INVALID_HANDLE_VALUE) {
	Tcl_AppendResult(interp, "can't create metafile \"", fileName, 
		"\":", Blt_LastError(), (char *)NULL);
	return TCL_ERROR;
    }
    if ((!WriteFile(hFile, (LPVOID)mfhPtr, sizeof(APMHEADER), &count, 
		NULL)) || (count != sizeof(APMHEADER))) {
	Tcl_AppendResult(interp, "can't create metafile header to \"", 
			 fileName, "\":", Blt_LastError(), (char *)NULL);
	goto error;
    }
    nBytes = GetWinMetaFileBits(hMetaFile, 0, NULL, MM_ANISOTROPIC, hDC);
    hMem = GlobalAlloc(GHND, nBytes);
    if (hMem == NULL) {
	Tcl_AppendResult(interp, "can't create allocate global memory:", 
		Blt_LastError(), (char *)NULL);
	goto error;
    }
    buffer = (LPVOID)GlobalLock(hMem);
    if (!GetWinMetaFileBits(hMetaFile, nBytes, buffer, MM_ANISOTROPIC, hDC)) {
	Tcl_AppendResult(interp, "can't get metafile bits:", 
		Blt_LastError(), (char *)NULL);
	goto error;
    }
    if ((!WriteFile(hFile, buffer, nBytes, &count, NULL)) ||
	(count != nBytes)) {
	Tcl_AppendResult(interp, "can't write metafile bits:", 
		Blt_LastError(), (char *)NULL);
	goto error;
    }
    result = TCL_OK;
 error:
    CloseHandle(hFile);
    if (hMem != NULL) {
	GlobalUnlock(hMem);
	GlobalFree(hMem);
    }
    return result;
}
#endif /*WIN32*/

/*
 *---------------------------------------------------------------------------
 *
 * SnapOp --
 *
 *	Snaps a picture of the graph and stores it in the specified image.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains
 *	the list of the graph coordinates. If an error occurred
 *	while parsing the window positions, TCL_ERROR is returned,
 *	then interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
SnapOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int result;
    Pixmap drawable;
    int i;
    SnapSwitches switches;

    /* .g snap ?switches? name */
    switches.height = Tk_Height(graphPtr->tkwin);
    if ((switches.height < 2) && (graphPtr->reqHeight > 0)) {
	switches.height = graphPtr->reqHeight;
    }
    switches.width = Tk_Width(graphPtr->tkwin);
    if ((switches.width < 2) && (graphPtr->reqWidth > 0)) {
	switches.width = graphPtr->reqWidth;
    }
    switches.format = FORMAT_PICTURE;
    /* Process switches  */
    i = Blt_ParseSwitches(interp, snapSwitches, objc - 2, objv + 2, &switches, 
	BLT_SWITCH_OBJV_PARTIAL);
    if (i < 0) {
	return TCL_ERROR;
    }
    i += 2;
    if (i >= objc) {
	Tcl_AppendResult(interp, "missing name argument: should be \"",
		Tcl_GetString(objv[0]), "snap ?switches? name\"", (char *)NULL);
	return TCL_ERROR;
    }
    switches.name = Tcl_GetString(objv[i]);
    if (switches.width < 2) {
	switches.width = Tk_ReqWidth(graphPtr->tkwin);
    }
    if (switches.height < 2) {
	switches.width = Tk_ReqHeight(graphPtr->tkwin);
    }
    /* Always re-compute the layout of the graph before snapping the picture. */
    graphPtr->width = switches.width;
    graphPtr->height = switches.height;
    Blt_MapGraph(graphPtr);

    drawable = Tk_WindowId(graphPtr->tkwin);
    switch (switches.format) {
    case FORMAT_PICTURE:
    case FORMAT_PHOTO:
	drawable = Tk_GetPixmap(graphPtr->display, drawable, graphPtr->width, 
		graphPtr->height, Tk_Depth(graphPtr->tkwin));
#ifdef WIN32
	assert(drawable != None);
#endif
	graphPtr->flags |= RESET_WORLD;
	Blt_DrawGraph(graphPtr, drawable);
	if (switches.format == FORMAT_PICTURE) {
	    result = Blt_SnapPicture(interp, graphPtr->tkwin, drawable, 0, 0, 
		switches.width, switches.height, switches.width, 
		switches.height, switches.name, 1.0);
	} else {
	    result = Blt_SnapPhoto(interp, graphPtr->tkwin, drawable, 0, 0, 
		switches.width, switches.height, switches.width, 
		switches.height, switches.name, 1.0);
	}
	Tk_FreePixmap(graphPtr->display, drawable);
	break;

#ifdef WIN32
    case FORMAT_WMF:
    case FORMAT_EMF:
	{
	    TkWinDC drawableDC;
	    TkWinDCState state;
	    HDC hRefDC, hDC;
	    HENHMETAFILE hMetaFile;
	    Tcl_DString dString;
	    const char *title;
	    
	    hRefDC = TkWinGetDrawableDC(graphPtr->display, drawable, &state);
	    
	    Tcl_DStringInit(&dString);
	    Tcl_DStringAppend(&dString, "BLT Graph ", -1);
	    Tcl_DStringAppend(&dString, BLT_VERSION, -1);
	    Tcl_DStringAppend(&dString, "\0", -1);
	    Tcl_DStringAppend(&dString, Tk_PathName(graphPtr->tkwin), -1);
	    Tcl_DStringAppend(&dString, "\0", -1);
	    title = Tcl_DStringValue(&dString);
	    hDC = CreateEnhMetaFile(hRefDC, NULL, NULL, title);
	    Tcl_DStringFree(&dString);
	    
	    if (hDC == NULL) {
		Tcl_AppendResult(interp, "can't create metafile: ",
				 Blt_LastError(), (char *)NULL);
		return TCL_ERROR;
	    }
	    
	    drawableDC.hdc = hDC;
	    drawableDC.type = TWD_WINDC;
	    
	    graphPtr->width = switches.width;
	    graphPtr->height = switches.height;
	    Blt_MapGraph(graphPtr);
	    graphPtr->flags |= RESET_WORLD;
	    Blt_DrawGraph(graphPtr, (Drawable)&drawableDC);
	    hMetaFile = CloseEnhMetaFile(hDC);
	    if (strcmp(switches.name, "CLIPBOARD") == 0) {
		HWND hWnd;
		
		hWnd = Tk_GetHWND(drawable);
		OpenClipboard(hWnd);
		EmptyClipboard();
		SetClipboardData(CF_ENHMETAFILE, hMetaFile);
		CloseClipboard();
		result = TCL_OK;
	    } else {
		result = TCL_ERROR;
		if (switches.format == FORMAT_WMF) {
		    APMHEADER mfh;
		    
		    assert(sizeof(mfh) == 22);
		    InitMetaFileHeader(graphPtr->tkwin, switches.width, 
				       switches.height, &mfh);
		    result = CreateAPMetaFile(interp, hMetaFile, hRefDC, &mfh, 
					      switches.name);
		} else {
		    HENHMETAFILE hMetaFile2;
		    
		    hMetaFile2 = CopyEnhMetaFile(hMetaFile, switches.name);
		    if (hMetaFile2 != NULL) {
			result = TCL_OK;
			DeleteEnhMetaFile(hMetaFile2); 
		    }
		}
		DeleteEnhMetaFile(hMetaFile); 
	    }
	    TkWinReleaseDrawableDC(drawable, hRefDC, &state);
	}
	break;
#endif /*WIN32*/
    default:
	Tcl_AppendResult(interp, "bad snapshot format", (char *)NULL);
	return TCL_ERROR;
    }
    graphPtr->flags |= MAP_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * GraphWidgetCmd --
 *
 *	This procedure is invoked to process the TCL command that
 *	corresponds to a widget managed by this module.  See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
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
#ifndef NO_PRINTER
    {"print1",       2, Print1Op,          2, 3, "?printerName?",},
    {"print2",       2, Print2Op,          2, 3, "?printerName?",},
#endif /*NO_PRINTER*/
    {"snap",         1, SnapOp,            3, 0, "?switches? name",},
    {"transform",    1, TransformOp,       4, 4, "x y",},
    {"x2axis",       2, X2AxisOp,          2, 0, "oper ?args?",},
    {"xaxis",        2, XAxisOp,           2, 0, "oper ?args?",},
    {"y2axis",       2, Y2AxisOp,          2, 0, "oper ?args?",},
    {"yaxis",        2, YAxisOp,           2, 0, "oper ?args?",},
};
static int nGraphOps = sizeof(graphOps) / sizeof(Blt_OpSpec);

int
Blt_GraphInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
		     Tcl_Obj *const *objv)
{
    GraphCmdProc *proc;
    int result;
    Graph *graphPtr = clientData;

    proc = Blt_GetOpFromObj(interp, nGraphOps, graphOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(graphPtr);
    result = (*proc) (graphPtr, interp, objc, objv);
    Tcl_Release(graphPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewGraph --
 *
 *	Creates a new window and TCL command representing an instance of a
 *	graph widget.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static int
NewGraph(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, ClassId classId)
{
    Graph *graphPtr;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    graphPtr = CreateGraph(interp, objc, objv, classId);
    if (graphPtr == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GraphCmd --
 *
 *	Creates a new window and TCL command representing an instance of a
 *	graph widget.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GraphCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    return NewGraph(interp, objc, objv, CID_ELEM_LINE);
}

/*
 *---------------------------------------------------------------------------
 *
 * BarchartCmd --
 *
 *	Creates a new window and TCL command representing an instance of a
 *	barchart widget.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BarchartCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    return NewGraph(interp, objc, objv, CID_ELEM_BAR);
}

/*
 *---------------------------------------------------------------------------
 *
 * StripchartCmd --
 *
 *	Creates a new window and TCL command representing an instance of a
 *	barchart widget.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StripchartCmd(ClientData clientData, Tcl_Interp *interp, int objc,
	      Tcl_Obj *const *objv)
{
    return NewGraph(interp, objc, objv, CID_ELEM_STRIP);
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
static void
DrawMargins(Graph *graphPtr, Drawable drawable)
{
    XRectangle rects[4];
    int site;

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
    site = Blt_Legend_Site(graphPtr);
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

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * DrawPlotRegion --
 *
 *	Draws the contents of the plotting area.  This consists of the
 *	elements, markers (draw under elements), axis limits, and possibly the
 *	legend.  Typically, the output will be cached into a backing store
 *	pixmap, so that redraws can occur quickly.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawPlotRegion(Graph *graphPtr, Drawable drawable)
{
    int site;

    /* Clear the background of the plotting area. */
    Blt_FillBackgroundRectangle(graphPtr->tkwin, drawable, graphPtr->plotBg,
	graphPtr->left, graphPtr->top, graphPtr->right - graphPtr->left + 1,
	graphPtr->bottom - graphPtr->top + 1, TK_RELIEF_FLAT, 0);

    /* Draw the elements, markers, legend, and axis limits. */

    Blt_DrawGrids(graphPtr, drawable);
    Blt_DrawMarkers(graphPtr, drawable, MARKER_UNDER);
    site = Blt_Legend_Site(graphPtr);
    if ((site & LEGEND_PLOTAREA_MASK) && 
	(!Blt_Legend_IsRaised(graphPtr))) {
	Blt_DrawLegend(graphPtr, drawable);
    } else if (site == LEGEND_WINDOW) {
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
    Blt_DrawAxisLimits(graphPtr, drawable);
    DrawMargins(graphPtr, drawable);
    Blt_DrawElements(graphPtr, drawable);
    Blt_DrawAxes(graphPtr, drawable);
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * DrawPlot --
 *
 *	Draws the contents of the plotting area.  This consists of the
 *	elements, markers (draw under elements), axis limits, and possibly the
 *	legend.  Typically, the output will be cached into a backing store
 *	pixmap, so that redraws can occur quickly.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawPlot(Graph *graphPtr, Drawable drawable)
{
    int site;

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

    site = Blt_Legend_Site(graphPtr);
    if ((site & LEGEND_PLOTAREA_MASK) && (!Blt_Legend_IsRaised(graphPtr))) {
	Blt_DrawLegend(graphPtr, drawable);
    } else if (site == LEGEND_WINDOW) {
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
    Blt_DrawAxisLimits(graphPtr, drawable);
    Blt_DrawElements(graphPtr, drawable);
    /* Blt_DrawAxes(graphPtr, drawable); */
}

void
Blt_MapGraph(Graph *graphPtr)
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

void
Blt_DrawGraph(Graph *graphPtr, Drawable drawable)
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

static void
UpdateMarginTraces(Graph *graphPtr)
{
    Margin *marginPtr, *endPtr;

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

/*
 *---------------------------------------------------------------------------
 *
 * DisplayGraph --
 *
 *	This procedure is invoked to display a graph widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the graph in its current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayGraph(ClientData clientData)
{
    Graph *graphPtr = clientData;
    Pixmap drawable;
    Tk_Window tkwin;
    int site;

    graphPtr->flags &= ~REDRAW_PENDING;
    if (graphPtr->tkwin == NULL) {
	return;				/* Window has been destroyed (we
					 * should not get here) */
    }
    tkwin = graphPtr->tkwin;
#ifdef notdef
    fprintf(stderr, "Calling DisplayGraph(%s)\n", Tk_PathName(tkwin));
#endif
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
#ifdef WIN32
    assert(drawable != None);
#endif
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

/*LINTLIBRARY*/
int
Blt_GraphCmdInitProc(Tcl_Interp *interp)
{
    static Blt_InitCmdSpec cmdSpecs[] = {
	{"graph", GraphCmd,},
	{"barchart", BarchartCmd,},
	{"stripchart", StripchartCmd,},
    };
    return Blt_InitCmds(interp, "::blt", cmdSpecs, 3);
}

Graph *
Blt_GetGraphFromWindowData(Tk_Window tkwin)
{
    Graph *graphPtr;

    while (tkwin != NULL) {
	graphPtr = (Graph *)Blt_GetWindowInstanceData(tkwin);
	if (graphPtr != NULL) {
	    return graphPtr;
	}
	tkwin = Tk_Parent(tkwin);
    }
    return NULL;
}

int
Blt_GraphType(Graph *graphPtr)
{
    switch (graphPtr->classId) {
    case CID_ELEM_LINE:
	return GRAPH;
    case CID_ELEM_BAR:
	return BARCHART;
    case CID_ELEM_STRIP:
	return STRIPCHART;
    default:
	return 0;
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ReconfigureGraph --
 *
 *	Allocates resources for the graph.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ReconfigureGraph(Graph *graphPtr)	
{
    ConfigureGraph(graphPtr);
    Blt_ConfigureLegend(graphPtr);
    Blt_ConfigureElements(graphPtr);
    Blt_ConfigureAxes(graphPtr);
    Blt_ConfigureMarkers(graphPtr);
}
