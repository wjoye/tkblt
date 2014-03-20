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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

extern "C" {
#include "bltInt.h"
#include "bltGraph.h"
#include "bltOp.h"
};

#include "bltConfig.h"
#include "bltGrElem.h"

/*
 *  Selection related flags:
 *
 *	SELECT_PENDING		A "selection" command idle task is pending.
 *
 *	SELECT_CLEAR		Clear selection flag of entry.
 *
 *	SELECT_SET		Set selection flag of entry.
 *
 *	SELECT_TOGGLE		Toggle selection flag of entry.
 *			        Mask of selection set/clear/toggle flags.
 *
 *	SELECT_SORTED		Indicates if the entries in the selection 
 *				should be sorted or displayed in the order 
 *				they were selected.
 *
 */

#define SELECT_CLEAR		(1<<24)
#define SELECT_PENDING		(1<<25)
#define SELECT_SET		(1<<26)
#define SELECT_SORTED		(1<<27)
#define SELECT_TOGGLE		(SELECT_SET | SELECT_CLEAR)

#define LABEL_PAD	2

typedef enum {
  SELECT_MODE_SINGLE, SELECT_MODE_MULTIPLE
} SelectMode;

struct _Legend {
  Tk_OptionTable optionTable;
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

  Graph* graphPtr;
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
  XColor* fgColor;
  Tk_3DBorder activeBg;		/* Active legend entry background
				 * color. */
  XColor* activeFgColor;
  int activeRelief;			/* 3-D effect on active entry. */
  int entryBW;		/* Border width around each entry in
			 * legend. */
  Tk_3DBorder normalBg;		/* 3-D effect of legend. */
  int borderWidth;			/* Width of legend 3-D border */
  int relief;				/* 3-d effect of border around the
					 * legend: TK_RELIEF_RAISED etc. */

  Blt_BindTable bindTable;

  int selRelief;
  int selBW;

  XColor* selInFocusFgColor;		/* Text color of a selected entry. */
  XColor* selOutFocusFgColor;

  Tk_3DBorder selInFocusBg;
  Tk_3DBorder selOutFocusBg;

  XColor* focusColor;
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
  int hide;
  int raised;
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
  SelectMode selectMode;			/* Mode of selection: single or
						 * multiple. */
  Tcl_HashTable selectTable;		/* Table of selected elements. Used to
					 * quickly determine whether an element
					 * is selected. */
  Blt_Chain selected;			/* List of selected elements. */

  const char *title;
  unsigned int titleWidth, titleHeight;
  TextStyle titleStyle;		/* Legend title attributes */
};

// Defs

extern "C" {
  Tcl_ObjCmdProc Blt_GraphInstCmdProc;
};

static int SelectionOp(Graph* graphPtr, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[]);
static int LegendObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			      int objc, Tcl_Obj* const objv[]);
static void ConfigureLegend(Graph* graphPtr);
static int EntryIsSelected(Legend* legendPtr, Element* elemPtr);
static int GetElementFromObj(Graph* graphPtr, Tcl_Obj *objPtr, 
			     Element **elemPtrPtr);
static void ClearSelection(Legend* legendPtr);
static void DeselectElement(Legend* legendPtr, Element* elemPtr);
static int SelectRange(Legend* legendPtr, Element *fromPtr, Element *toPtr);
static void EventuallyInvokeSelectCmd(Legend* legendPtr);
static void SelectEntry(Legend* legendPtr, Element* elemPtr);
static int CreateLegendWindow(Tcl_Interp* interp, Legend* legendPtr, 
			      const char *pathName);

static Tcl_IdleProc DisplayLegend;
static Blt_BindPickProc PickEntryProc;
static Tk_EventProc LegendEventProc;
static Tcl_TimerProc BlinkCursorProc;
static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;

typedef int (GraphLegendProc)(Graph* graphPtr, Tcl_Interp* interp, 
			      int objc, Tcl_Obj* const objv[]);

// OptionSpecs

static const char* selectmodeObjOption[] = {"single", "multiple", NULL};

static Tk_CustomOptionSetProc PositionSetProc;
static Tk_CustomOptionGetProc PositionGetProc;
Tk_ObjCustomOption positionObjOption =
  {
    "position", PositionSetProc, PositionGetProc, NULL, NULL, NULL
  };

