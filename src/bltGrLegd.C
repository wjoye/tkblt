/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrLegd.c --
 *
 * This module implements the legend for the BLT graph widget.
 *
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltInt.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltGrElem.h"
#include "bltConfig.h"

/*
 *  Selection related flags:
 *
 *	SELECT_EXPORT		Export the selection to X11.
 *
 *	SELECT_PENDING		A "selection" command idle task is pending.
 *
 *	SELECT_CLEAR		Clear selection flag of entry.
 *
 *	SELECT_SET		Set selection flag of entry.
 *
 *	SELECT_TOGGLE		Toggle selection flag of entry.
 *
 *	SELECT_BLTMASK		Mask of selection set/clear/toggle flags.
 *
 *	SELECT_SORTED		Indicates if the entries in the selection 
 *				should be sorted or displayed in the order 
 *				they were selected.
 *
 */

#define SELECT_CLEAR		(1<<16)
#define SELECT_EXPORT		(1<<17) 
#define SELECT_PENDING		(1<<18)
#define SELECT_SET		(1<<19)
#define SELECT_TOGGLE		(SELECT_SET | SELECT_CLEAR)
#define SELECT_BLTMASK		(SELECT_SET | SELECT_CLEAR)
#define SELECT_SORTED		(1<<20)

#define RAISED			(1<<21)

#define SELECT_MODE_SINGLE	(1<<0)
#define SELECT_MODE_MULTIPLE	(1<<1)

