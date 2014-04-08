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

#define LEGEND_RIGHT	(1<<0)	// Right margin
#define LEGEND_LEFT	(1<<1)	// Left margin
#define LEGEND_BOTTOM	(1<<2)	// Bottom margin
#define LEGEND_TOP	(1<<3)	// Top margin, below the graph title
#define LEGEND_PLOT	(1<<4)	// Plot area
#define LEGEND_XY	(1<<5)	// Screen coordinates in the plotting area
#define LEGEND_MARGIN_MASK (LEGEND_RIGHT|LEGEND_LEFT|LEGEND_BOTTOM|LEGEND_TOP)
#define LEGEND_PLOTAREA_MASK  (LEGEND_PLOT | LEGEND_XY)

//  Selection related flags:
//	SELECT_PENDING		A "selection" command idle task is pending.
//	SELECT_CLEAR		Clear selection flag of entry.
//	SELECT_SET		Set selection flag of entry.
//	SELECT_TOGGLE		Toggle selection flag of entry.
//			        Mask of selection set/clear/toggle flags.
//	SELECT_SORTED		Indicates if the entries in the selection 
//				should be sorted or displayed in the order 
//				they were selected.

#define SELECT_CLEAR		(1<<24)
#define SELECT_PENDING		(1<<25)
#define SELECT_SET		(1<<26)
#define SELECT_SORTED		(1<<27)
#define SELECT_TOGGLE		(SELECT_SET | SELECT_CLEAR)

typedef enum {
  SELECT_MODE_SINGLE, SELECT_MODE_MULTIPLE
} SelectMode;

class Legend;

typedef struct {
  Legend* legendPtr;

  Tk_3DBorder activeBg;
  XColor* activeFgColor;
  int activeRelief;
  Tk_3DBorder normalBg;
  XColor* fgColor;
  Tk_Anchor anchor;
  int borderWidth;
  int reqColumns;
  int exportSelection;
  Blt_Dashes focusDashes;
  XColor* focusColor;
  TextStyle style;
  int hide;
  int ixPad;
  int iyPad;
  int xPad;
  int yPad;
  int raised;
  int relief;
  int reqRows;
  int entryBW;
  int selBW;
  const char *selectCmd;
  Tk_3DBorder selOutFocusBg;
  Tk_3DBorder selInFocusBg;
  XColor* selOutFocusFgColor;
  XColor* selInFocusFgColor;
  SelectMode selectMode;
  int selRelief;
  const char *takeFocus;
  const char *title;
  TextStyle titleStyle;
} LegendOptions;

class Legend {
 public:
  Graph* graphPtr_;
  Tk_OptionTable optionTable_;
  void* ops_;

  unsigned int flags;
  int nEntries;
  int nColumns;
  int nRows;
  int width;
  int height;
  int entryWidth;
  int entryHeight;
  int site;
  int xReq;
  int yReq;
  int x;
  int y;
  Tk_Window tkwin;
  int maxSymSize;
  Blt_BindTable bindTable;
  GC focusGC;
  int focus;
  int cursorX;
  int cursorY;
  short int cursorWidth;
  short int cursorHeight;
  Element *focusPtr;
  Element *selAnchorPtr;
  Element *selMarkPtr;
  Element *selFirstPtr;
  Element *selLastPtr;
  int active;
  int cursorOn;
  int onTime;
  int offTime;
  Tcl_TimerToken timerToken;
  Tcl_HashTable selectTable;
  Blt_Chain selected;
  unsigned int titleWidth;
  unsigned int titleHeight;

 public:
  Legend();
  virtual ~Legend();
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