static int PositionSetProc(ClientData clientData, Tcl_Interp* interp,
			   Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			   int offset, char* save, int flags)
{
  Legend* legendPtr = (Legend*)widgRec;
  int length;
  const char* string = Tcl_GetStringFromObj(*objPtr, &length);
  char c;
  c = string[0];
  if (c == '\0')
    legendPtr->site = LEGEND_RIGHT;
  else if ((c == 'l') && (strncmp(string, "leftmargin", length) == 0))
    legendPtr->site = LEGEND_LEFT;
  else if ((c == 'r') && (strncmp(string, "rightmargin", length) == 0))
    legendPtr->site = LEGEND_RIGHT;
  else if ((c == 't') && (strncmp(string, "topmargin", length) == 0))
    legendPtr->site = LEGEND_TOP;
  else if ((c == 'b') && (strncmp(string, "bottommargin", length) == 0))
    legendPtr->site = LEGEND_BOTTOM;
  else if ((c == 'p') && (strncmp(string, "plotarea", length) == 0))
    legendPtr->site = LEGEND_PLOT;
  else if (c == '@') {
    char *comma;
    long x, y;
    int result;
	
    comma = (char*)strchr(string + 1, ',');
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
  }
  else if (c == '.') {
    if (CreateLegendWindow(interp, legendPtr, string) != TCL_OK) {
      return TCL_ERROR;
    }
  }
  else {
    Tcl_AppendResult(interp, "bad position \"", string, "\": should be  \
\"leftmargin\", \"rightmargin\", \"topmargin\", \"bottommargin\", \
\"plotarea\", windowName or @x,y", (char *)NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static Tcl_Obj* PositionGetProc(ClientData clientData, Tk_Window tkwin, 
				char *widgRec, int offset)
{
  Legend* legendPtr = (Legend* )widgRec;
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

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_BORDER, "-activebackground", "activeBackground",
   "ActiveBackground", 
   STD_ACTIVE_BACKGROUND, -1, Tk_Offset(Legend, activeBg), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-activeborderwidth", "activeBorderWidth", 
   "ActiveBorderWidth", 
   STD_BORDERWIDTH, -1, Tk_Offset(Legend, entryBW), 0, NULL, 0}, 
  {TK_OPTION_COLOR, "-activeforeground", "activeForeground", "ActiveForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(Legend, activeFgColor), 0, NULL, 0},
  {TK_OPTION_RELIEF, "-activerelief", "activeRelief", "ActiveRelief",
   "flat", -1, Tk_Offset(Legend, activeRelief), 0, NULL, 0},
  {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor", 
   "n", -1, Tk_Offset(Legend, anchor), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   NULL, -1, Tk_Offset(Legend, normalBg), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(Legend, borderWidth), 0, NULL, 0}, 
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_INT, "-columns", "columns", "columns",
   "0", -1, Tk_Offset(Legend, reqColumns), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-exportselection", "exportSelection", "ExportSelection", 
   "no",  -1, Tk_Offset(Legend, exportSelection), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-focusdashes", "focusDashes", "FocusDashes",
   "dot", -1, Tk_Offset(Legend, focusDashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(Legend, focusColor), 0, NULL, 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_SMALL, -1, Tk_Offset(Legend, style.font), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Legend, fgColor), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(Legend, hide), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-ipadx", "iPadX", "Pad", 
   "1", -1, Tk_Offset(Legend, ixPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-ipady", "iPadY", "Pad", 
   "1", -1, Tk_Offset(Legend, iyPad), 0, NULL, 0},
  {TK_OPTION_BORDER, "-nofocusselectbackground", "noFocusSelectBackground", 
   "NoFocusSelectBackground", 
   STD_ACTIVE_BACKGROUND, -1, Tk_Offset(Legend, selOutFocusBg), 0, NULL, 0},
  {TK_OPTION_COLOR, "-nofocusselectforeground", "noFocusSelectForeground", 
   "NoFocusSelectForeground", 
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(Legend, selOutFocusFgColor), 0, NULL,0},
  {TK_OPTION_PIXELS, "-padx", "padX", "Pad", 
   "1", -1, Tk_Offset(Legend, xPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-pady", "padY", "Pad", 
   "1", -1, Tk_Offset(Legend, yPad), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-position", "position", "Position", 
   "rightmargin", -1, 0, 0, &positionObjOption, 0},
  {TK_OPTION_BOOLEAN, "-raised", "raised", "Raised", 
   "no", -1, Tk_Offset(Legend, raised), 0, NULL, 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief", 
   "flat", -1, Tk_Offset(Legend, relief), 0, NULL, 0},
  {TK_OPTION_INT, "-rows", "rows", "rows", 
   "0", -1, Tk_Offset(Legend, reqRows), 0, NULL, 0},
  {TK_OPTION_BORDER, "-selectbackground", "selectBackground", 
   "SelectBackground", 
   STD_ACTIVE_BACKGROUND, -1, Tk_Offset(Legend, selInFocusBg), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth", 
   "SelectBorderWidth", 
   "1", -1, Tk_Offset(Legend, selBW), 0, NULL, 0},
  {TK_OPTION_STRING, "-selectcommand", "selectCommand", "SelectCommand",
   NULL, -1, Tk_Offset(Legend, selectCmd), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "SelectForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(Legend, selInFocusFgColor), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-selectmode", "selectMode", "SelectMode",
   "multiple", -1, Tk_Offset(Legend, selectMode), 0, &selectmodeObjOption, 0},
  {TK_OPTION_RELIEF, "-selectrelief", "selectRelief", "SelectRelief",
   "flat", -1, Tk_Offset(Legend, selRelief), 0, NULL, 0},
  {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus", 
   NULL, -1, Tk_Offset(Legend, takeFocus), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title", 
   NULL, -1, Tk_Offset(Legend, title), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_ANCHOR, "-titleanchor", "titleAnchor", "TitleAnchor", 
   "nw", -1, Tk_Offset(Legend, titleStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-titlecolor", "titleColor", "TitleColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Legend, titleStyle.color), 0, NULL, 0},
  {TK_OPTION_FONT, "-titlefont", "titleFont", "TitleFont",
   STD_FONT_SMALL, -1, Tk_Offset(Legend, titleStyle.font), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

int Blt_CreateLegend(Graph* graphPtr)
{
  Legend* legendPtr = (Legend*)calloc(1, sizeof(Legend));
  graphPtr->legend = legendPtr;
  legendPtr->graphPtr = graphPtr;
  legendPtr->tkwin = graphPtr->tkwin;
  legendPtr->xReq = -SHRT_MAX;
  legendPtr->yReq = -SHRT_MAX;
  Blt_Ts_InitStyle(legendPtr->style);
  Blt_Ts_InitStyle(legendPtr->titleStyle);
  legendPtr->style.justify = TK_JUSTIFY_LEFT;
  legendPtr->style.anchor = TK_ANCHOR_NW;
  legendPtr->titleStyle.justify = TK_JUSTIFY_LEFT;
  legendPtr->titleStyle.anchor = TK_ANCHOR_NW;
  legendPtr->bindTable = 
    Blt_CreateBindingTable(graphPtr->interp, graphPtr->tkwin, 
			   graphPtr, PickEntryProc, Blt_GraphTags);

  Tcl_InitHashTable(&legendPtr->selectTable, TCL_ONE_WORD_KEYS);
  legendPtr->selected = Blt_Chain_Create();
  Tk_CreateSelHandler(legendPtr->tkwin, XA_PRIMARY, XA_STRING, 
		      SelectionProc, legendPtr, XA_STRING);
  legendPtr->onTime = 600;
  legendPtr->offTime = 300;

  legendPtr->optionTable =Tk_CreateOptionTable(graphPtr->interp, optionSpecs);
  return Tk_InitOptions(graphPtr->interp, (char*)legendPtr, 
			legendPtr->optionTable, graphPtr->tkwin);
}

void Blt_DestroyLegend(Graph* graphPtr)
{
  Legend* legendPtr = graphPtr->legend;
  if (!legendPtr)
    return;

  Blt_Ts_FreeStyle(graphPtr->display, &legendPtr->style);
  Blt_Ts_FreeStyle(graphPtr->display, &legendPtr->titleStyle);
  Blt_DestroyBindingTable(legendPtr->bindTable);
    
  if (legendPtr->focusGC)
    Blt_FreePrivateGC(graphPtr->display, legendPtr->focusGC);

  if (legendPtr->timerToken)
    Tcl_DeleteTimerHandler(legendPtr->timerToken);

  if (legendPtr->tkwin)
    Tk_DeleteSelHandler(legendPtr->tkwin, XA_PRIMARY, XA_STRING);

  if (legendPtr->site == LEGEND_WINDOW) {
    Tk_Window tkwin;
	
    /* The graph may be in the process of being torn down */
    if (legendPtr->cmdToken) {
      Tcl_DeleteCommandFromToken(graphPtr->interp, legendPtr->cmdToken);

      if (legendPtr->flags & REDRAW_PENDING) {
	Tcl_CancelIdleCall(DisplayLegend, legendPtr);
	legendPtr->flags &= ~REDRAW_PENDING;
      }
      tkwin = legendPtr->tkwin;
      legendPtr->tkwin = NULL;
      if (tkwin) {
	Tk_DeleteEventHandler(tkwin, ExposureMask | StructureNotifyMask,
			      LegendEventProc, graphPtr);
	Tk_DestroyWindow(tkwin);
      }
    }
  }

  Tk_FreeConfigOptions((char*)legendPtr, legendPtr->optionTable, 
		       graphPtr->tkwin);
  free(legendPtr);
}

static void LegendEventProc(ClientData clientData, XEvent *eventPtr)
{
  Graph* graphPtr = (Graph*)clientData;
  Legend* legendPtr;

  legendPtr = graphPtr->legend;
  if (eventPtr->type == Expose) {
    if (eventPtr->xexpose.count == 0) {
      Blt_Legend_EventuallyRedraw(graphPtr);
    }
  } 
  else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
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
	legendPtr->timerToken = Tcl_CreateTimerHandler(legendPtr->onTime,
						       BlinkCursorProc,
						       graphPtr);
      }
    } 
    else {
      legendPtr->cursorOn = FALSE;
      legendPtr->timerToken = (Tcl_TimerToken)NULL;
    }
    Blt_Legend_EventuallyRedraw(graphPtr);
  } 
  else if (eventPtr->type == DestroyNotify) {
    Graph* graphPtr = legendPtr->graphPtr;

    if (legendPtr->site == LEGEND_WINDOW) {
      if (legendPtr->cmdToken) {
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
    legendPtr->hide = 1;
    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
    Blt_MoveBindingTable(legendPtr->bindTable, graphPtr->tkwin);
    Blt_EventuallyRedrawGraph(graphPtr);
  } 
  else if (eventPtr->type == ConfigureNotify) {
    Blt_Legend_EventuallyRedraw(graphPtr);
  }
}

// Configure

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Legend* legendPtr = graphPtr->legend;
  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)legendPtr, 
				      legendPtr->optionTable,
				      objv[3], graphPtr->tkwin);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp,
		       int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, (char*)legendPtr, 
				       legendPtr->optionTable, 
				       (objc == 4) ? objv[3] : NULL, 
				       graphPtr->tkwin);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return LegendObjConfigure(interp, graphPtr, objc-3, objv+3);
}

static int LegendObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			      int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)legendPtr, legendPtr->optionTable, 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    graphPtr->flags |= mask;
    graphPtr->flags |= (RESET_WORLD | CACHE_DIRTY);
    ConfigureLegend(graphPtr);
    Blt_EventuallyRedrawGraph(graphPtr);

    break; 
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