typedef int (GraphLegendProc)(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

/*
 * Legend --
 *
 * 	Contains information specific to how the legend will be displayed.
 */
struct _Legend {
    unsigned int flags;
    ClassId classId;			/* Type: Element or Marker. */

    int nEntries;			/* Number of element entries in
					 * table. */
    short int nColumns, nRows;	        /* Number of columns and rows in
					 * legend */
    short int width, height;		/* Dimensions of the legend */
    short int entryWidth, entryHeight;

    int site;
    short int xReq, yReq;		/* User-requested site of legend, not
					 * the final actual position. Used in
					 * conjunction with the anchor below
					 * to determine location of the
					 * legend. */

    Tk_Anchor anchor;			/* Anchor of legend. Used to interpret
					 * the positioning point of the legend
					 * in the graph*/

    int x, y;				/* Computed origin of legend. */

    Graph *graphPtr;
    Tcl_Command cmdToken;		/* Token for graph's widget command. */
    int reqColumns, reqRows;

    int ixPad, iyPad;		        /* # of pixels interior padding around
					 * legend entries */
    int xPad, yPad;			/* # of pixels padding to exterior of
					 * legend */
    Tk_Window tkwin;			/* If non-NULL, external window to draw
					 * legend. */
    TextStyle style;

    int maxSymSize;			/* Size of largest symbol to be
					 * displayed.  Used to calculate size
					 * of legend */
    XColor *fgColor;
    Blt_Background activeBg;		/* Active legend entry background
					 * color. */
    XColor *activeFgColor;
    int activeRelief;			/* 3-D effect on active entry. */
    int entryBW;		/* Border width around each entry in
					 * legend. */
    Blt_Background normalBg;		/* 3-D effect of legend. */
    int borderWidth;			/* Width of legend 3-D border */
    int relief;				/* 3-d effect of border around the
					 * legend: TK_RELIEF_RAISED etc. */

    Blt_BindTable bindTable;

    int selRelief;
    int selBW;

    XColor *selInFocusFgColor;		/* Text color of a selected entry. */
    XColor *selOutFocusFgColor;

    Blt_Background selInFocusBg;
    Blt_Background selOutFocusBg;

    XColor *focusColor;
    Blt_Dashes focusDashes;		/* Dash on-off value. */
    GC focusGC;				/* Graphics context for the active
					 * label. */

    const char *takeFocus;
    int focus;				/* Position of the focus entry. */

    int cursorX, cursorY;		/* Position of the insertion cursor in
					 * the textbox window. */
    short int cursorWidth;		/* Size of the insertion cursor
					 * symbol. */
    short int cursorHeight;
    Element *focusPtr;			/* Element that currently has the
					 * focus. If NULL, no legend entry has
					 * the focus. */
    Element *selAnchorPtr;		/* Fixed end of selection. Used to
					 * extend the selection while
					 * maintaining the other end of the
					 * selection. */
    Element *selMarkPtr;
    Element *selFirstPtr;		/* First element selected in current
					 * pick. */
    Element *selLastPtr;		/* Last element selected in current
					 * pick. */
    int exportSelection;
    int active;
    int cursorOn;			/* Indicates if the cursor is
					 * displayed. */
    int onTime, offTime;		/* Time in milliseconds to wait before
					 * changing the cursor from off-to-on
					 * and on-to-off. Setting offTime to 0
					 * makes the * cursor steady. */
    Tcl_TimerToken timerToken;		/* Handle for a timer event called
					 * periodically to blink the cursor. */
    const char *selectCmd;		/* TCL script that's invoked whenever
					 * the selection changes. */
    int selectMode;			/* Mode of selection: single or
					 * multiple. */
    Tcl_HashTable selectTable;		/* Table of selected elements. Used to
					 * quickly determine whether an element
					 * is selected. */
    Blt_Chain selected;			/* List of selected elements. */

    const char *title;
    unsigned int titleWidth, titleHeight;
    TextStyle titleStyle;		/* Legend title attributes */
};

#define LABEL_PAD	2

#define DEF_LEGEND_ACTIVEBACKGROUND 	skyblue4
#define DEF_LEGEND_ACTIVEBORDERWIDTH    STD_BORDERWIDTH
#define DEF_LEGEND_ACTIVEFOREGROUND	white
#define DEF_LEGEND_ACTIVERELIEF		"flat"
#define DEF_LEGEND_ANCHOR	   	"n"
#define DEF_LEGEND_BACKGROUND	   	(char *)NULL
#define DEF_LEGEND_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_LEGEND_COLUMNS		"0"
#define DEF_LEGEND_EXPORTSELECTION	"no"
#define DEF_LEGEND_FONT			"hevetica 8 normal roman"
#define DEF_LEGEND_FOREGROUND		STD_NORMAL_FOREGROUND
#define DEF_LEGEND_HIDE			"no"
#define DEF_LEGEND_IPADX		"1"
#define DEF_LEGEND_IPADY		"1"
#define DEF_LEGEND_PADX			"1"
#define DEF_LEGEND_PADY			"1"
#define DEF_LEGEND_POSITION		"rightmargin"
#define DEF_LEGEND_RAISED       	"no"
#define DEF_LEGEND_RELIEF		"flat"
#define DEF_LEGEND_ROWS			"0"
#define DEF_LEGEND_SELECTBACKGROUND 	skyblue4
#define DEF_LEGEND_SELECT_BG_MONO  	STD_SELECT_BG_MONO
#define DEF_LEGEND_SELECTBORDERWIDTH	"1"
#define DEF_LEGEND_SELECTMODE		"multiple"
#define DEF_LEGEND_SELECT_FG_MONO  	STD_SELECT_FG_MONO
#define DEF_LEGEND_SELECTFOREGROUND 	white
#define DEF_LEGEND_SELECTRELIEF		"flat"
#define DEF_LEGEND_FOCUSDASHES		"dot"
#define DEF_LEGEND_FOCUSEDIT		"no"
#define DEF_LEGEND_FOCUSFOREGROUND	STD_ACTIVE_FOREGROUND
#define DEF_LEGEND_FOCUS_FG_MONO	STD_ACTIVE_FG_MONO
#define DEF_LEGEND_TAKEFOCUS		"1"
#define DEF_LEGEND_TITLE		(char *)NULL
#define	DEF_LEGEND_TITLECOLOR		STD_NORMAL_FOREGROUND
#define DEF_LEGEND_TITLEFONT		STD_FONT_SMALL

static Blt_OptionParseProc ObjToPosition;
static Blt_OptionPrintProc PositionToObj;
static Blt_CustomOption legendPositionOption =
{
    ObjToPosition, PositionToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToSelectmode;
static Blt_OptionPrintProc SelectmodeToObj;
static Blt_CustomOption selectmodeOption = {
    ObjToSelectmode, SelectmodeToObj, NULL, NULL,
};

Blt_CustomOption bitmaskGrLegdExportOption =
{
  ObjToBitmaskProc, BitmaskToObjProc, NULL, (ClientData)SELECT_EXPORT
};

Blt_CustomOption bitmaskGrLegdHideOption =
{
  ObjToBitmaskProc, BitmaskToObjProc, NULL, (ClientData)HIDE
};

Blt_CustomOption bitmaskGrLegdRaisedOption =
{
  ObjToBitmaskProc, BitmaskToObjProc, NULL, (ClientData)RAISED
};

static Blt_ConfigSpec configSpecs[] = {
  {BLT_CONFIG_CUSTOM, "-activebackground", "activeBackground",
   "ActiveBackground", DEF_LEGEND_ACTIVEBACKGROUND, 
   Tk_Offset(Legend, activeBg), 0,
   &backgroundOption},
  {BLT_CONFIG_PIXELS, "-activeborderwidth", "activeBorderWidth",
   "BorderWidth", DEF_LEGEND_BORDERWIDTH, 
   Tk_Offset(Legend, entryBW), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
   "ActiveForeground", DEF_LEGEND_ACTIVEFOREGROUND,
   Tk_Offset(Legend, activeFgColor), 0},
  {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
   DEF_LEGEND_ACTIVERELIEF, Tk_Offset(Legend, activeRelief),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_LEGEND_ANCHOR, 
   Tk_Offset(Legend, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
  {BLT_CONFIG_CUSTOM, "-background", "background", "Background",
   DEF_LEGEND_BACKGROUND, Tk_Offset(Legend, normalBg),BLT_CONFIG_NULL_OK,
   &backgroundOption},
  {BLT_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   DEF_LEGEND_BORDERWIDTH, Tk_Offset(Legend, borderWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
  {BLT_CONFIG_INT, "-columns", "columns", "columns",
   DEF_LEGEND_COLUMNS, Tk_Offset(Legend, reqColumns),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-exportselection", "exportSelection",
   "ExportSelection", DEF_LEGEND_EXPORTSELECTION, 
   Tk_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
   &bitmaskGrLegdExportOption},
  {BLT_CONFIG_CUSTOM, "-focusdashes", "focusDashes", "FocusDashes",
   DEF_LEGEND_FOCUSDASHES, Tk_Offset(Legend, focusDashes), 
   BLT_CONFIG_NULL_OK, &dashesOption},
  {BLT_CONFIG_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
   DEF_LEGEND_FOCUSFOREGROUND, Tk_Offset(Legend, focusColor),
   BLT_CONFIG_COLOR_ONLY},
  {BLT_CONFIG_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
   DEF_LEGEND_FOCUS_FG_MONO, Tk_Offset(Legend, focusColor),
   BLT_CONFIG_MONO_ONLY},
  {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_LEGEND_FONT, 
   Tk_Offset(Legend, style.font), 0},
  {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
  {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
   DEF_LEGEND_FOREGROUND, Tk_Offset(Legend, fgColor), 0},
  {BLT_CONFIG_CUSTOM, "-hide", "hide", "Hide", DEF_LEGEND_HIDE, 
   Tk_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
   &bitmaskGrLegdHideOption},
  {BLT_CONFIG_PIXELS, "-ipadx", "iPadX", "Pad", DEF_LEGEND_IPADX, 
   Tk_Offset(Legend, ixPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-ipady", "iPadY", "Pad", DEF_LEGEND_IPADY, 
   Tk_Offset(Legend, iyPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-nofocusselectbackground", 
   "noFocusSelectBackground", "NoFocusSelectBackground", 
   DEF_LEGEND_SELECTBACKGROUND, Tk_Offset(Legend, selOutFocusBg), 0,
   &backgroundOption},
  {BLT_CONFIG_COLOR, "-nofocusselectforeground", "noFocusSelectForeground", 
   "NoFocusSelectForeground", DEF_LEGEND_SELECTFOREGROUND, 
   Tk_Offset(Legend, selOutFocusFgColor), 0},
  {BLT_CONFIG_PIXELS, "-padx", "padX", "Pad", DEF_LEGEND_PADX, 
   Tk_Offset(Legend, xPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-pady", "padY", "Pad", DEF_LEGEND_PADY, 
   Tk_Offset(Legend, yPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-position", "position", "Position", 
   DEF_LEGEND_POSITION, 0, BLT_CONFIG_DONT_SET_DEFAULT, 
   &legendPositionOption},
  {BLT_CONFIG_CUSTOM, "-raised", "raised", "Raised", DEF_LEGEND_RAISED, 
   Tk_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT,
   &bitmaskGrLegdRaisedOption},
  {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_LEGEND_RELIEF, 
   Tk_Offset(Legend, relief), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_INT, "-rows", "rows", "rows", DEF_LEGEND_ROWS, 
   Tk_Offset(Legend, reqRows),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-selectbackground", "selectBackground", 
   "Background", DEF_LEGEND_SELECTBACKGROUND, 
   Tk_Offset(Legend, selInFocusBg), 0,
   &backgroundOption},
  {BLT_CONFIG_PIXELS, "-selectborderwidth", "selectBorderWidth", 
   "BorderWidth", DEF_LEGEND_SELECTBORDERWIDTH, 
   Tk_Offset(Legend, selBW), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-selectcommand", "selectCommand", "SelectCommand",
   (char *)NULL, Tk_Offset(Legend, selectCmd), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
   DEF_LEGEND_SELECT_FG_MONO, Tk_Offset(Legend, selInFocusFgColor),
   BLT_CONFIG_MONO_ONLY},
  {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
   DEF_LEGEND_SELECTFOREGROUND, Tk_Offset(Legend, selInFocusFgColor),
   BLT_CONFIG_COLOR_ONLY},
  {BLT_CONFIG_CUSTOM, "-selectmode", "selectMode", "SelectMode",
   DEF_LEGEND_SELECTMODE, Tk_Offset(Legend, selectMode),
   BLT_CONFIG_DONT_SET_DEFAULT, &selectmodeOption},
  {BLT_CONFIG_RELIEF, "-selectrelief", "selectRelief", "Relief",
   DEF_LEGEND_SELECTRELIEF, Tk_Offset(Legend, selRelief),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
   DEF_LEGEND_TAKEFOCUS, Tk_Offset(Legend, takeFocus), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_STRING, "-title", "title", "Title", DEF_LEGEND_TITLE, 
   Tk_Offset(Legend, title), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-titlecolor", "titleColor", "Foreground",
   DEF_LEGEND_TITLECOLOR, Tk_Offset(Legend, titleStyle.color), 0},
  {BLT_CONFIG_FONT, "-titlefont", "titleFont", "Font",
   DEF_LEGEND_TITLEFONT, Tk_Offset(Legend, titleStyle.font), 0},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Tcl_IdleProc DisplayLegend;
static Blt_BindPickProc PickEntryProc;
static Tk_EventProc LegendEventProc;
static Tcl_TimerProc BlinkCursorProc;
static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;

extern Tcl_ObjCmdProc Blt_GraphInstCmdProc;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Legend_EventuallyRedraw --
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
Blt_Legend_EventuallyRedraw(Graph *graphPtr) 
{
    Legend *legendPtr = graphPtr->legend;

    if ((legendPtr->tkwin != NULL) && !(legendPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayLegend, legendPtr);
	legendPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection changes.
 *      Executes some application-specific code in the -selectcommand option.
 *      This provides a way for applications to handle selection changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
SelectCmdProc(ClientData clientData) 
{
    Legend *legendPtr = clientData;

    Tcl_Preserve(legendPtr);
    legendPtr->flags &= ~SELECT_PENDING;
    if (legendPtr->selectCmd != NULL) {
	Tcl_Interp *interp;

	interp = legendPtr->graphPtr->interp;
	if (Tcl_GlobalEval(interp, legendPtr->selectCmd) != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
    }
    Tcl_Release(legendPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeSelectCmd --
 *
 *      Queues a request to execute the -selectcommand code associated with
 *      the widget at the next idle point.  Invoked whenever the selection
 *      changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyInvokeSelectCmd(Legend *legendPtr)
{
    if ((legendPtr->flags & SELECT_PENDING) == 0) {
	legendPtr->flags |= SELECT_PENDING;
	Tcl_DoWhenIdle(SelectCmdProc, legendPtr);
    }
}

static void
ClearSelection(Legend *legendPtr)
{
    Tcl_DeleteHashTable(&legendPtr->selectTable);
    Tcl_InitHashTable(&legendPtr->selectTable, TCL_ONE_WORD_KEYS);
    Blt_Chain_Reset(legendPtr->selected);
    Blt_Legend_EventuallyRedraw(legendPtr->graphPtr);
    if (legendPtr->selectCmd != NULL) {
	EventuallyInvokeSelectCmd(legendPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * LostSelectionProc --
 *
 *	This procedure is called back by Tk when the selection is grabbed away
 *	from a Text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is marked as
 *	not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelectionProc(ClientData clientData)
{
    Legend *legendPtr = clientData;

    if (legendPtr->flags & SELECT_EXPORT) {
	ClearSelection(legendPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LegendEventProc --
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
LegendEventProc(ClientData clientData, XEvent *eventPtr)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    Blt_Legend_EventuallyRedraw(graphPtr);
	}
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail == NotifyInferior) {
	    return;
	}
	if (eventPtr->type == FocusIn) {
	    legendPtr->flags |= FOCUS;
	} else {
	    legendPtr->flags &= ~FOCUS;
	}
	Tcl_DeleteTimerHandler(legendPtr->timerToken);
	if ((legendPtr->active) && (legendPtr->flags & FOCUS)) {
	    legendPtr->cursorOn = TRUE;
	    if (legendPtr->offTime != 0) {
		legendPtr->timerToken = Tcl_CreateTimerHandler(
			legendPtr->onTime, BlinkCursorProc, graphPtr);
	    }
	} else {
	    legendPtr->cursorOn = FALSE;
	    legendPtr->timerToken = (Tcl_TimerToken)NULL;
	}
	Blt_Legend_EventuallyRedraw(graphPtr);
    } else if (eventPtr->type == DestroyNotify) {
	Graph *graphPtr = legendPtr->graphPtr;

	if (legendPtr->site == LEGEND_WINDOW) {
	    if (legendPtr->cmdToken != NULL) {
		Tcl_DeleteCommandFromToken(graphPtr->interp, 
					   legendPtr->cmdToken);
		legendPtr->cmdToken = NULL;
	    }
	    legendPtr->tkwin = graphPtr->tkwin;
	}
	if (legendPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayLegend, legendPtr);
	    legendPtr->flags &= ~REDRAW_PENDING;
	}
	legendPtr->site = LEGEND_RIGHT;
	legendPtr->flags |= HIDE;
	graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
	Blt_MoveBindingTable(legendPtr->bindTable, graphPtr->tkwin);
	Blt_EventuallyRedrawGraph(graphPtr);
    } else if (eventPtr->type == ConfigureNotify) {
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
}

static int
CreateLegendWindow(Tcl_Interp *interp, Legend *legendPtr, const char *pathName)
{
    Graph *graphPtr;
    Tk_Window tkwin;

    graphPtr = legendPtr->graphPtr;
    tkwin = Tk_CreateWindowFromPath(interp, graphPtr->tkwin, pathName, NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    ((TkWindow*)tkwin)->instanceData = legendPtr;
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	  LegendEventProc, graphPtr);
    /* Move the legend's binding table to the new window. */
    Blt_MoveBindingTable(legendPtr->bindTable, tkwin);
    if (legendPtr->tkwin != graphPtr->tkwin) {
	Tk_DestroyWindow(legendPtr->tkwin);
    }
    /* Create a command by the same name as the legend window so that Legend
     * bindings can use %W interchangably.  */
    legendPtr->cmdToken = Tcl_CreateObjCommand(interp, pathName, 
	Blt_GraphInstCmdProc, graphPtr, NULL);
    legendPtr->tkwin = tkwin;
    legendPtr->site = LEGEND_WINDOW;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPosition --
 *
 *	Convert the string representation of a legend XY position into window
 *	coordinates.  The form of the string must be "@x,y" or none.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is written
 *	into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* New legend position string */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    Graph *graphPtr;
    Legend *legendPtr = (Legend *)widgRec;
    char c;
    int length;
    const char *string;

    graphPtr = legendPtr->graphPtr;
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if (c == '\0') {
	legendPtr->site = LEGEND_RIGHT;
    } else if ((c == 'l') && (strncmp(string, "leftmargin", length) == 0)) {
	legendPtr->site = LEGEND_LEFT;
    } else if ((c == 'r') && (strncmp(string, "rightmargin", length) == 0)) {
	legendPtr->site = LEGEND_RIGHT;
    } else if ((c == 't') && (strncmp(string, "topmargin", length) == 0)) {
	legendPtr->site = LEGEND_TOP;
    } else if ((c == 'b') && (strncmp(string, "bottommargin", length) == 0)) {
	legendPtr->site = LEGEND_BOTTOM;
    } else if ((c == 'p') && (strncmp(string, "plotarea", length) == 0)) {
	legendPtr->site = LEGEND_PLOT;
    } else if (c == '@') {
	char *comma;
	long x, y;
	int result;
	
	comma = strchr(string + 1, ',');
	if (comma == NULL) {
	    Tcl_AppendResult(interp, "bad screen position \"", string,
			     "\": should be @x,y", (char *)NULL);
	    return TCL_ERROR;
	}
	x = y = 0;
	*comma = '\0';
	result = ((Tcl_ExprLong(interp, string + 1, &x) == TCL_OK) &&
		  (Tcl_ExprLong(interp, comma + 1, &y) == TCL_OK));
	*comma = ',';
	if (!result) {
	    return TCL_ERROR;
	}
	legendPtr->xReq = x;
	legendPtr->yReq = y;
	legendPtr->site = LEGEND_XY;
    } else if (c == '.') {
	if (CreateLegendWindow(interp, legendPtr, string) != TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_Legend_EventuallyRedraw(graphPtr);
    } else {
	Tcl_AppendResult(interp, "bad position \"", string, "\": should be  \
\"leftmargin\", \"rightmargin\", \"topmargin\", \"bottommargin\", \
\"plotarea\", windowName or @x,y", (char *)NULL);
	return TCL_ERROR;
    }
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PositionToObj --
 *
 *	Convert the window coordinates into a string.
 *
 * Results:
 *	The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PositionToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    Legend *legendPtr = (Legend *)widgRec;
    Tcl_Obj *objPtr;

    switch (legendPtr->site) {
    case LEGEND_LEFT:
	objPtr = Tcl_NewStringObj("leftmargin", -1);
	break;

    case LEGEND_RIGHT:
	objPtr = Tcl_NewStringObj("rightmargin", -1);
	break;

    case LEGEND_TOP:
	objPtr = Tcl_NewStringObj("topmargin", -1);
	break;

    case LEGEND_BOTTOM:
	objPtr = Tcl_NewStringObj("bottommargin", -1);
	break;

    case LEGEND_PLOT:
	objPtr = Tcl_NewStringObj("plotarea", -1);
	break;

    case LEGEND_WINDOW:
	objPtr = Tcl_NewStringObj(Tk_PathName(legendPtr->tkwin), -1);
	break;

    case LEGEND_XY:
	{
	    char string[200];

	    sprintf_s(string, 200, "@%d,%d", legendPtr->xReq, legendPtr->yReq);
	    objPtr = Tcl_NewStringObj(string, -1);
	}
    default:
	objPtr = Tcl_NewStringObj("unknown legend position", -1);
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSelectmode --
 *
 *	Convert the string reprsenting a select mode, to its numeric form.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSelectmode(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representing the new value. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    char *string;
    char c;
    int *modePtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 's') && (strcmp(string, "single") == 0)) {
	*modePtr = SELECT_MODE_SINGLE;
    } else if ((c == 'm') && (strcmp(string, "multiple") == 0)) {
	*modePtr = SELECT_MODE_MULTIPLE;
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
	*modePtr = SELECT_MODE_SINGLE;
    } else {
	Tcl_AppendResult(interp, "bad select mode \"", string,
	    "\": should be \"single\" or \"multiple\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectmodeToObj --
 *
 * Results:
 *	The string representation of the select mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SelectmodeToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,
    int offset,				/* Offset to field in structure */
    int flags)	
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case SELECT_MODE_SINGLE:
	return Tcl_NewStringObj("single", -1);
    case SELECT_MODE_MULTIPLE:
	return Tcl_NewStringObj("multiple", -1);
    default:
	return Tcl_NewStringObj("unknown scroll mode", -1);
    }
}


static void
SetLegendOrigin(Legend *legendPtr)
{
    Graph *graphPtr;
    int x, y, w, h;

    graphPtr = legendPtr->graphPtr;
    x = y = w = h = 0;			/* Suppress compiler warning. */
    switch (legendPtr->site) {
    case LEGEND_RIGHT:
	w = graphPtr->rightMargin.width - graphPtr->rightMargin.axesOffset;
	h = graphPtr->bottom - graphPtr->top;
	x = graphPtr->right + graphPtr->rightMargin.axesOffset;
	y = graphPtr->top;
	break;

    case LEGEND_LEFT:
	w = graphPtr->leftMargin.width - graphPtr->leftMargin.axesOffset;
	h = graphPtr->bottom - graphPtr->top;
	x = graphPtr->inset;
	y = graphPtr->top;
	break;

    case LEGEND_TOP:
	w = graphPtr->right - graphPtr->left;
	h = graphPtr->topMargin.height - graphPtr->topMargin.axesOffset;
	if (graphPtr->title != NULL) {
	    h -= graphPtr->titleHeight;
	}
	x = graphPtr->left;
	y = graphPtr->inset;
	if (graphPtr->title != NULL) {
	    y += graphPtr->titleHeight;
	}
	break;

    case LEGEND_BOTTOM:
	w = graphPtr->right - graphPtr->left;
	h = graphPtr->bottomMargin.height - graphPtr->bottomMargin.axesOffset;
	x = graphPtr->left;
	y = graphPtr->bottom + graphPtr->bottomMargin.axesOffset;
	break;

    case LEGEND_PLOT:
	w = graphPtr->right - graphPtr->left;
	h = graphPtr->bottom - graphPtr->top;
	x = graphPtr->left;
	y = graphPtr->top;
	break;

    case LEGEND_XY:
	w = legendPtr->width;
	h = legendPtr->height;
	x = legendPtr->xReq;
	y = legendPtr->yReq;
	if (x < 0) {
	    x += graphPtr->width;
	}
	if (y < 0) {
	    y += graphPtr->height;
	}
	break;

    case LEGEND_WINDOW:
	legendPtr->anchor = TK_ANCHOR_NW;
	legendPtr->x = legendPtr->y = 0;
	return;
    }

    switch (legendPtr->anchor) {
    case TK_ANCHOR_NW:			/* Upper left corner */
	break;
    case TK_ANCHOR_W:			/* Left center */
	if (h > legendPtr->height) {
	    y += (h - legendPtr->height) / 2;
	}
	break;
    case TK_ANCHOR_SW:			/* Lower left corner */
	if (h > legendPtr->height) {
	    y += (h - legendPtr->height);
	}
	break;
    case TK_ANCHOR_N:			/* Top center */
	if (w > legendPtr->width) {
	    x += (w - legendPtr->width) / 2;
	}
	break;
    case TK_ANCHOR_CENTER:		/* Center */
	if (h > legendPtr->height) {
	    y += (h - legendPtr->height) / 2;
	}
	if (w > legendPtr->width) {
	    x += (w - legendPtr->width) / 2;
	}
	break;
    case TK_ANCHOR_S:			/* Bottom center */
	if (w > legendPtr->width) {
	    x += (w - legendPtr->width) / 2;
	}
	if (h > legendPtr->height) {
	    y += (h - legendPtr->height);
	}
	break;
    case TK_ANCHOR_NE:			/* Upper right corner */
	if (w > legendPtr->width) {
	    x += w - legendPtr->width;
	}
	break;
    case TK_ANCHOR_E:			/* Right center */
	if (w > legendPtr->width) {
	    x += w - legendPtr->width;
	}
	if (h > legendPtr->height) {
	    y += (h - legendPtr->height) / 2;
	}
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
	if (w > legendPtr->width) {
	    x += w - legendPtr->width;
	}
	if (h > legendPtr->height) {
	    y += (h - legendPtr->height);
	}
	break;
    }
    legendPtr->x = x + legendPtr->xPad;
    legendPtr->y = y + legendPtr->yPad;
}

static int
EntryIsSelected(Legend *legendPtr, Element *elemPtr)
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
    return (hPtr != NULL);
}

static void
SelectElement(Legend *legendPtr, Element *elemPtr)
{
    int isNew;
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_CreateHashEntry(&legendPtr->selectTable, (char *)elemPtr,&isNew);
    if (isNew) {
	Blt_ChainLink link;

	link = Blt_Chain_Append(legendPtr->selected, elemPtr);
	Tcl_SetHashValue(hPtr, link);
    }
}

static void
DeselectElement(Legend *legendPtr, Element *elemPtr)
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
    if (hPtr != NULL) {
	Blt_ChainLink link;

	link = Tcl_GetHashValue(hPtr);
	Blt_Chain_DeleteLink(legendPtr->selected, link);
	Tcl_DeleteHashEntry(hPtr);
    }
}

static void
SelectEntry(Legend *legendPtr, Element *elemPtr)
{
    Tcl_HashEntry *hPtr;

    switch (legendPtr->flags & SELECT_BLTMASK) {
    case SELECT_CLEAR:
	DeselectElement(legendPtr, elemPtr);
	break;

    case SELECT_SET:
	SelectElement(legendPtr, elemPtr);
	break;

    case SELECT_TOGGLE:
	hPtr = Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
	if (hPtr != NULL) {
	    DeselectElement(legendPtr, elemPtr);
	} else {
	    SelectElement(legendPtr, elemPtr);
	}
	break;
    }
}

/*ARGSUSED*/
static ClientData
PickEntryProc(ClientData clientData, int x, int y, ClientData *contextPtr)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;
    int w, h;

    legendPtr = graphPtr->legend;
    w = legendPtr->width;
    h = legendPtr->height;

    if (legendPtr->titleHeight > 0) {
	y -= legendPtr->titleHeight + legendPtr->yPad;
    }
    x -= legendPtr->x + legendPtr->borderWidth;
    y -= legendPtr->y + legendPtr->borderWidth;
    w -= 2 * legendPtr->borderWidth + 2*legendPtr->xPad;
    h -= 2 * legendPtr->borderWidth + 2*legendPtr->yPad;

    if ((x >= 0) && (x < w) && (y >= 0) && (y < h)) {
	int row, column;
	int n;

	/*
	 * It's in the bounding box, so compute the index.
	 */
	row    = y / legendPtr->entryHeight;
	column = x / legendPtr->entryWidth;
	n = (column * legendPtr->nRows) + row;
	if (n < legendPtr->nEntries) {
	    Blt_ChainLink link;
	    int count;

	    /* Legend entries are stored in bottom-to-top. */
	    count = 0;
	    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
		 link != NULL; link = Blt_Chain_NextLink(link)) {
		Element *elemPtr;

		elemPtr = Blt_Chain_GetValue(link);
		if (elemPtr->label != NULL) {
		    if (count == n) {
			return elemPtr;
		    }
		    count++;
		}
	    }	      
	    if (link != NULL) {
		return Blt_Chain_GetValue(link);
	    }	
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MapLegend --
 *
 * 	Calculates the dimensions (width and height) needed for the legend.
 * 	Also determines the number of rows and columns necessary to list all
 * 	the valid element labels.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *   	The following fields of the legend are calculated and set.
 *
 * 	nEntries   - number of valid labels of elements in the
 *		      display list.
 * 	nRows	    - number of rows of entries
 * 	nColumns    - number of columns of entries
 * 	entryHeight - height of each entry
 * 	entryWidth  - width of each entry
 * 	height	    - width of legend (includes borders and padding)
 * 	width	    - height of legend (includes borders and padding)
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MapLegend(
    Graph *graphPtr,
    int plotWidth,			/* Maximum width available in window
					 * to draw the legend. Will calculate
					 * # of columns from this. */
    int plotHeight)			/* Maximum height available in window
					 * to draw the legend. Will calculate
					 * # of rows from this. */
{
    Legend *legendPtr = graphPtr->legend;
    Blt_ChainLink link;
    int nRows, nColumns, nEntries;
    int lw, lh;
    int maxWidth, maxHeight;
    int symbolWidth;
    Tk_FontMetrics fontMetrics;

    /* Initialize legend values to default (no legend displayed) */
    legendPtr->entryWidth = legendPtr->entryHeight = 0;
    legendPtr->nRows = legendPtr->nColumns = legendPtr->nEntries = 0;
    legendPtr->height = legendPtr->width = 0;

    if (legendPtr->site == LEGEND_WINDOW) {
	if (Tk_Width(legendPtr->tkwin) > 1) {
	    plotWidth = Tk_Width(legendPtr->tkwin);
	}
	if (Tk_Height(legendPtr->tkwin) > 1) {
	    plotHeight = Tk_Height(legendPtr->tkwin);
	}
    }
    Blt_Ts_GetExtents(&legendPtr->titleStyle, legendPtr->title, 
		      &legendPtr->titleWidth, &legendPtr->titleHeight);
    /*   
     * Count the number of legend entries and determine the widest and tallest
     * label.  The number of entries would normally be the number of elements,
     * but elements can have no legend entry (-label "").
     */
    nEntries = 0;
    maxWidth = maxHeight = 0;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	unsigned int w, h;
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;			/* Element has no legend entry. */
	}
	Blt_Ts_GetExtents(&legendPtr->style, elemPtr->label, &w, &h);
	if (maxWidth < w) {
	    maxWidth = w;
	}
	if (maxHeight < h) {
	    maxHeight = h;
	}
	nEntries++;
    }
    if (nEntries == 0) {
	return;				/* No visible legend entries. */
    }


    Tk_GetFontMetrics(legendPtr->style.font, &fontMetrics);
    symbolWidth = 2 * fontMetrics.ascent;

    maxWidth += 2 * legendPtr->entryBW + 2*legendPtr->ixPad +
	+ symbolWidth + 3 * LABEL_PAD;

    maxHeight += 2 * legendPtr->entryBW + 2*legendPtr->iyPad;

    maxWidth |= 0x01;
    maxHeight |= 0x01;

    lw = plotWidth - 2 * legendPtr->borderWidth - 2*legendPtr->xPad;
    lh = plotHeight - 2 * legendPtr->borderWidth - 2*legendPtr->yPad;

    /*
     * The number of rows and columns is computed as one of the following:
     *
     *	both options set		User defined. 
     *  -rows				Compute columns from rows.
     *  -columns			Compute rows from columns.
     *	neither set			Compute rows and columns from
     *					size of plot.  
     */
    if (legendPtr->reqRows > 0) {
	nRows = MIN(legendPtr->reqRows, nEntries); 
	if (legendPtr->reqColumns > 0) {
	    nColumns = MIN(legendPtr->reqColumns, nEntries);
	} else {
	    nColumns = ((nEntries - 1) / nRows) + 1; /* Only -rows. */
	}
    } else if (legendPtr->reqColumns > 0) { /* Only -columns. */
	nColumns = MIN(legendPtr->reqColumns, nEntries);
	nRows = ((nEntries - 1) / nColumns) + 1;
    } else {			
	/* Compute # of rows and columns from the legend size. */
	nRows = lh / maxHeight;
	nColumns = lw / maxWidth;
	if (nRows < 1) {
	    nRows = nEntries;
	}
	if (nColumns < 1) {
	    nColumns = nEntries;
	}
	if (nRows > nEntries) {
	    nRows = nEntries;
	} 
	switch (legendPtr->site) {
	case LEGEND_TOP:
	case LEGEND_BOTTOM:
	    nRows = ((nEntries - 1) / nColumns) + 1;
	    break;
	case LEGEND_LEFT:
	case LEGEND_RIGHT:
	default:
	    nColumns = ((nEntries - 1) / nRows) + 1;
	    break;
	}
    }
    if (nColumns < 1) {
	nColumns = 1;
    } 
    if (nRows < 1) {
	nRows = 1;
    }

    lh = (nRows * maxHeight);
    if (legendPtr->titleHeight > 0) {
	lh += legendPtr->titleHeight + legendPtr->yPad;
    }
    lw = nColumns * maxWidth;
    if (lw < legendPtr->titleWidth) {
	lw = legendPtr->titleWidth;
    }
    legendPtr->width = lw + 2 * legendPtr->borderWidth + 
	2*legendPtr->xPad;
    legendPtr->height = lh + 2 * legendPtr->borderWidth + 
	2*legendPtr->yPad;
    legendPtr->nRows = nRows;
    legendPtr->nColumns = nColumns;
    legendPtr->nEntries = nEntries;
    legendPtr->entryHeight = maxHeight;
    legendPtr->entryWidth = maxWidth;

    {
	int row, col, count;

	row = col = count = 0;
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    count++;
	    elemPtr->row = row;
	    elemPtr->col = col;
	    row++;
	    if ((count % nRows) == 0) {
		col++;
		row = 0;
	    }
	}
    }
    if ((legendPtr->site == LEGEND_WINDOW) &&
	((Tk_ReqWidth(legendPtr->tkwin) != legendPtr->width) ||
	 (Tk_ReqHeight(legendPtr->tkwin) != legendPtr->height))) {
	Tk_GeometryRequest(legendPtr->tkwin,legendPtr->width,legendPtr->height);
    }
}

void
Blt_DrawLegend(Graph *graphPtr, Drawable drawable)
{
    Blt_Background bg;
    Blt_ChainLink link;
    Tk_FontMetrics fontMetrics;
    Legend *legendPtr = graphPtr->legend;
    Pixmap pixmap;
    Tk_Window tkwin;
    int count;
    int symbolSize, xMid, yMid;
    int x, y, w, h;
    int xLabel, yStart, xSymbol, ySymbol;

    if ((legendPtr->flags & HIDE) || (legendPtr->nEntries == 0)) {
	return;
    }

    SetLegendOrigin(legendPtr);
    graphPtr = legendPtr->graphPtr;
    tkwin = legendPtr->tkwin;
    if (legendPtr->site == LEGEND_WINDOW) {
	w = Tk_Width(tkwin);
	h = Tk_Height(tkwin);
    } else {
	w = legendPtr->width;
	h = legendPtr->height;
    }

    pixmap = Tk_GetPixmap(graphPtr->display, Tk_WindowId(tkwin), w, h, 
	Tk_Depth(tkwin));

    if (legendPtr->normalBg != NULL) {
	Blt_FillBackgroundRectangle(tkwin, pixmap, legendPtr->normalBg, 0, 0, 
		w, h, 0, TK_RELIEF_FLAT);
    } else if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
	/* 
	 * Legend background is transparent and is positioned over the the
	 * plot area.  Either copy the part of the background from the backing
	 * store pixmap or (if no backing store exists) just fill it with the
	 * background color of the plot.
	 */
	if (graphPtr->cache != None) {
	    XCopyArea(graphPtr->display, graphPtr->cache, pixmap, 
		graphPtr->drawGC, legendPtr->x, legendPtr->y, w, h, 0, 0);
        } else {
	    Blt_FillBackgroundRectangle(tkwin, pixmap, graphPtr->plotBg, 0, 0, 
		w, h, TK_RELIEF_FLAT, 0);
 	}
    } else {
	int xOrigin, yOrigin;
	/* 
	 * The legend is located in one of the margins or the external window.
	 */
	Blt_GetBackgroundOrigin(graphPtr->normalBg, &xOrigin, &yOrigin);
	Blt_SetBackgroundOrigin(graphPtr->tkwin, graphPtr->normalBg, 
		xOrigin - legendPtr->x,yOrigin - legendPtr->y);
	Blt_FillBackgroundRectangle(tkwin, pixmap, graphPtr->normalBg, 0, 0, 
		w, h, 0, TK_RELIEF_FLAT);
	Blt_SetBackgroundOrigin(tkwin, graphPtr->normalBg, xOrigin, yOrigin);
    }
    Tk_GetFontMetrics(legendPtr->style.font, &fontMetrics);

    symbolSize = fontMetrics.ascent;
    xMid = symbolSize + 1 + legendPtr->entryBW;
    yMid = (symbolSize / 2) + 1 + legendPtr->entryBW;
    xLabel = 2 * symbolSize + legendPtr->entryBW + 
	legendPtr->ixPad + 2 * LABEL_PAD;
    ySymbol = yMid + legendPtr->iyPad; 
    xSymbol = xMid + LABEL_PAD;

    x = legendPtr->xPad + legendPtr->borderWidth;
    y = legendPtr->yPad + legendPtr->borderWidth;
    Blt_DrawText(tkwin, pixmap, legendPtr->title, &legendPtr->titleStyle, x, y);
    if (legendPtr->titleHeight > 0) {
	y += legendPtr->titleHeight + legendPtr->yPad;
    }
    count = 0;
    yStart = y;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;
	int isSelected;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;			/* Skip this entry */
	}
	isSelected = EntryIsSelected(legendPtr, elemPtr);
	if (elemPtr->flags & LABEL_ACTIVE) {
	    int xOrigin, yOrigin;

	    Blt_GetBackgroundOrigin(legendPtr->activeBg, &xOrigin, &yOrigin);
	    Blt_SetBackgroundOrigin(tkwin, legendPtr->activeBg, 
		xOrigin - legendPtr->x, yOrigin - legendPtr->y);
	    Blt_Ts_SetForeground(legendPtr->style, legendPtr->activeFgColor);
	    Blt_FillBackgroundRectangle(tkwin, pixmap, legendPtr->activeBg, 
		x, y, legendPtr->entryWidth, legendPtr->entryHeight, 
		legendPtr->entryBW, legendPtr->activeRelief);
	    Blt_SetBackgroundOrigin(tkwin, legendPtr->activeBg, 
		xOrigin, yOrigin);
	} else if (isSelected) {
	    int xOrigin, yOrigin;
	    Blt_Background bg;
	    XColor *fg;

	    fg = (legendPtr->flags & FOCUS) ?
		legendPtr->selInFocusFgColor : legendPtr->selOutFocusFgColor;
	    bg = (legendPtr->flags & FOCUS) ?
		    legendPtr->selInFocusBg : legendPtr->selOutFocusBg;
	    Blt_GetBackgroundOrigin(bg, &xOrigin, &yOrigin);
	    Blt_SetBackgroundOrigin(tkwin, bg, xOrigin - legendPtr->x, 
		yOrigin - legendPtr->y);
	    Blt_Ts_SetForeground(legendPtr->style, fg);
	    Blt_FillBackgroundRectangle(tkwin, pixmap, bg, x, y, 
		legendPtr->entryWidth, legendPtr->entryHeight, 
		legendPtr->selBW, legendPtr->selRelief);
	    Blt_SetBackgroundOrigin(tkwin, bg, xOrigin, yOrigin);
	} else {
	    Blt_Ts_SetForeground(legendPtr->style, legendPtr->fgColor);
	    if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
		Blt_FillBackgroundRectangle(tkwin, pixmap, graphPtr->normalBg, 
			x, y, legendPtr->entryWidth, 
			legendPtr->entryHeight, legendPtr->entryBW, 
			elemPtr->legendRelief);
	    }
	}
	(*elemPtr->procsPtr->drawSymbolProc) (graphPtr, pixmap, elemPtr,
		x + xSymbol, y + ySymbol, symbolSize);
	Blt_DrawText(tkwin, pixmap, elemPtr->label, &legendPtr->style, 
		x + xLabel, 
		y + legendPtr->entryBW + legendPtr->iyPad);
	count++;
	if (legendPtr->focusPtr == elemPtr) { /* Focus outline */
	    if (isSelected) {
		XColor *color;

		color = (legendPtr->flags & FOCUS) ?
		    legendPtr->selInFocusFgColor :
		    legendPtr->selOutFocusFgColor;
		XSetForeground(graphPtr->display, legendPtr->focusGC, 
			       color->pixel);
	    }
	    XDrawRectangle(graphPtr->display, pixmap, legendPtr->focusGC, 
		x + 1, y + 1, legendPtr->entryWidth - 3, 
		legendPtr->entryHeight - 3);
	    if (isSelected) {
		XSetForeground(graphPtr->display, legendPtr->focusGC, 
			legendPtr->focusColor->pixel);
	    }
	}
	/* Check when to move to the next column */
	if ((count % legendPtr->nRows) > 0) {
	    y += legendPtr->entryHeight;
	} else {
	    x += legendPtr->entryWidth;
	    y = yStart;
	}
    }
    /*
     * Draw the border and/or background of the legend.
     */
    bg = legendPtr->normalBg;
    if (bg == NULL) {
	bg = graphPtr->normalBg;
    }
    /* Disable crosshairs before redisplaying to the screen */
    if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
	Blt_DisableCrosshairs(graphPtr);
    }
    Blt_DrawBackgroundRectangle(tkwin, pixmap, bg, 0, 0, w, h, 
	legendPtr->borderWidth, legendPtr->relief);
    XCopyArea(graphPtr->display, pixmap, drawable, graphPtr->drawGC, 0, 0, w, h,
	legendPtr->x, legendPtr->y);
    if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
	Blt_EnableCrosshairs(graphPtr);
    }
    Tk_FreePixmap(graphPtr->display, pixmap);
    graphPtr->flags &= ~DRAW_LEGEND;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LegendToPostScript --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_LegendToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    Legend *legendPtr = graphPtr->legend;
    double x, y, yStart;
    int xLabel, xSymbol, ySymbol;
    int count;
    Blt_ChainLink link;
    int symbolSize, xMid, yMid;
    int width, height;
    Tk_FontMetrics fontMetrics;

    if ((legendPtr->flags & HIDE) || (legendPtr->nEntries == 0)) {
	return;
    }
    SetLegendOrigin(legendPtr);

    x = legendPtr->x, y = legendPtr->y;
    width = legendPtr->width - 2*legendPtr->xPad;
    height = legendPtr->height - 2*legendPtr->yPad;

    Blt_Ps_Append(ps, "% Legend\n");
    graphPtr = legendPtr->graphPtr;
    if (graphPtr->pageSetup->flags & PS_DECORATIONS) {
	if (legendPtr->normalBg != NULL) {
	    Tk_3DBorder border;

	    border = Blt_BackgroundBorder(legendPtr->normalBg);
	    Blt_Ps_Fill3DRectangle(ps, border, x, y, width, height, 
		legendPtr->borderWidth, legendPtr->relief);
	} else {
	    Tk_3DBorder border;

	    border = Blt_BackgroundBorder(graphPtr->normalBg);
	    Blt_Ps_Draw3DRectangle(ps, border, x, y, width, height, 
		legendPtr->borderWidth, legendPtr->relief);
	}
    } else {
	Blt_Ps_SetClearBackground(ps);
	Blt_Ps_XFillRectangle(ps, x, y, width, height);
    }
    Tk_GetFontMetrics(legendPtr->style.font, &fontMetrics);
    symbolSize = fontMetrics.ascent;
    xMid = symbolSize + 1 + legendPtr->entryBW;
    yMid = (symbolSize / 2) + 1 + legendPtr->entryBW;
    xLabel = 2 * symbolSize + legendPtr->entryBW + legendPtr->ixPad + 5;
    xSymbol = xMid + legendPtr->ixPad;
    ySymbol = yMid + legendPtr->iyPad;

    x += legendPtr->borderWidth;
    y += legendPtr->borderWidth;
    Blt_Ps_DrawText(ps, legendPtr->title, &legendPtr->titleStyle, x, y);
    if (legendPtr->titleHeight > 0) {
	y += legendPtr->titleHeight + legendPtr->yPad;
    }

    count = 0;
    yStart = y;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;			/* Skip this label */
	}
	if (elemPtr->flags & LABEL_ACTIVE) {
	    Tk_3DBorder border;
	    
	    border = Blt_BackgroundBorder(legendPtr->activeBg);
	    Blt_Ts_SetForeground(legendPtr->style, legendPtr->activeFgColor);
	    Blt_Ps_Fill3DRectangle(ps, border, x, y, legendPtr->entryWidth, 
		legendPtr->entryHeight, legendPtr->entryBW, 
		legendPtr->activeRelief);
	} else {
	    Blt_Ts_SetForeground(legendPtr->style, legendPtr->fgColor);
	    if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
		Tk_3DBorder border;

		border = Blt_BackgroundBorder(graphPtr->normalBg);
		Blt_Ps_Draw3DRectangle(ps, border, x, y, legendPtr->entryWidth,
			legendPtr->entryHeight, legendPtr->entryBW, 
			elemPtr->legendRelief);
	    }
	}
	(*elemPtr->procsPtr->printSymbolProc) (graphPtr, ps, elemPtr, 
		x + xSymbol, y + ySymbol, symbolSize);
	Blt_Ps_DrawText(ps, elemPtr->label, &legendPtr->style, 
		x + xLabel, y + legendPtr->entryBW + legendPtr->iyPad);
	count++;
	if ((count % legendPtr->nRows) > 0) {
	    y += legendPtr->entryHeight;
	} else {
	    x += legendPtr->entryWidth;
	    y = yStart;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayLegend --
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayLegend(ClientData clientData)
{
    Legend *legendPtr = clientData;
    Graph *graphPtr;

    legendPtr->flags &= ~REDRAW_PENDING;
    if (legendPtr->tkwin == NULL) {
	return;				/* Window has been destroyed. */
    }
    graphPtr = legendPtr->graphPtr;
    if (legendPtr->site == LEGEND_WINDOW) {
	int w, h;

	w = Tk_Width(legendPtr->tkwin);
	h = Tk_Height(legendPtr->tkwin);
	if ((w != legendPtr->width) || (h != legendPtr->height)) {
	    Blt_MapLegend(graphPtr, w, h);
	}
    }
    if (Tk_IsMapped(legendPtr->tkwin)) {
	Blt_DrawLegend(graphPtr, Tk_WindowId(legendPtr->tkwin));
    }
}

void Blt_ConfigureLegend(Graph *graphPtr)
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    /* GC for active label. Dashed outline. */
    gcMask = GCForeground | GCLineStyle;
    gcValues.foreground = legendPtr->focusColor->pixel;
    gcValues.line_style = (LineIsDashed(legendPtr->focusDashes))
	? LineOnOffDash : LineSolid;
    newGC = Blt_GetPrivateGC(legendPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(legendPtr->focusDashes)) {
	legendPtr->focusDashes.offset = 2;
	Blt_SetDashes(graphPtr->display, newGC, &legendPtr->focusDashes);
    }
    if (legendPtr->focusGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, legendPtr->focusGC);
    }
    legendPtr->focusGC = newGC;
    
    /*
     *  Update the layout of the graph (and redraw the elements) if any of
     *  the following legend options (all of which affect the size of the
     *  legend) have changed.
     *
     *		-activeborderwidth, -borderwidth
     *		-border
     *		-font
     *		-hide
     *		-ipadx, -ipady, -padx, -pady
     *		-rows
     *
     *  If the position of the legend changed to/from the default
     *  position, also indicate that a new layout is needed.
     *
     */
    if (legendPtr->site == LEGEND_WINDOW) {
	Blt_Legend_EventuallyRedraw(graphPtr);
    } else if (Blt_ConfigModified(configSpecs, "-*border*", "-*pad?",
	"-hide", "-font", "-rows", (char *)NULL)) {
	graphPtr->flags |= RESET_WORLD;
	graphPtr->flags |= (REDRAW_WORLD | CACHE_DIRTY);
	Blt_EventuallyRedrawGraph(graphPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyLegend --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with the legend are freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyLegend(Graph *graphPtr)
{
    Legend *legendPtr = graphPtr->legend;

    if (graphPtr->legend == NULL) {
	return;
    }

    Blt_FreeOptions(configSpecs, (char *)legendPtr, graphPtr->display, 0);
    Blt_Ts_FreeStyle(graphPtr->display, &legendPtr->style);
    Blt_Ts_FreeStyle(graphPtr->display, &legendPtr->titleStyle);
    Blt_DestroyBindingTable(legendPtr->bindTable);
    
    if (legendPtr->focusGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, legendPtr->focusGC);
    }
    if (legendPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(legendPtr->timerToken);
    }
    if (legendPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(legendPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    if (legendPtr->site == LEGEND_WINDOW) {
	Tk_Window tkwin;
	
	/* The graph may be in the process of being torn down */
	if (legendPtr->cmdToken != NULL) {
	    Tcl_DeleteCommandFromToken(graphPtr->interp, legendPtr->cmdToken);
	}
	if (legendPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayLegend, legendPtr);
	    legendPtr->flags &= ~REDRAW_PENDING;
	}
	tkwin = legendPtr->tkwin;
	legendPtr->tkwin = NULL;
	if (tkwin != NULL) {
	    Tk_DeleteEventHandler(tkwin, ExposureMask | StructureNotifyMask,
		LegendEventProc, graphPtr);
	    Tk_DestroyWindow(tkwin);
	}
    }
    free(legendPtr);
}

int Blt_CreateLegend(Graph *graphPtr)
{
    Legend *legendPtr;

    legendPtr = calloc(1, sizeof(Legend));
    graphPtr->legend = legendPtr;
    legendPtr->graphPtr = graphPtr;
    legendPtr->tkwin = graphPtr->tkwin;
    legendPtr->xReq = legendPtr->yReq = -SHRT_MAX;
    legendPtr->relief = TK_RELIEF_SUNKEN;
    legendPtr->activeRelief = TK_RELIEF_FLAT;
    legendPtr->entryBW = 2;
    legendPtr->borderWidth = 2;
    legendPtr->ixPad = 1;
    legendPtr->iyPad = 1;
    legendPtr->xPad = 1;
    legendPtr->yPad = 1;
    legendPtr->anchor = TK_ANCHOR_N;
    legendPtr->site = LEGEND_RIGHT;
    legendPtr->selectMode = SELECT_MODE_MULTIPLE;
    Blt_Ts_InitStyle(legendPtr->style);
    Blt_Ts_InitStyle(legendPtr->titleStyle);
    legendPtr->style.justify = TK_JUSTIFY_LEFT;
    legendPtr->style.anchor = TK_ANCHOR_NW;
    legendPtr->titleStyle.justify = TK_JUSTIFY_LEFT;
    legendPtr->titleStyle.anchor = TK_ANCHOR_NW;
    legendPtr->bindTable = Blt_CreateBindingTable(graphPtr->interp,
	graphPtr->tkwin, graphPtr, PickEntryProc, Blt_GraphTags);

    Tcl_InitHashTable(&legendPtr->selectTable, TCL_ONE_WORD_KEYS);
    legendPtr->selected = Blt_Chain_Create();
    Tk_CreateSelHandler(legendPtr->tkwin, XA_PRIMARY, XA_STRING, 
	SelectionProc, legendPtr, XA_STRING);
    legendPtr->selRelief = TK_RELIEF_FLAT;
    legendPtr->selBW = 1;
    legendPtr->onTime = 600;
    legendPtr->offTime = 300;
    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
	    "legend", "Legend", configSpecs, 0, (Tcl_Obj **)NULL,
	    (char *)legendPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;
}

static Element *
GetNextRow(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col;
    row = focusPtr->row + 1;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;	
	}
    }
    return NULL;
}

static Element *
GetNextColumn(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col + 1;
    row = focusPtr->row;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;		/* Don't go beyond legend boundaries. */
	}
    }
    return NULL;
}

static Element *
GetPreviousRow(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col;
    row = focusPtr->row - 1;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;	
	}
    }
    return NULL;
}

static Element *
GetPreviousColumn(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col - 1;
    row = focusPtr->row;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;	
	}
    }
    return NULL;
}

static Element *
GetFirstElement(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label != NULL) {
	    return elemPtr;
	}
    }
    return NULL;
}

static Element *
GetLastElement(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label != NULL) {
	    return elemPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetElementFromObj --
 *
 *	Parse an index into an entry and return either its value or an error.
 *
 * Results:
 *	A standard TCL result.  If all went well, then *indexPtr is filled in
 *	with the character index (into entryPtr) corresponding to string.  The
 *	index value is guaranteed to lie between 0 and the number of characters
 *	in the string, inclusive.  If an error occurs then an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetElementFromObj(Graph *graphPtr, Tcl_Obj *objPtr, Element **elemPtrPtr)
{
    Element *elemPtr;
    Legend *legendPtr;
    Tcl_Interp *interp;
    char c;
    const char *string;
    int last;

    legendPtr = graphPtr->legend;
    interp = graphPtr->interp;
    string = Tcl_GetString(objPtr);
    c = string[0];
    elemPtr = NULL;

    last = Blt_Chain_GetLength(graphPtr->elements.displayList) - 1;
    if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
	elemPtr = legendPtr->selAnchorPtr;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
	elemPtr = (Element *)Blt_GetCurrentItem(legendPtr->bindTable);
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
	elemPtr = GetFirstElement(graphPtr);
    } else if ((c == 'f') && (strcmp(string, "focus") == 0)) {
	elemPtr = legendPtr->focusPtr;
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
	elemPtr = GetLastElement(graphPtr);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	elemPtr = GetLastElement(graphPtr);
    } else if ((c == 'n') && (strcmp(string, "next.row") == 0)) {
	elemPtr = GetNextRow(graphPtr, legendPtr->focusPtr);
    } else if ((c == 'n') && (strcmp(string, "next.column") == 0)) {
	elemPtr = GetNextColumn(graphPtr, legendPtr->focusPtr);
    } else if ((c == 'p') && (strcmp(string, "previous.row") == 0)) {
	elemPtr = GetPreviousRow(graphPtr, legendPtr->focusPtr);
    } else if ((c == 'p') && (strcmp(string, "previous.column") == 0)) {
	elemPtr = GetPreviousColumn(graphPtr, legendPtr->focusPtr);
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
	elemPtr = legendPtr->selFirstPtr;
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
	elemPtr = legendPtr->selLastPtr;
    } else if (c == '@') {
	int x, y;

	if (Blt_GetXY(interp, legendPtr->tkwin, string, &x, &y) != TCL_OK) {
	    return TCL_ERROR;
	}
	elemPtr = (Element *)PickEntryProc(graphPtr, x, y, NULL);
    } else {
	if (Blt_GetElement(interp, graphPtr, objPtr, &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (elemPtr->link == NULL) {
	    Tcl_AppendResult(interp, "bad legend index \"", string, "\"",
				 (char *)NULL);
	    return TCL_ERROR;
	}
	if (elemPtr->label == NULL) {
	    elemPtr = NULL;
	}
    }
    *elemPtrPtr = elemPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectRange --
 *
 *	Sets the selection flag for a range of nodes.  The range is determined
 *	by two pointers which designate the first/last nodes of the range.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectRange(Legend *legendPtr, Element *fromPtr, Element *toPtr)
{

    if (Blt_Chain_IsBefore(fromPtr->link, toPtr->link)) {
	Blt_ChainLink link;

	for (link = fromPtr->link; link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    SelectEntry(legendPtr, elemPtr);
	    if (link == toPtr->link) {
		break;
	    }
	}
    } else {
	Blt_ChainLink link;

	for (link = fromPtr->link; link != NULL;
	     link = Blt_Chain_PrevLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    SelectEntry(legendPtr, elemPtr);
	    if (link == toPtr->link) {
		break;
	    }
	}
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 * 	Activates a particular label in the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    unsigned int active, redraw;
    const char *string;
    int i;

    string = Tcl_GetString(objv[2]);
    active = (string[0] == 'a') ? LABEL_ACTIVE : 0;
    redraw = FALSE;
    for (i = 3; i < objc; i++) {
	Blt_ChainLink link;
	const char *pattern;

	pattern = Tcl_GetString(objv[i]);
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	
	    elemPtr = Blt_Chain_GetValue(link);
	    if (Tcl_StringMatch(elemPtr->obj.name, pattern)) {
		fprintf(stderr, "legend %s(%s) %s is currently %d\n",
			string, pattern, elemPtr->obj.name, 
			(elemPtr->flags & LABEL_ACTIVE));
		if (active) {
		    if ((elemPtr->flags & LABEL_ACTIVE) == 0) {
			elemPtr->flags |= LABEL_ACTIVE;
			redraw = TRUE;
		    }
		} else {
		    if (elemPtr->flags & LABEL_ACTIVE) {
			elemPtr->flags &= ~LABEL_ACTIVE;
			redraw = TRUE;
		    }
		}
		fprintf(stderr, "legend %s(%s) %s is now %d\n",
			string, pattern, elemPtr->obj.name, 
			(elemPtr->flags & LABEL_ACTIVE));
	    }
	}
    }
    if ((redraw) && ((legendPtr->flags & HIDE) == 0)) {
	/*
	 * See if how much we need to draw. If the graph is already scheduled
	 * for a redraw, just make sure the right flags are set.  Otherwise
	 * redraw only the legend: it's either in an external window or it's
	 * the only thing that need updating.
	 */
	if ((legendPtr->site != LEGEND_WINDOW) && 
	    (graphPtr->flags & REDRAW_PENDING)) {
	    graphPtr->flags |= CACHE_DIRTY;
	    graphPtr->flags |= REDRAW_WORLD; /* Redraw entire graph. */
	} else {
	    Blt_Legend_EventuallyRedraw(graphPtr);
	}
    }
    {
	Blt_ChainLink link;
	Tcl_Obj *listObjPtr;
	
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* List active elements in stacking order. */
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;

	    elemPtr = Blt_Chain_GetValue(link);
	    if (elemPtr->flags & LABEL_ACTIVE) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	  .t bind index sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (objc == 3) {
	Tcl_HashEntry *hPtr;
	Tcl_HashSearch iter;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.tagTable, &iter);
	    hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
	    const char *tagName;
	    Tcl_Obj *objPtr;

	    tagName = Tcl_GetHashKey(&graphPtr->elements.tagTable, hPtr);
	    objPtr = Tcl_NewStringObj(tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->legend->bindTable,
	Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, 
	objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 * 	Queries or resets options for the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)graphPtr->legend, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * 	Queries or resets options for the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int flags = BLT_CONFIG_OBJV_ONLY;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)legendPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)legendPtr, objv[3], flags);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
		objc - 3, objv + 3, (char *)legendPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_ConfigureLegend(graphPtr);
    return TCL_OK;
}


/*ARGSUSED*/
static int
CurselectionOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (legendPtr->flags & SELECT_SORTED) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(legendPtr->selected); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    Tcl_Obj *objPtr;

	    elemPtr = Blt_Chain_GetValue(link);
	    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_ChainLink link;

	/* List of selected entries is in stacking order. */
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;

	    elemPtr = Blt_Chain_GetValue(link);

	    if (EntryIsSelected(legendPtr, elemPtr)) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
FocusOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;

    if (objc == 4) {
	Element *elemPtr;

	if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((elemPtr != NULL) && (elemPtr != legendPtr->focusPtr)) {
	    /* Changing focus can only affect the visible entries.  The entry
	     * layout stays the same. */
	    legendPtr->focusPtr = elemPtr;
	}
	Blt_SetFocusItem(legendPtr->bindTable, legendPtr->focusPtr, 
			 CID_LEGEND_ENTRY);
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
    if (legendPtr->focusPtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		legendPtr->focusPtr->obj.name, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 * 	Find the legend entry from the given argument.  The argument can be
 * 	either a screen position "@x,y" or the name of an element.
 *
 *	I don't know how useful it is to test with the name of an element.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;

    if (((legendPtr->flags & HIDE) == 0) && (legendPtr->nEntries > 0)) {
	Element *elemPtr;

	if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (elemPtr != NULL) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name,-1);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionAnchorOp --
 *
 *	Sets the selection anchor to the element given by a index.  The
 *	selection anchor is the end of the selection that is fixed while
 *	dragging out a selection with the mouse.  The index "anchor" may be
 *	used to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    Element *elemPtr;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Set both the anchor and the mark. Indicates that a single entry
     * is selected. */
    legendPtr->selAnchorPtr = elemPtr;
    legendPtr->selMarkPtr = NULL;
    if (elemPtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
    }
    Blt_Legend_EventuallyRedraw(graphPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectionClearallOp
 *
 *	Clears the entire selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearallOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;

    ClearSelection(legendPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *	Returns 1 if the element indicated by index is currently
 *	selected, 0 if it isn't.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    Element *elemPtr;
    int bool;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = EntryIsSelected(legendPtr, elemPtr);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionMarkOp --
 *
 *	Sets the selection mark to the element given by a index.  The
 *	selection anchor is the end of the selection that is movable while
 *	dragging out a selection with the mouse.  The index "mark" may be used
 *	to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    Element *elemPtr;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (legendPtr->selAnchorPtr == NULL) {
	Tcl_AppendResult(interp, "selection anchor must be set first", 
		 (char *)NULL);
	return TCL_ERROR;
    }
    if (legendPtr->selMarkPtr != elemPtr) {
	Blt_ChainLink link, next;

	/* Deselect entry from the list all the way back to the anchor. */
	for (link = Blt_Chain_LastLink(legendPtr->selected); link != NULL; 
	     link = next) {
	    Element *selectPtr;

	    next = Blt_Chain_PrevLink(link);
	    selectPtr = Blt_Chain_GetValue(link);
	    if (selectPtr == legendPtr->selAnchorPtr) {
		break;
	    }
	    DeselectElement(legendPtr, selectPtr);
	}
	legendPtr->flags &= ~SELECT_BLTMASK;
	legendPtr->flags |= SELECT_SET;
	SelectRange(legendPtr, legendPtr->selAnchorPtr, elemPtr);
	Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
	legendPtr->selMarkPtr = elemPtr;

	Blt_Legend_EventuallyRedraw(graphPtr);
	if (legendPtr->selectCmd != NULL) {
	    EventuallyInvokeSelectCmd(legendPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionPresentOp
 *
 *	Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *	A standard TCL result.  interp->result will contain a boolean string
 *	indicating if there is a selection.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    int bool;

    bool = (Blt_Chain_GetLength(legendPtr->selected) > 0);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionSetOp
 *
 *	Selects, deselects, or toggles all of the elements in the range
 *	between first and last, inclusive, without affecting the selection
 *	state of elements outside that range.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.g legend selection set first last
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Legend *legendPtr = graphPtr->legend;
    Element *firstPtr, *lastPtr;
    const char *string;

    legendPtr->flags &= ~SELECT_BLTMASK;
    string = Tcl_GetString(objv[3]);
    switch (string[0]) {
    case 's':
	legendPtr->flags |= SELECT_SET;
	break;
    case 'c':
	legendPtr->flags |= SELECT_CLEAR;
	break;
    case 't':
	legendPtr->flags |= SELECT_TOGGLE;
	break;
    }
    if (GetElementFromObj(graphPtr, objv[4], &firstPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((firstPtr->flags & HIDE) && ((legendPtr->flags & SELECT_CLEAR)==0)) {
	Tcl_AppendResult(interp, "can't select hidden node \"", 
		Tcl_GetString(objv[4]), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    lastPtr = firstPtr;
    if (objc > 5) {
	if (GetElementFromObj(graphPtr, objv[5], &lastPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((lastPtr->flags & HIDE) && 
	    ((legendPtr->flags & SELECT_CLEAR) == 0)) {
	    Tcl_AppendResult(interp, "can't select hidden node \"", 
		     Tcl_GetString(objv[5]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (firstPtr == lastPtr) {
	SelectEntry(legendPtr, firstPtr);
    } else {
	SelectRange(legendPtr, firstPtr, lastPtr);
    }
    /* Set both the anchor and the mark. Indicates that a single entry is
     * selected. */
    if (legendPtr->selAnchorPtr == NULL) {
	legendPtr->selAnchorPtr = firstPtr;
    }
    if (legendPtr->flags & SELECT_EXPORT) {
	Tk_OwnSelection(legendPtr->tkwin, XA_PRIMARY, LostSelectionProc, 
			legendPtr);
    }
    Blt_Legend_EventuallyRedraw(graphPtr);
    if (legendPtr->selectCmd != NULL) {
	EventuallyInvokeSelectCmd(legendPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *	This procedure handles the individual options for text selections.
 *	The selected text is designated by start and end indices into the text
 *	pool.  The selected segment has both a anchored and unanchored ends.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.g legend selection anchor 
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"anchor",   1, SelectionAnchorOp,   5, 5, "elem",},
    {"clear",    5, SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"clearall", 6, SelectionClearallOp, 4, 4, "",},
    {"includes", 1, SelectionIncludesOp, 5, 5, "elem",},
    {"mark",     1, SelectionMarkOp,     5, 5, "elem",},
    {"present",  1, SelectionPresentOp,  4, 4, "",},
    {"set",      1, SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"toggle",   1, SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
};
static int nSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int
SelectionOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    GraphLegendProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nSelectionOps, selectionOps, BLT_OP_ARG3, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (graphPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LegendOp --
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Legend is possibly redrawn.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec legendOps[] =
{
    {"activate",     1, ActivateOp,      3, 0, "?pattern?...",},
    {"bind",         1, BindOp,          3, 6, "elem sequence command",},
    {"cget",         2, CgetOp,          4, 4, "option",},
    {"configure",    2, ConfigureOp,     3, 0, "?option value?...",},
    {"curselection", 2, CurselectionOp,  3, 3, "",},
    {"deactivate",   1, ActivateOp,      3, 0, "?pattern?...",},
    {"focus",        1, FocusOp,         4, 4, "elem",},
    {"get",          1, GetOp,           4, 4, "elem",},
    //    {"icon",         1, IconOp,          5, 5, "elem image",},
    {"selection",    1, SelectionOp,     3, 0, "args"},
};
static int nLegendOps = sizeof(legendOps) / sizeof(Blt_OpSpec);

int
Blt_LegendOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    GraphLegendProc *proc;

    proc = Blt_GetOpFromObj(interp, nLegendOps, legendOps, BLT_OP_ARG2, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (graphPtr, interp, objc, objv);
}

int 
Blt_Legend_Site(Graph *graphPtr)
{
    return graphPtr->legend->site;
}

int 
Blt_Legend_Width(Graph *graphPtr)
{
    return graphPtr->legend->width;
}

int 
Blt_Legend_Height(Graph *graphPtr)
{
    return graphPtr->legend->height;
}

int 
Blt_Legend_IsHidden(Graph *graphPtr)
{
    return (graphPtr->legend->flags & HIDE);
}

int 
Blt_Legend_IsRaised(Graph *graphPtr)
{
    return (graphPtr->legend->flags & RAISED);
}

int 
Blt_Legend_X(Graph *graphPtr)
{
    return graphPtr->legend->x;
}

int 
Blt_Legend_Y(Graph *graphPtr)
{
    return graphPtr->legend->y;
}

void
Blt_Legend_RemoveElement(Graph *graphPtr, Element *elemPtr)
{
    Blt_DeleteBindings(graphPtr->legend->bindTable, elemPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionProc --
 *
 *	This procedure is called back by Tk when the selection is requested by
 *	someone.  It returns part or all of the selection in a buffer provided
 *	by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NUL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectionProc(
    ClientData clientData,		/* Information about the widget. */
    int offset,				/* Offset within selection of first
					 * character to be returned. */
    char *buffer,			/* Location in which to place
					 * selection. */
    int maxBytes)			/* Maximum number of bytes to place at
					 * buffer, not including terminating
					 * NULL character. */
{
    Legend *legendPtr = clientData;
    int nBytes;
    Tcl_DString dString;

    if ((legendPtr->flags & SELECT_EXPORT) == 0) {
	return -1;
    }
    /* Retrieve the names of the selected entries. */
    Tcl_DStringInit(&dString);
    if (legendPtr->flags & SELECT_SORTED) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(legendPtr->selected); 
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;

	    elemPtr = Blt_Chain_GetValue(link);
	    Tcl_DStringAppend(&dString, elemPtr->obj.name, -1);
	    Tcl_DStringAppend(&dString, "\n", -1);
	}
    } else {
	Blt_ChainLink link;
	Graph *graphPtr;

	graphPtr = legendPtr->graphPtr;
	/* List of selected entries is in stacking order. */
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    if (EntryIsSelected(legendPtr, elemPtr)) {
		Tcl_DStringAppend(&dString, elemPtr->obj.name, -1);
		Tcl_DStringAppend(&dString, "\n", -1);
	    }
	}
    }
    nBytes = Tcl_DStringLength(&dString) - offset;
    strncpy(buffer, Tcl_DStringValue(&dString) + offset, maxBytes);
    Tcl_DStringFree(&dString);
    buffer[maxBytes] = '\0';
    return MIN(nBytes, maxBytes);
}


/*
 *---------------------------------------------------------------------------
 *
 * BlinkCursorProc --
 *
 *	This procedure is called as a timer handler to blink the insertion
 *	cursor off and on.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off, redisplay gets invoked, and this
 *	procedure reschedules itself.
 *
 *---------------------------------------------------------------------------
 */
static void
BlinkCursorProc(ClientData clientData)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (!(legendPtr->flags & FOCUS) || (legendPtr->offTime == 0)) {
	return;
    }
    if (legendPtr->active) {
	int time;

	legendPtr->cursorOn ^= 1;
	time = (legendPtr->cursorOn) ? legendPtr->onTime : legendPtr->offTime;
	legendPtr->timerToken = Tcl_CreateTimerHandler(time, BlinkCursorProc, 
		graphPtr);
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
}
