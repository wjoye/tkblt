/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGraph.h --
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

#ifndef _BLT_GRAPH_H
#define _BLT_GRAPH_H

#include "bltInt.h"
#include "bltBind.h"
#include "bltChain.h"
#include "bltPs.h"

typedef struct _Graph Graph;
typedef struct _Element Element;
typedef struct _Legend Legend;

typedef enum {
  CID_NONE, CID_AXIS_X, CID_AXIS_Y, CID_ELEM_BAR, CID_ELEM_LINE,
  CID_MARKER_BITMAP, CID_MARKER_IMAGE, CID_MARKER_LINE, CID_MARKER_POLYGON,
  CID_MARKER_TEXT, CID_MARKER_WINDOW, CID_LEGEND_ENTRY,
} ClassId;

typedef struct {
  /* Generic fields common to all graph objects. */
  ClassId classId;		/* Class type of object. */
  const char *name;		/* Identifier to refer the object. */
  const char *className;	/* Class name of object. */

  Graph* graphPtr;		/* Graph containing of the object. */

  const char **tags;		/* Binding tags for the object. */
} GraphObj;

#include "bltGrAxis.h"
#include "bltGrLegd.h"

#define MARKER_UNDER	1	/* Draw markers designated to lie underneath
				 * elements, grids, legend, etc. */
#define MARKER_ABOVE	0	/* Draw markers designated to rest above
				 * elements, grids, legend, etc. */

#define PADX		2	/* Padding between labels/titles */
#define PADY    	2	/* Padding between labels */

#define MINIMUM_MARGIN	20	/* Minimum margin size */


#define BOUND(x, lo, hi) (((x) > (hi)) ? (hi) : ((x) < (lo)) ? (lo) : (x))

/*
 *---------------------------------------------------------------------------
 *
 * 	Graph component structure definitions
 *
 *---------------------------------------------------------------------------
 */
#define PointInGraph(g,x,y) (((x) <= (g)->right) && ((x) >= (g)->left) && ((y) <= (g)->bottom) && ((y) >= (g)->top))

/*
 * Mask values used to selectively enable GRAPH or BARCHART entries in the
 * various configuration specs.
 */
#define GRAPH			(BLT_CONFIG_USER_BIT << 1)
#define STRIPCHART		(BLT_CONFIG_USER_BIT << 2)
#define BARCHART		(BLT_CONFIG_USER_BIT << 3)
#define LINE_GRAPHS		(GRAPH | STRIPCHART)
#define ALL_GRAPHS		(GRAPH | BARCHART | STRIPCHART)

#define ACTIVE_PEN		(BLT_CONFIG_USER_BIT << 16)
#define NORMAL_PEN		(BLT_CONFIG_USER_BIT << 17)
#define ALL_PENS		(NORMAL_PEN | ACTIVE_PEN)

typedef struct {
  Segment2d *segments;
  int length;
  int *map;
} GraphSegments;

typedef struct {
  Point2d *points;
  int length;
  int *map;
} GraphPoints;

/*
 *---------------------------------------------------------------------------
 *
 * BarGroup --
 *
 *	Represents a sets of bars with the same abscissa. This structure is
 *	used to track the display of the bars in the group.  
 *
 *	Each unique abscissa has at least one group.  Groups can be
 *	defined by the bar element's -group option.  Multiple groups are
 *	needed when you are displaying/comparing similar sets of data (same
 *	abscissas) but belong to a separate group.
 *	
 *---------------------------------------------------------------------------
 */
typedef struct {
  int nSegments;			/* Number of occurrences of
					 * x-coordinate */
  Axis2d axes;			/* The axes associated with this
				 * group. (mapped to the x-value) */
  float sum;				/* Sum of the ordinates (y-coorinate) of
					 * each duplicate abscissa. Used to
					 * determine height of stacked bars. */
  int count;				/* Current number of bars seen.  Used to
					 * position of the next bar in the
					 * group. */
  float lastY;			/* y-cooridinate position of the
				 * last bar seen. */
  size_t index;			/* Order of group in set (an unique
				 * abscissa may have more than one
				 * group). */
} BarGroup;