static void ConfigureLegend(Graph* graphPtr)
{
  Legend* legendPtr = graphPtr->legend;

  /* GC for active label. Dashed outline. */
  unsigned long gcMask = GCForeground | GCLineStyle;
  XGCValues gcValues;
  gcValues.foreground = legendPtr->focusColor->pixel;
  gcValues.line_style = (LineIsDashed(legendPtr->focusDashes))
    ? LineOnOffDash : LineSolid;
  GC newGC = Blt_GetPrivateGC(legendPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(legendPtr->focusDashes)) {
    legendPtr->focusDashes.offset = 2;
    Blt_SetDashes(graphPtr->display, newGC, &legendPtr->focusDashes);
  }
  if (legendPtr->focusGC)
    Blt_FreePrivateGC(graphPtr->display, legendPtr->focusGC);

  legendPtr->focusGC = newGC;
}

// Ops

static int ActivateOp(Graph* graphPtr, Tcl_Interp* interp, 
		      int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
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
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
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
  if ((redraw) && ((legendPtr->hide) == 0)) {
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
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
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

static int BindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.tagTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      const char *tagName = 
	(const char*)Tcl_GetHashKey(&graphPtr->elements.tagTable, hPtr);
      Tcl_Obj *objPtr = Tcl_NewStringObj(tagName, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  return Blt_ConfigureBindingsFromObj(interp, graphPtr->legend->bindTable, Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}

static int CurselectionOp(Graph* graphPtr, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (legendPtr->flags & SELECT_SORTED) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(legendPtr->selected); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
  } else {
    Blt_ChainLink link;

    /* List of selected entries is in stacking order. */
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);

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

static int FocusOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;

  if (objc == 4) {
    Element* elemPtr;

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
  if (legendPtr->focusPtr) {
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		     legendPtr->focusPtr->obj.name, -1);
  }
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;

  if (((legendPtr->hide) == 0) && (legendPtr->nEntries > 0)) {
    Element* elemPtr;

    if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
      return TCL_ERROR;
    }
    if (elemPtr) {
      Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name,-1);
    }
  }
  return TCL_OK;
}

