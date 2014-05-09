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

#ifndef __BltGrMisc_h__
#define __BltGrMisc_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include <tk.h>

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

#define MARGIN_NONE	-1
#define MARGIN_BOTTOM	0		/* x */
#define MARGIN_LEFT	1 		/* y */
#define MARGIN_TOP	2		/* x2 */
#define MARGIN_RIGHT	3		/* y2 */

class Graph;

typedef struct {
  double x;
  double y;
} Point2d;

typedef struct {
  double left;
  double right;
  double top;
  double bottom;
} Region2d;

typedef struct {
  Point2d p;
  Point2d q;
} Segment2d;

typedef enum {
  CID_NONE, CID_AXIS_X, CID_AXIS_Y, CID_ELEM_BAR, CID_ELEM_LINE,
  CID_MARKER_BITMAP, CID_MARKER_IMAGE, CID_MARKER_LINE, CID_MARKER_POLYGON,
  CID_MARKER_TEXT, CID_LEGEND_ENTRY,
} ClassId;

typedef struct {
  unsigned char values[12];
  int offset;
} Blt_Dashes;

#define LineIsDashed(d) ((d).values[0] != 0)

namespace Blt {
extern char* dupstr(const char*);
};

extern void Blt_SetDashes (Display *display, GC gc, Blt_Dashes *dashesPtr);

extern int Blt_PointInPolygon(Point2d *samplePtr, Point2d *screenPts, 
			      int nScreenPts);
extern int Blt_GetXY(Tcl_Interp* interp, Tk_Window tkwin, 
		     const char *string, int *xPtr, int *yPtr);
extern int Blt_PolyRectClip(Region2d *extsPtr, Point2d *inputPts,
			    int nInputPts, Point2d *outputPts);
extern void Blt_Draw2DSegments(Display *display, Drawable drawable, GC gc, 
			       Segment2d *segments, int nSegments);
extern int Blt_LineRectClip(Region2d *regionPtr, Point2d *p, Point2d *q);
extern GC Blt_GetPrivateGC(Tk_Window tkwin, unsigned long gcMask,
	XGCValues *valuePtr);
extern void Blt_FreePrivateGC(Display *display, GC gc);
extern Point2d Blt_GetProjection (int x, int y, Point2d *p, Point2d *q);
extern long Blt_MaxRequestSize (Display *display, size_t elemSize);
extern Graph *Blt_GetGraphFromWindowData(Tk_Window tkwin);


#endif