/*
 *---------------------------------------------------------------------------
 *
 * BarSetKey --
 *
 *	Key for hash table of set of bars.  The bar set is defined by
 *	coordinates with the same abscissa (x-coordinate).
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
  float value;			/* Duplicated abscissa */
  Axis2d axes;			/* Axis mapping of element */
} BarSetKey;

/*
 * BarModes --
 *
 *	Bar elements are displayed according to their x-y coordinates.  If two
 *	bars have the same abscissa (x-coordinate), the bar segments will be
 *	drawn according to one of the following modes:
 */

typedef enum BarModes {
  BARS_INFRONT, BARS_STACKED, BARS_ALIGNED, BARS_OVERLAP
} BarMode;

typedef struct _Pen Pen;
typedef struct _Marker Marker;

typedef Pen *(PenCreateProc)(void);
typedef int (PenConfigureProc)(Graph* graphPtr, Pen* penPtr);
typedef void (PenDestroyProc)(Graph* graphPtr, Pen* penPtr);

/*
 *---------------------------------------------------------------------------
 *
 * Crosshairs
 *
 *	Contains the line segments positions and graphics context used to
 *	simulate crosshairs (by XOR-ing) on the graph.
 *
 *---------------------------------------------------------------------------
 */
typedef struct _Crosshairs Crosshairs;

typedef struct {
  short int width, height;		/* Dimensions of the margin */
  short int axesOffset;
  short int axesTitleLength;		/* Width of the widest title to be
					 * shown.  Multiple titles are displayed
					 * in another margin. This is the
					 * minimum space requirement. */
  short int maxTickWidth;
  short int maxTickHeight;
  unsigned int nAxes;			/* # of axes to be displayed */
  Blt_Chain axes;			/* Axes associated with this margin */
  const char *varName;		/* If non-NULL, name of variable to be
				 * updated when the margin size
				 * changes */
  int reqSize;			/* Requested size of margin */
  int site;				/* Indicates where margin is located:
					 * left, right, top, or bottom. */
} Margin;

#define MARGIN_NONE	-1
#define MARGIN_BOTTOM	0		/* x */
#define MARGIN_LEFT	1 		/* y */
#define MARGIN_TOP	2		/* x2 */
#define MARGIN_RIGHT	3		/* y2 */

#define rightMargin	margins[MARGIN_RIGHT]
#define leftMargin	margins[MARGIN_LEFT]
#define topMargin	margins[MARGIN_TOP]
#define bottomMargin	margins[MARGIN_BOTTOM]

/*
 *---------------------------------------------------------------------------
 *
 * Graph --
 *
 *	Top level structure containing everything pertaining to the graph.
 *
 *---------------------------------------------------------------------------
 */
struct _Graph {
  Tcl_Interp* interp;			/* Interpreter associated with graph */
  Tk_Window tkwin;			/* Window that embodies the graph.
					 * NULL means that the window has been
					 * destroyed but the data structures
					 * haven't yet been cleaned up. */
  Display *display;			/* Display containing widget; used to
					 * release resources after tkwin has
					 * already gone away. */
  Tcl_Command cmdToken;		/* Token for graph's widget command. */
  Tk_OptionTable optionTable;

  ClassId classId;			/* Default element type */
  Tk_Cursor cursor;
  int inset;				/* Sum of focus highlight and 3-D
					 * border.  Indicates how far to
					 * offset the graph from outside edge
					 * of the window. */
  int borderWidth;			/* Width of the exterior border */
  int relief;				/* Relief of the exterior border. */
  unsigned int flags;			/* Flags;  see below for definitions. */
  Tk_3DBorder normalBg;		/* 3-D border used to delineate the
					 * plot surface and outer edge of
					 * window. */
  int highlightWidth;			/* Width in pixels of highlight to
					 * draw around widget when it has the
					 * focus.  <= 0 means don't draw a
					 * highlight. */
  XColor* highlightBgColor;		/* Color for drawing traversal
					 * highlight area when highlight is
					 * off. */
  XColor* highlightColor;		/* Color for drawing traversal
					 * highlight. */
  const char *title;			/* Graph title */
  short int titleX, titleY;		/* Position of title on graph. */
  short int titleWidth, titleHeight;	/* Dimensions of title. */
  TextStyle titleTextStyle;		/* Title attributes: font, color,
					 * etc.*/
    