static Blt_OpSpec legendOps[] =
  {
    {"activate",     1, (void*)ActivateOp,      3, 0, "?pattern?...",},
    {"bind",         1, (void*)BindOp,          3, 6, "elem sequence command",},
    {"cget",         2, (void*)CgetOp,          4, 4, "option",},
    {"configure",    2, (void*)ConfigureOp,     3, 0, "?option value?...",},
    {"curselection", 2, (void*)CurselectionOp,  3, 3, "",},
    {"deactivate",   1, (void*)ActivateOp,      3, 0, "?pattern?...",},
    {"focus",        1, (void*)FocusOp,         4, 4, "elem",},
    {"get",          1, (void*)GetOp,           4, 4, "elem",},
    {"selection",    1, (void*)SelectionOp,     3, 0, "args"},
  };
static int nLegendOps = sizeof(legendOps) / sizeof(Blt_OpSpec);

int Blt_LegendOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  GraphLegendProc *proc = (GraphLegendProc*)Blt_GetOpFromObj(interp, nLegendOps, legendOps, BLT_OP_ARG2, objc, objv,0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(graphPtr, interp, objc, objv);
}

// Selection Widget Ops

static int SelectionAnchorOp(Graph* graphPtr, Tcl_Interp* interp, 
			     int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  Element* elemPtr;

  if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  /* Set both the anchor and the mark. Indicates that a single entry
   * is selected. */
  legendPtr->selAnchorPtr = elemPtr;
  legendPtr->selMarkPtr = NULL;
  if (elemPtr) {
    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
  }
  Blt_Legend_EventuallyRedraw(graphPtr);
  return TCL_OK;
}

