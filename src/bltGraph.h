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

#ifndef __BltGraph_h__
#define __BltGraph_h__

#include "bltInt.h"
#include "bltBind.h"
#include "bltChain.h"
#include "bltPs.h"

#define MARGIN_NONE	-1
#define MARGIN_BOTTOM	0		/* x */
#define MARGIN_LEFT	1 		/* y */
#define MARGIN_TOP	2		/* x2 */
#define MARGIN_RIGHT	3		/* y2 */

#define rightMargin	margins[MARGIN_RIGHT]
#define leftMargin	margins[MARGIN_LEFT]
#define topMargin	margins[MARGIN_TOP]
#define bottomMargin	margins[MARGIN_BOTTOM]

typedef struct _Graph Graph;
class Crosshairs;
class Axis;
class Element;
class Legend;

typedef struct {
  Axis *x, *y;
} Axis2d;

typedef enum {
  CID_NONE, CID_AXIS_X, CID_AXIS_Y, CID_ELEM_BAR, CID_ELEM_LINE,
  CID_MARKER_BITMAP, CID_MARKER_IMAGE, CID_MARKER_LINE, CID_MARKER_POLYGON,
  CID_MARKER_TEXT, CID_MARKER_WINDOW, CID_LEGEND_ENTRY,
} ClassId;

typedef struct {
  int halo;
  int mode;
  int x;
  int y;
  int along;

  Element* elemPtr;
  Point2d point;
  int index;
  double dist;
} ClosestSearch;

typedef struct {
  Segment2d *segments;
  int length;
  int *map;
} GraphSegments;

typedef struct {
  int nSegments;
  Axis2d axes;
  float sum;
  int count;
  float lastY;
  size_t index;
} BarGroup;

typedef struct {
  float value;
  Axis2d axes;
} BarSetKey;

typedef enum BarModes {
  BARS_INFRONT, BARS_STACKED, BARS_ALIGNED, BARS_OVERLAP
} BarMode;

typedef struct {
  short int width;
  short int height;
  short int axesOffset;
  short int axesTitleLength;
  short int maxTickWidth;
  short int maxTickHeight;
  unsigned int nAxes;
  Blt_Chain axes;
  const char *varName;
  int reqSize;
  int site;
} Margin;

struct _Graph {
  Tcl_Interp* interp;
  Tk_Window tkwin;
  Display *display;
  Tcl_Command cmdToken;
  Tk_OptionTable optionTable;
  ClassId classId;
  Tk_Cursor cursor;
  int inset;
  int borderWidth;
  int relief;
  unsigned int flags;
  Tk_3DBorder normalBg;
  int highlightWidth;
  XColor* highlightBgColor;
  XColor* highlightColor;
  const char *title;
  short int titleX;
  short int titleY;
  short int titleWidth;
  short int titleHeight;
  TextStyle titleTextStyle;
  const char *takeFocus; // nor used in C code
  Axis *focusPtr;
  int reqWidth;
  int reqHeight;
  int reqPlotWidth;
  int reqPlotHeight;
  int width;
  int height;
  Tcl_HashTable penTable;

  struct Component {
    Tcl_HashTable table;
    Blt_Chain displayList;
    Tcl_HashTable tagTable;
  } elements, markers, axes;

  Tcl_HashTable dataTables;
  Blt_BindTable bindTable;
  int nextMarkerId;
  Blt_Chain axisChain[4];
  Margin margins[4];
  PageSetup *pageSetup;
  Legend *legend;
  Crosshairs *crosshairs;

  int halo;
  int inverted;
  int stackAxes;
  GC drawGC;
  int plotBW;
  int plotRelief;
  Tk_3DBorder plotBg;
  double aspect;
  short int left;
  short int right;
  short int top;
  short int bottom;	
  int xPad;
  int vRange;
  int vOffset;
  int yPad;
  int hRange;
  int hOffset;
  float vScale;
  float hScale;
  int doubleBuffer;
  int backingStore;
  Pixmap cache;
  short int cacheWidth;
  short int cacheHeight;
  ClosestSearch search;

  // barchart specific information
  double baseline;
  double barWidth;
  BarMode barMode;
  BarGroup *barGroups;
  int nBarGroups;
  Tcl_HashTable setTable;
  int maxBarSetSize;
};