  const char *takeFocus;		/* Not used in C code */
  Axis *focusPtr;			/* The axis that currently has focus. */
    
  int reqWidth, reqHeight;		/* Requested size of graph window */
  int reqPlotWidth, reqPlotHeight;	/* Requested size of plot area. Zero
					 * means to adjust the dimension
					 * according to the available space
					 * left in the window. */
  int width, height;			/* Actual size (in pixels) of graph
					 * window or PostScript page. */
  Tcl_HashTable penTable;		/* Table of pens */
  struct Component {
    Tcl_HashTable table;		/* Hash table of ids. */
    Blt_Chain displayList;		/* Display list. */
    Tcl_HashTable tagTable;		/* Table of bind tags. */
  } elements, markers, axes;

  Tcl_HashTable dataTables;		/* Hash table of datatable clients. */
  Blt_BindTable bindTable;
  int nextMarkerId;			/* Tracks next marker identifier
					 * available */
  Blt_Chain axisChain[4];		/* Chain of axes for each of the
					 * margins.  They're separate from the
					 * margin structures to make it easier
					 * to invert the X-Y axes by simply
					 * switching chain pointers. */
  Margin margins[4];
  PageSetup *pageSetup;		/* Page layout options: see bltGrPS.c */
  Legend *legend;			/* Legend information: see
					 * bltGrLegd.c */
  Crosshairs *crosshairs;		/* Crosshairs information: see
					 * bltGrHairs.c */
  int halo;				/* Maximum distance allowed between
					 * points when searching for a point */
  int inverted;			/* If non-zero, indicates the x and y
				 * axis positions should be inverted. */
  int stackAxes;			/* If non-zero, indicates to stack
					 * mulitple axes in a margin, rather
					 * than layering them one on top of
					 * another. */
  GC drawGC;				/* GC for drawing on the margins. This
					 * includes the axis lines */  
  int plotBW;				/* Width of interior 3-D border. */
  int plotRelief;			/* 3-d effect: TK_RELIEF_RAISED etc. */
  Tk_3DBorder plotBg;		/* Color of plotting surface */

  /* If non-zero, force plot to conform to aspect ratio W/H */
  double aspect;

  short int left, right;		/* Coordinates of plot bbox */
  short int top, bottom;	

  int xPad;			/* Vertical padding for plotarea */
  int vRange, vOffset;		/* Vertical axis range and offset from
				 * the left side of the graph
				 * window. Used to transform coordinates
				 * to vertical axes. */
  int yPad;			/* Horizontal padding for plotarea */
  int hRange, hOffset;		/* Horizontal axis range and offset from
				 * the top of the graph window. Used to
				 * transform horizontal axes */
  float vScale, hScale;

  int doubleBuffer;			/* If non-zero, draw the graph into a
					 * pixmap first to reduce flashing. */
  int backingStore;			/* If non-zero, cache elements by
					 * drawing them into a pixmap */
  Pixmap cache;			/* Pixmap used to cache elements
				 * displayed.  If *backingStore* is
				 * non-zero, each element is drawn into
				 * this pixmap before it is copied onto
				 * the screen.  The pixmap then acts as
				 * a cache (only the pixmap is
				 * redisplayed if the none of elements
				 * have changed). This is done so that
				 * markers can be redrawn quickly over
				 * elements without redrawing each
				 * element. */
  short int cacheWidth, cacheHeight;	/* Size of element backing store
					 * pixmap. */