static int SelectionClearallOp(Graph* graphPtr, Tcl_Interp* interp, 
			       int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;

  ClearSelection(legendPtr);
  return TCL_OK;
}

static int SelectionIncludesOp(Graph* graphPtr, Tcl_Interp* interp, 
			       int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  Element* elemPtr;
  if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  int boo = EntryIsSelected(legendPtr, elemPtr);
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), boo);
  return TCL_OK;
}

static int SelectionMarkOp(Graph* graphPtr, Tcl_Interp* interp, 
			   int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  Element* elemPtr;

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
      next = Blt_Chain_PrevLink(link);
      Element *selectPtr = (Element*)Blt_Chain_GetValue(link);
      if (selectPtr == legendPtr->selAnchorPtr) {
	break;
      }
      DeselectElement(legendPtr, selectPtr);
    }
    legendPtr->flags &= ~SELECT_TOGGLE;
    legendPtr->flags |= SELECT_SET;
    SelectRange(legendPtr, legendPtr->selAnchorPtr, elemPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
    legendPtr->selMarkPtr = elemPtr;

    Blt_Legend_EventuallyRedraw(graphPtr);
    if (legendPtr->selectCmd) {
      EventuallyInvokeSelectCmd(legendPtr);
    }
  }
  return TCL_OK;
}

static int SelectionPresentOp(Graph* graphPtr, Tcl_Interp* interp, 
			      int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  int boo = (Blt_Chain_GetLength(legendPtr->selected) > 0);
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), boo);
  return TCL_OK;
}