/*
 * Bit flags definitions:
 *
 * 	All kinds of state information kept here.  All these things happen
 * 	when the window is available to draw into (DisplayGraph). Need the
 * 	window width and height before we can calculate graph layout (i.e. the
 * 	screen coordinates of the axes, elements, titles, etc). But we want to
 * 	do this only when we have to, not every time the graph is redrawn.
 *
 *	Same goes for maintaining a pixmap to double buffer graph elements.
 *	Need to mark when the pixmap needs to updated.
 *
 *
 *	MAP_ITEM		Indicates that the element/marker/axis
 *				configuration has changed such that
 *				its layout of the item (i.e. its
 *				position in the graph window) needs
 *				to be recalculated.
 *
 *	MAP_ALL			Indicates that the layout of the axes and 
 *				all elements and markers and the graph need 
 *				to be recalculated. Otherwise, the layout
 *				of only those markers and elements that
 *				have changed will be reset. 
 *
 *	GET_AXIS_GEOMETRY	Indicates that the size of the axes needs 
 *				to be recalculated. 
 *
 *	RESET_AXES		Flag to call to Blt_ResetAxes routine.  
 *				This routine recalculates the scale offset
 *				(used for mapping coordinates) of each axis.
 *				If an axis limit has changed, then it sets 
 *				flags to re-layout and redraw the entire 
 *				graph.  This needs to happend before the axis
 *				can compute transformations between graph and 
 *				screen coordinates. 
 *
 *	LAYOUT_NEEDED		
 *
 *	CACHE_DIRTY		If set, redraw all elements into the pixmap 
 *				used for buffering elements. 
 *
 *	REDRAW_PENDING		Non-zero means a DoWhenIdle handler has 
 *				already been queued to redraw this window. 
 *
 *	DRAW_LEGEND		Non-zero means redraw the legend. If this is 
 *				the only DRAW_* flag, the legend display 
 *				routine is called instead of the graph 
 *				display routine. 
 *
 *	DRAW_MARGINS		Indicates that the margins bordering 
 *				the plotting area need to be redrawn. 
 *				The possible reasons are:
 *
 *				1) an axis configuration changed
 *				2) an axis limit changed
 *				3) titles have changed
 *				4) window was resized. 
 *
 *	GRAPH_FOCUS	
 */

#define DELETE_PENDING		(1<<1) /* 0x0002 */
#define REDRAW_PENDING		(1<<2) /* 0x0004 */
#define	ACTIVE_PENDING		(1<<3) /* 0x0008 */
#define	MAP_ITEM		(1<<4) /* 0x0010 */
#define DIRTY			(1<<5) /* 0x0020 */
#define ACTIVE			(1<<6) /* 0x0040 */
#define FOCUS			(1<<7) /* 0x0080 */

#define	MAP_ALL			(1<<8) /* 0x0100 */
#define LAYOUT_NEEDED		(1<<9) /* 0x0200 */
#define RESET_AXES		(1<<10)/* 0x0400 */
#define	GET_AXIS_GEOMETRY	(1<<11)/* 0x0800 */

#define DRAW_LEGEND		(1<<12)/* 0x1000 */
#define DRAW_MARGINS		(1<<13)/* 0x2000 */
#define	CACHE_DIRTY		(1<<14)/* 0x4000 */

#define	GRAPH_DELETED		(1<<15)/* 0x4000 */

#define	MAP_WORLD		(MAP_ALL|RESET_AXES|GET_AXIS_GEOMETRY)
#define REDRAW_WORLD		(DRAW_LEGEND)
#define RESET_WORLD		(REDRAW_WORLD | MAP_WORLD)

extern void Blt_MapGraph(Graph* graphPtr);
extern void Blt_ReconfigureGraph(Graph* graphPtr);
extern void Blt_GraphExtents(Graph* graphPtr, Region2d *extsPtr);
extern void Blt_EventuallyRedrawGraph(Graph* graphPtr);

typedef ClientData (MakeTagProc)(Graph* graphPtr, const char *tagName);
extern MakeTagProc Blt_MakeElementTag;
extern MakeTagProc Blt_MakeAxisTag;
extern Blt_BindTagProc Blt_GraphTags;
extern Blt_BindTagProc Blt_AxisTags;

extern Graph *Blt_GetGraphFromWindowData(Tk_Window tkwin);

#endif