  /*
   * barchart specific information
   */
  double baseline;			/* Baseline from bar chart.  */
  double barWidth;			/* Default width of each bar in graph
					 * units.  The default width is 1.0
					 * units. */
  BarMode barMode;			/* Mode describing how to display bars
				 * with the same x-coordinates. Mode can
				 * be "stacked", "aligned", "overlap",
				 * or "infront" */
  BarGroup *barGroups;		/* Contains information about duplicate
				 * x-values in bar elements (malloc-ed).
				 * This information can also be accessed
				 * by the group hash table */
  int nBarGroups;			/* # of entries in barGroups array.  If 
					 * zero, indicates nothing special
					 * needs to be * done for "stack" or
					 * "align" modes */
  Tcl_HashTable setTable;		/* Table managing sets of bars with
					 * the same abscissas. The bars in a
					 * set may be displayed is various
					 * ways: aligned, overlap, infront, or
					 * stacked. */
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

#define HIDE			(1<<0) /* 0x0001 */
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

/*
 * ---------------------- Forward declarations ------------------------
 */

extern int Blt_CreatePageSetup(Graph* graphPtr);
extern int Blt_ConfigurePageSetup(Graph* graphPtr);

extern int Blt_CreateCrosshairs(Graph* graphPtr);
extern void Blt_DestroyCrosshairs(Graph* graphPtr);

extern double Blt_InvHMap(Axis *axisPtr, double x);

extern double Blt_InvVMap(Axis *axisPtr, double x);

extern double Blt_HMap(Axis *axisPtr, double x);

extern double Blt_VMap(Axis *axisPtr, double y);

extern Point2d Blt_InvMap2D(Graph* graphPtr, double x, double y, 
			    Axis2d *pairPtr);

extern Point2d Blt_Map2D(Graph* graphPtr, double x, double y, 
			 Axis2d *pairPtr);

extern Graph *Blt_GetGraphFromWindowData(Tk_Window tkwin);

extern void Blt_AdjustAxisPointers(Graph* graphPtr);

extern int Blt_PolyRectClip(Region2d *extsPtr, Point2d *inputPts,
			    int nInputPts, Point2d *outputPts);

extern void Blt_ComputeBarStacks(Graph* graphPtr);

extern void Blt_ReconfigureGraph(Graph* graphPtr);

extern void Blt_DestroyAxes(Graph* graphPtr);


extern void Blt_DestroyElements(Graph* graphPtr);

extern void Blt_DestroyMarkers(Graph* graphPtr);

extern void Blt_DestroyPageSetup(Graph* graphPtr);

extern void Blt_DrawAxes(Graph* graphPtr, Drawable drawable);

extern void Blt_DrawAxisLimits(Graph* graphPtr, Drawable drawable);

extern void Blt_DrawElements(Graph* graphPtr, Drawable drawable);

extern void Blt_DrawActiveElements(Graph* graphPtr, Drawable drawable);

extern void Blt_DrawGraph(Graph* graphPtr, Drawable drawable);

extern void Blt_DrawMarkers(Graph* graphPtr, Drawable drawable, int under);

extern void Blt_Draw2DSegments(Display *display, Drawable drawable, GC gc, 
			       Segment2d *segments, int nSegments);

extern int Blt_GetCoordinate(Tcl_Interp* interp, const char *string, 
			     double *valuePtr);

extern void Blt_InitBarSetTable(Graph* graphPtr);

extern void Blt_LayoutGraph(Graph* graphPtr);

extern void Blt_EventuallyRedrawGraph(Graph* graphPtr);

extern void Blt_ResetAxes(Graph* graphPtr);

extern void Blt_ResetBarGroups(Graph* graphPtr);

extern void Blt_GraphExtents(Graph* graphPtr, Region2d *extsPtr);

extern void Blt_DisableCrosshairs(Graph* graphPtr);

extern void Blt_EnableCrosshairs(Graph* graphPtr);

extern void Blt_MapGraph(Graph* graphPtr);

extern void Blt_MapAxes(Graph* graphPtr);

extern void Blt_MapElements(Graph* graphPtr);

extern void Blt_MapMarkers(Graph* graphPtr);

extern void Blt_DestroyPens(Graph* graphPtr);

extern int Blt_GetPenFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			     Tcl_Obj *objPtr, ClassId classId, Pen **penPtrPtr);

