/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __BltGrLegend_h__
#define __BltGrLegend_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

extern "C" {
#include "bltGraph.h"
};

#define LEGEND_RIGHT	(1<<0)	/* Right margin */
#define LEGEND_LEFT	(1<<1)	/* Left margin */
#define LEGEND_BOTTOM	(1<<2)	/* Bottom margin */
#define LEGEND_TOP	(1<<3)	/* Top margin, below the graph title. */
#define LEGEND_PLOT	(1<<4)	/* Plot area */
#define LEGEND_XY	(1<<5)	/* Screen coordinates in the plotting 
				 * area. */
#define LEGEND_WINDOW	(1<<6)	/* External window. */
#define LEGEND_MARGIN_MASK \
	(LEGEND_RIGHT | LEGEND_LEFT | LEGEND_BOTTOM | LEGEND_TOP)
#define LEGEND_PLOTAREA_MASK  (LEGEND_PLOT | LEGEND_XY)

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

extern int Blt_CreateLegend(Graph *graphPtr);
extern void Blt_DestroyLegend(Graph *graphPtr);

extern void Blt_DrawLegend(Graph *graphPtr, Drawable drawable);
extern void Blt_MapLegend(Graph *graphPtr, int width, int height);
extern int Blt_LegendOp(Graph *graphPtr, Tcl_Interp* interp, int objc, 
	Tcl_Obj* const objv[]);
extern int Blt_Legend_Site(Graph *graphPtr);
extern int Blt_Legend_Width(Graph *graphPtr);
extern int Blt_Legend_Height(Graph *graphPtr);
extern int Blt_Legend_IsHidden(Graph *graphPtr);
extern int Blt_Legend_IsRaised(Graph *graphPtr);
extern int Blt_Legend_X(Graph *graphPtr);
extern int Blt_Legend_Y(Graph *graphPtr);
extern void Blt_Legend_RemoveElement(Graph *graphPtr, Element *elemPtr);
extern void Blt_Legend_EventuallyRedraw(Graph *graphPtr);

#endif