static int SelectionSetOp(Graph* graphPtr, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend;
  Element *firstPtr, *lastPtr;
  const char *string;

  legendPtr->flags &= ~SELECT_TOGGLE;
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
  if ((firstPtr->hide) && ((legendPtr->flags & SELECT_CLEAR)==0)) {
    Tcl_AppendResult(interp, "can't select hidden node \"", 
		     Tcl_GetString(objv[4]), "\"", (char *)NULL);
    return TCL_ERROR;
  }
  lastPtr = firstPtr;
  if (objc > 5) {
    if (GetElementFromObj(graphPtr, objv[5], &lastPtr) != TCL_OK) {
      return TCL_ERROR;
    }
    if (lastPtr->hide && ((legendPtr->flags & SELECT_CLEAR) == 0)) {
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
  if (legendPtr->exportSelection) {
    Tk_OwnSelection(legendPtr->tkwin, XA_PRIMARY, LostSelectionProc, 
		    legendPtr);
  }
  Blt_Legend_EventuallyRedraw(graphPtr);
  if (legendPtr->selectCmd) {
    EventuallyInvokeSelectCmd(legendPtr);
  }
  return TCL_OK;
}

static Blt_OpSpec selectionOps[] =
  {
    {"anchor",   1, (void*)SelectionAnchorOp,   5, 5, "elem",},
    {"clear",    5, (void*)SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"clearall", 6, (void*)SelectionClearallOp, 4, 4, "",},
    {"includes", 1, (void*)SelectionIncludesOp, 5, 5, "elem",},
    {"mark",     1, (void*)SelectionMarkOp,     5, 5, "elem",},
    {"present",  1, (void*)SelectionPresentOp,  4, 4, "",},
    {"set",      1, (void*)SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"toggle",   1, (void*)SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
  };
static int nSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int SelectionOp(Graph* graphPtr, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  GraphLegendProc* proc = (GraphLegendProc*)Blt_GetOpFromObj(interp, nSelectionOps, selectionOps, BLT_OP_ARG3, objc, objv, 0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(graphPtr, interp, objc, objv);
}

// Support

static void DisplayLegend(ClientData clientData)
{
  Legend* legendPtr = (Legend*)clientData;
  Graph* graphPtr;

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

void Blt_Legend_EventuallyRedraw(Graph* graphPtr) 
{
  Legend* legendPtr = graphPtr->legend;

  if ((legendPtr->tkwin) && !(legendPtr->flags & REDRAW_PENDING)) {
    Tcl_DoWhenIdle(DisplayLegend, legendPtr);
    legendPtr->flags |= REDRAW_PENDING;
  }
}

static void SelectCmdProc(ClientData clientData) 
{
  Legend* legendPtr = (Legend*)clientData;

  Tcl_Preserve(legendPtr);
  legendPtr->flags &= ~SELECT_PENDING;
  if (legendPtr->selectCmd) {
    Tcl_Interp* interp;

    interp = legendPtr->graphPtr->interp;
    if (Tcl_GlobalEval(interp, legendPtr->selectCmd) != TCL_OK) {
      Tcl_BackgroundError(interp);
    }
  }
  Tcl_Release(legendPtr);
}

static void EventuallyInvokeSelectCmd(Legend* legendPtr)
{
  if ((legendPtr->flags & SELECT_PENDING) == 0) {
    legendPtr->flags |= SELECT_PENDING;
    Tcl_DoWhenIdle(SelectCmdProc, legendPtr);
  }
}

static void ClearSelection(Legend* legendPtr)
{
  Tcl_DeleteHashTable(&legendPtr->selectTable);
  Tcl_InitHashTable(&legendPtr->selectTable, TCL_ONE_WORD_KEYS);
  Blt_Chain_Reset(legendPtr->selected);
  Blt_Legend_EventuallyRedraw(legendPtr->graphPtr);
  if (legendPtr->selectCmd) {
    EventuallyInvokeSelectCmd(legendPtr);
  }
}

static void LostSelectionProc(ClientData clientData)
{
  Legend* legendPtr = (Legend*)clientData;

  if (legendPtr->exportSelection)
    ClearSelection(legendPtr);
}

static int CreateLegendWindow(Tcl_Interp* interp, Legend* legendPtr, 
			      const char *pathName)
{
  Graph* graphPtr;
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
					     Blt_GraphInstCmdProc, 
					     graphPtr, NULL);
  legendPtr->tkwin = tkwin;
  legendPtr->site = LEGEND_WINDOW;
  return TCL_OK;
}

static void SetLegendOrigin(Legend* legendPtr)
{
  Graph* graphPtr;
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
    if (graphPtr->title) {
      h -= graphPtr->titleHeight;
    }
    x = graphPtr->left;
    y = graphPtr->inset;
    if (graphPtr->title) {
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

static int EntryIsSelected(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry*hPtr = 
    Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
  return (hPtr != NULL);
}

static void SelectElement(Legend* legendPtr, Element* elemPtr)
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&legendPtr->selectTable, (char *)elemPtr,&isNew);
  if (isNew) {
    Blt_ChainLink link = Blt_Chain_Append(legendPtr->selected, elemPtr);
    Tcl_SetHashValue(hPtr, link);
  }
}

static void DeselectElement(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry* hPtr = 
    Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
  if (hPtr) {
    Blt_ChainLink link = (Blt_ChainLink)Tcl_GetHashValue(hPtr);
    Blt_Chain_DeleteLink(legendPtr->selected, link);
    Tcl_DeleteHashEntry(hPtr);
  }
}

static void SelectEntry(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry *hPtr;

  switch (legendPtr->flags & SELECT_TOGGLE) {
  case SELECT_CLEAR:
    DeselectElement(legendPtr, elemPtr);
    break;

  case SELECT_SET:
    SelectElement(legendPtr, elemPtr);
    break;

  case SELECT_TOGGLE:
    hPtr = Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
    if (hPtr) {
      DeselectElement(legendPtr, elemPtr);
    } else {
      SelectElement(legendPtr, elemPtr);
    }
    break;
  }
}

static ClientData PickEntryProc(ClientData clientData, int x, int y, 
				ClientData *contextPtr)
{
  Graph* graphPtr = (Graph*)clientData;
  Legend* legendPtr;
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
	Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
	if (elemPtr->label) {
	  if (count == n) {
	    return elemPtr;
	  }
	  count++;
	}
      }	      
      if (link) {
	return Blt_Chain_GetValue(link);
      }	
    }
  }
  return NULL;
}

void Blt_MapLegend(Graph* graphPtr, int plotWidth, int plotHeight)
{
  Legend* legendPtr = graphPtr->legend;
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
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL) {
      continue;			/* Element has no legend entry. */
    }
    Blt_Ts_GetExtents(&legendPtr->style, elemPtr->label, &w, &h);
    if (maxWidth < (int)w) {
      maxWidth = w;
    }
    if (maxHeight < (int)h) {
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
  if (lw < (int)(legendPtr->titleWidth)) {
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
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
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

void Blt_DrawLegend(Graph* graphPtr, Drawable drawable)
{
  Blt_ChainLink link;
  Tk_FontMetrics fontMetrics;
  Legend* legendPtr = graphPtr->legend;
  Pixmap pixmap;
  Tk_Window tkwin;
  int count;
  int symbolSize, xMid, yMid;
  int x, y, w, h;
  int xLabel, yStart, xSymbol, ySymbol;

  if ((legendPtr->hide) || (legendPtr->nEntries == 0)) {
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

  if (legendPtr->normalBg) {
    Tk_Fill3DRectangle(tkwin, pixmap, legendPtr->normalBg, 0, 0, 
		       w, h, 0, TK_RELIEF_FLAT);
  }
  else if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
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
      Tk_Fill3DRectangle(tkwin, pixmap, graphPtr->plotBg, 0, 0, 
			 w, h, TK_RELIEF_FLAT, 0);
    }
  }
  else {
    /* 
     * The legend is located in one of the margins or the external window.
     */
    Tk_Fill3DRectangle(tkwin, pixmap, graphPtr->normalBg, 0, 0, 
		       w, h, 0, TK_RELIEF_FLAT);
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
    int isSelected;

    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL) {
      continue;			/* Skip this entry */
    }
    isSelected = EntryIsSelected(legendPtr, elemPtr);
    if (elemPtr->flags & LABEL_ACTIVE) {
      Tk_Fill3DRectangle(tkwin, pixmap, legendPtr->activeBg, 
			 x, y, legendPtr->entryWidth, legendPtr->entryHeight, 
			 legendPtr->entryBW, legendPtr->activeRelief);
    } else if (isSelected) {
      XColor* fg = (legendPtr->flags & FOCUS) ?
	legendPtr->selInFocusFgColor : legendPtr->selOutFocusFgColor;
      Tk_3DBorder bg = (legendPtr->flags & FOCUS) ?
	legendPtr->selInFocusBg : legendPtr->selOutFocusBg;
      Blt_Ts_SetForeground(legendPtr->style, fg);
      Tk_Fill3DRectangle(tkwin, pixmap, bg, x, y, 
			 legendPtr->entryWidth, legendPtr->entryHeight, 
			 legendPtr->selBW, legendPtr->selRelief);
    } else {
      Blt_Ts_SetForeground(legendPtr->style, legendPtr->fgColor);
      if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
	Tk_Fill3DRectangle(tkwin, pixmap, graphPtr->normalBg, 
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
	XColor* color;

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
  Tk_3DBorder bg = legendPtr->normalBg;
  if (bg == NULL)
    bg = graphPtr->normalBg;

  /* Disable crosshairs before redisplaying to the screen */
  if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
    Blt_DisableCrosshairs(graphPtr);
  }
  Tk_Draw3DRectangle(tkwin, pixmap, bg, 0, 0, w, h, 
		     legendPtr->borderWidth, legendPtr->relief);
  XCopyArea(graphPtr->display, pixmap, drawable, graphPtr->drawGC, 0, 0, w, h,
	    legendPtr->x, legendPtr->y);
  if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
    Blt_EnableCrosshairs(graphPtr);
  }
  Tk_FreePixmap(graphPtr->display, pixmap);
  graphPtr->flags &= ~DRAW_LEGEND;
}

void Blt_LegendToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Legend* legendPtr = graphPtr->legend;
  double x, y, yStart;
  int xLabel, xSymbol, ySymbol;
  int count;
  Blt_ChainLink link;
  int symbolSize, xMid, yMid;
  int width, height;
  Tk_FontMetrics fontMetrics;

  if ((legendPtr->hide) || (legendPtr->nEntries == 0)) {
    return;
  }
  SetLegendOrigin(legendPtr);

  x = legendPtr->x, y = legendPtr->y;
  width = legendPtr->width - 2*legendPtr->xPad;
  height = legendPtr->height - 2*legendPtr->yPad;

  Blt_Ps_Append(ps, "% Legend\n");
  graphPtr = legendPtr->graphPtr;
  if (graphPtr->pageSetup->decorations) {
    if (legendPtr->normalBg)
      Blt_Ps_Fill3DRectangle(ps, legendPtr->normalBg, x, y, width, height, 
			     legendPtr->borderWidth, legendPtr->relief);
    else
      Blt_Ps_Draw3DRectangle(ps, graphPtr->normalBg, x, y, width, height, 
			     legendPtr->borderWidth, legendPtr->relief);

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
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL) {
      continue;			/* Skip this label */
    }
    if (elemPtr->flags & LABEL_ACTIVE) {
      Blt_Ts_SetForeground(legendPtr->style, legendPtr->activeFgColor);
      Blt_Ps_Fill3DRectangle(ps, legendPtr->activeBg, x, y, legendPtr->entryWidth, 
			     legendPtr->entryHeight, legendPtr->entryBW, 
			     legendPtr->activeRelief);
    } else {
      Blt_Ts_SetForeground(legendPtr->style, legendPtr->fgColor);
      if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
	Blt_Ps_Draw3DRectangle(ps, graphPtr->normalBg, x, y, 
			       legendPtr->entryWidth,
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

static Element *GetNextRow(Graph* graphPtr, Element *focusPtr)
{
  Blt_ChainLink link;
  int row, col;

  col = focusPtr->col;
  row = focusPtr->row + 1;
  for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL) {
      continue;
    }
    if ((elemPtr->col == col) && (elemPtr->row == row)) {
      return elemPtr;	
    }
  }
  return NULL;
}

static Element *GetNextColumn(Graph* graphPtr, Element *focusPtr)
{
  Blt_ChainLink link;
  int row, col;

  col = focusPtr->col + 1;
  row = focusPtr->row;
  for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL) {
      continue;
    }
    if ((elemPtr->col == col) && (elemPtr->row == row)) {
      return elemPtr;		/* Don't go beyond legend boundaries. */
    }
  }
  return NULL;
}