extern Pen* Blt_BarPen(Graph* graphPtr, const char* penName);

extern Pen* Blt_LinePen(Graph* graphPtr, const char* penName);

extern int Blt_CreatePen(Graph* graphPtr, Tcl_Interp* interp, 
			 const char* penName, ClassId classId, 
			 int objc, Tcl_Obj* const objv[]);

extern int Blt_InitLinePens(Graph* graphPtr);

extern int Blt_InitBarPens(Graph* graphPtr);

extern void Blt_FreePen(Pen* penPtr);

extern int Blt_AxisOp(Graph* graphPtr, Tcl_Interp* interp, 
		      int objc, Tcl_Obj* const objv[]);

extern int Blt_DefAxisOp(Tcl_Interp* interp, Graph* graphPtr, int margin, 
			 int objc, Tcl_Obj* const objv[]);

extern int Blt_ElementOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			 Tcl_Obj* const objv[], ClassId classId);

extern int Blt_CrosshairsOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
				Tcl_Obj* const objv[]);

extern int Blt_MarkerOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			Tcl_Obj* const objv[]);

extern int Blt_PenOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
		     Tcl_Obj* const objv[]);

extern int Blt_PointInPolygon(Point2d *samplePtr, Point2d *screenPts, 
			      int nScreenPts);

extern int Blt_RegionInPolygon(Region2d *extsPtr, Point2d *points, 
			       int nPoints, int enclosed);

extern int Blt_PointInSegments(Point2d *samplePtr, Segment2d *segments, 
			       int nSegments, double halo);

extern int Blt_PostScriptOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			    Tcl_Obj* const objv[]);

extern int Blt_GraphUpdateNeeded(Graph* graphPtr);

extern int Blt_CreateAxes(Graph* graphPtr);

extern Axis *Blt_GetFirstAxis(Blt_Chain chain);

extern void Blt_UpdateAxisBackgrounds(Graph* graphPtr);

extern Marker *Blt_NearestMarker(Graph* graphPtr, int x, int y, int under);

extern Axis *Blt_NearestAxis(Graph* graphPtr, int x, int y);

typedef ClientData (MakeTagProc)(Graph* graphPtr, const char *tagName);
extern MakeTagProc Blt_MakeElementTag;
extern MakeTagProc Blt_MakeMarkerTag;
extern MakeTagProc Blt_MakeAxisTag;
extern Blt_BindTagProc Blt_GraphTags;
extern Blt_BindTagProc Blt_AxisTags;

extern int Blt_GraphType(Graph* graphPtr);

extern void Blt_GraphSetObjectClass(GraphObj *graphObjPtr,ClassId classId);

extern void Blt_MarkersToPostScript(Graph* graphPtr, Blt_Ps ps, int under);
extern void Blt_ElementsToPostScript(Graph* graphPtr, Blt_Ps ps);
extern void Blt_ActiveElementsToPostScript(Graph* graphPtr, Blt_Ps ps);
extern void Blt_LegendToPostScript(Graph* graphPtr, Blt_Ps ps);
extern void Blt_AxesToPostScript(Graph* graphPtr, Blt_Ps ps);
extern void Blt_AxisLimitsToPostScript(Graph* graphPtr, Blt_Ps ps);

extern Element *Blt_LineElement(Graph* graphPtr, const char *name, 
				ClassId classId);
extern Element *Blt_BarElement(Graph* graphPtr, const char *name, 
			       ClassId classId);

extern void Blt_DrawGrids(Graph* graphPtr, Drawable drawable);

extern void Blt_GridsToPostScript(Graph* graphPtr, Blt_Ps ps);
extern void Blt_InitBarSetTable(Graph* graphPtr);
extern void Blt_DestroyBarSets(Graph* graphPtr);

/* ---------------------- Global declarations ------------------------ */

extern const char *Blt_GraphClassName(ClassId classId);


#endif