static Element *GetPreviousRow(Graph* graphPtr, Element *focusPtr)
{
  int col = focusPtr->col;
  int row = focusPtr->row - 1;
  for (Blt_ChainLink link = focusPtr->link; link != NULL; 
       link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL)
      continue;

    if ((elemPtr->col == col) && (elemPtr->row == row))
      return elemPtr;	
  }
  return NULL;
}

static Element *GetPreviousColumn(Graph* graphPtr, Element *focusPtr)
{
  int col = focusPtr->col - 1;
  int row = focusPtr->row;
  for (Blt_ChainLink link = focusPtr->link; link != NULL;
       link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label == NULL)
      continue;

    if ((elemPtr->col == col) && (elemPtr->row == row))
      return elemPtr;	
  }
  return NULL;
}

static Element *GetFirstElement(Graph* graphPtr)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements.displayList);link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label)
      return elemPtr;
  }
  return NULL;
}

static Element *GetLastElement(Graph* graphPtr)
{
  for (Blt_ChainLink link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (elemPtr->label)
      return elemPtr;
  }
  return NULL;
}

static int GetElementFromObj(Graph* graphPtr, Tcl_Obj *objPtr, 
			     Element **elemPtrPtr)
{
  Element* elemPtr;
  Legend* legendPtr;
  Tcl_Interp* interp;
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

static int SelectRange(Legend* legendPtr, Element *fromPtr, Element *toPtr)
{
  if (Blt_Chain_IsBefore(fromPtr->link, toPtr->link)) {
    for (Blt_ChainLink link = fromPtr->link; link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      SelectEntry(legendPtr, elemPtr);
      if (link == toPtr->link) {
	break;
      }
    }
  } 
  else {
    for (Blt_ChainLink link = fromPtr->link; link != NULL;
	 link = Blt_Chain_PrevLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      SelectEntry(legendPtr, elemPtr);
      if (link == toPtr->link) {
	break;
      }
    }
  }
  return TCL_OK;
}

int Blt_Legend_Site(Graph* graphPtr)
{
  return graphPtr->legend->site;
}

int Blt_Legend_Width(Graph* graphPtr)
{
  return graphPtr->legend->width;
}

int Blt_Legend_Height(Graph* graphPtr)
{
  return graphPtr->legend->height;
}

int Blt_Legend_IsHidden(Graph* graphPtr)
{
  return (graphPtr->legend->hide);
}

int Blt_Legend_IsRaised(Graph* graphPtr)
{
  return (graphPtr->legend->raised);
}

int Blt_Legend_X(Graph* graphPtr)
{
  return graphPtr->legend->x;
}

int Blt_Legend_Y(Graph* graphPtr)
{
  return graphPtr->legend->y;
}

void Blt_Legend_RemoveElement(Graph* graphPtr, Element* elemPtr)
{
  Blt_DeleteBindings(graphPtr->legend->bindTable, elemPtr);
}

static int SelectionProc(ClientData clientData, int offset, 
			 char *buffer, int maxBytes)
{
  Legend* legendPtr = (Legend*)clientData;
  int nBytes;
  Tcl_DString dString;

  if ((legendPtr->exportSelection) == 0) {
    return -1;
  }
  /* Retrieve the names of the selected entries. */
  Tcl_DStringInit(&dString);
  if (legendPtr->flags & SELECT_SORTED) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(legendPtr->selected); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      Tcl_DStringAppend(&dString, elemPtr->obj.name, -1);
      Tcl_DStringAppend(&dString, "\n", -1);
    }
  } else {
    Blt_ChainLink link;
    Graph* graphPtr;

    graphPtr = legendPtr->graphPtr;
    /* List of selected entries is in stacking order. */
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
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

static void BlinkCursorProc(ClientData clientData)
{
  Graph* graphPtr = (Graph*)clientData;
  Legend* legendPtr = graphPtr->legend;
  if (!(legendPtr->flags & FOCUS) || (legendPtr->offTime == 0))
    return;

  if (legendPtr->active) {
    int time;

    legendPtr->cursorOn ^= 1;
    time = (legendPtr->cursorOn) ? legendPtr->onTime : legendPtr->offTime;
    legendPtr->timerToken = 
      Tcl_CreateTimerHandler(time, BlinkCursorProc, graphPtr);
    Blt_Legend_EventuallyRedraw(graphPtr);
  }
}
