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

#include <stdarg.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bltC.h"

extern "C" {
#include "bltInt.h"
#include "bltGraph.h"
};

// Converts a string in the form "@x,y" into an XPoint structure of the x
// and y coordinates.
int Blt_GetXY(Tcl_Interp* interp, Tk_Window tkwin, const char* string, 
	      int* xPtr, int* yPtr)
{
  char *comma;
  int result;
  int x, y;

  if ((string == NULL) || (*string == '\0')) {
    *xPtr = *yPtr = -SHRT_MAX;
    return TCL_OK;
  }
  if (*string != '@') {
    goto badFormat;
  }
  comma = (char*)strchr(string + 1, ',');
  if (comma == NULL) {
    goto badFormat;
  }
  *comma = '\0';
  result = ((Tk_GetPixels(interp, tkwin, string + 1, &x) == TCL_OK) &&
	    (Tk_GetPixels(interp, tkwin, comma + 1, &y) == TCL_OK));
  *comma = ',';
  if (!result) {
    Tcl_AppendResult(interp, ": can't parse position \"", string, "\"",
		     (char *)NULL);
    return TCL_ERROR;
  }
  *xPtr = x, *yPtr = y;
  return TCL_OK;

 badFormat:
  Tcl_AppendResult(interp, "bad position \"", string, 
		   "\": should be \"@x,y\"", (char *)NULL);
  return TCL_ERROR;
}

int Blt_PointInPolygon(Point2d *s, Point2d *points, int nPoints)
{
  Point2d *p, *q, *qend;
  int count;

  count = 0;
  for (p = points, q = p + 1, qend = p + nPoints; q < qend; p++, q++) {
    if (((p->y <= s->y) && (s->y < q->y)) || 
	((q->y <= s->y) && (s->y < p->y))) {
      double b;

      b = (q->x - p->x) * (s->y - p->y) / (q->y - p->y) + p->x;
      if (s->x < b) {
	count++;	/* Count the number of intersections. */
      }
    }
  }
  return (count & 0x01);
}

/*
 *---------------------------------------------------------------------------
 * Generates a bounding box representing the plotting area of the
 * graph. This data structure is used to clip the points and line
 * segments of the line element.
 * The clip region is the plotting area plus such arbitrary extra space.
 * The reason we clip with a bounding box larger than the plot area is so
 * that symbols will be drawn even if their center point isn't in the
 * plotting area.
 *---------------------------------------------------------------------------
 */

void Blt_GraphExtents(Graph* graphPtr, Region2d *regionPtr)
{
  regionPtr->left = (double)(graphPtr->hOffset - graphPtr->xPad);
  regionPtr->top = (double)(graphPtr->vOffset - graphPtr->yPad);
  regionPtr->right = (double)(graphPtr->hOffset + graphPtr->hRange + 
			      graphPtr->xPad);
  regionPtr->bottom = (double)(graphPtr->vOffset + graphPtr->vRange + 
			       graphPtr->yPad);
}

static int ClipTest (double ds, double dr, double *t1, double *t2)
{
  double t;

  if (ds < 0.0) {
    t = dr / ds;
    if (t > *t2) {
      return 0;
    } 
    if (t > *t1) {
      *t1 = t;
    }
  } else if (ds > 0.0) {
    t = dr / ds;
    if (t < *t1) {
      return 0;
    } 
    if (t < *t2) {
      *t2 = t;
    }
  } else {
    /* d = 0, so line is parallel to this clipping edge */
    if (dr < 0.0) {		/* Line is outside clipping edge */
      return 0;
    }
  }
  return 1;
}

/*
 *---------------------------------------------------------------------------
 *	Clips the given line segment to a rectangular region.  The coordinates
 *	of the clipped line segment are returned.  The original coordinates
 *	are overwritten.
 *
 *	Reference: 
 *	  Liang, Y-D., and B. Barsky, A new concept and method for
 *	  Line Clipping, ACM, TOG,3(1), 1984, pp.1-22.
 *---------------------------------------------------------------------------
 */
int Blt_LineRectClip(Region2d* regionPtr, Point2d *p, Point2d *q)
{
  double t1, t2;
  double dx, dy;

  t1 = 0.0, t2 = 1.0;
  dx = q->x - p->x;
  if ((ClipTest (-dx, p->x - regionPtr->left, &t1, &t2)) &&
      (ClipTest (dx, regionPtr->right - p->x, &t1, &t2))) {
    dy = q->y - p->y;
    if ((ClipTest (-dy, p->y - regionPtr->top, &t1, &t2)) && 
	(ClipTest (dy, regionPtr->bottom - p->y, &t1, &t2))) {
      if (t2 < 1.0) {
	q->x = p->x + t2 * dx;
	q->y = p->y + t2 * dy;
      }
      if (t1 > 0.0) {
	p->x += t1 * dx;
	p->y += t1 * dy;
      }
      return 1;
    }
  }
  return 0;
}

/*
 *---------------------------------------------------------------------------
 *	Clips the given polygon to a rectangular region.  The resulting
 *	polygon is returned. Note that the resulting polyon may be complex,
 *	connected by zero width/height segments.  The drawing routine (such as
 *	XFillPolygon) will not draw a connecting segment.
 *
 *	Reference:  
 *	  Liang Y. D. and Brian A. Barsky, "Analysis and Algorithm for
 *	  Polygon Clipping", Communications of ACM, Vol. 26,
 *	  p.868-877, 1983
 *---------------------------------------------------------------------------
 */
#define EPSILON  FLT_EPSILON
#define AddVertex(vx, vy)	    r->x=(vx), r->y=(vy), r++, count++ 
#define LastVertex(vx, vy)	    r->x=(vx), r->y=(vy), count++ 

int Blt_PolyRectClip(Region2d *regionPtr, Point2d *points, int nPoints,
		     Point2d *clipPts)
{
  Point2d *p;			/* First vertex of input polygon edge. */
  Point2d *pend;
  Point2d *q;			/* Last vertex of input polygon edge. */
  Point2d *r;
  int count;

  r = clipPts;
  count = 0;			/* Counts # of vertices in output polygon. */

  points[nPoints] = points[0];
  for (p = points, q = p + 1, pend = p + nPoints; p < pend; p++, q++) {
    double dx, dy;
    double tin1, tin2, tinx, tiny;
    double xin, yin, xout, yout;

    dx = q->x - p->x;	/* X-direction */
    dy = q->y - p->y;	/* Y-direction */

    if (fabs(dx) < EPSILON) { 
      dx = (p->x > regionPtr->left) ? -EPSILON : EPSILON ;
    }
    if (fabs(dy) < EPSILON) { 
      dy = (p->y > regionPtr->top) ? -EPSILON : EPSILON ;
    }

    if (dx > 0.0) {		/* Left */
      xin = regionPtr->left;
      xout = regionPtr->right + 1.0;
    } else {		/* Right */
      xin = regionPtr->right + 1.0;
      xout = regionPtr->left;
    }
    if (dy > 0.0) {		/* Top */
      yin = regionPtr->top;
      yout = regionPtr->bottom + 1.0;
    } else {		/* Bottom */
      yin = regionPtr->bottom + 1.0;
      yout = regionPtr->top;
    }
	
    tinx = (xin - p->x) / dx;
    tiny = (yin - p->y) / dy;
	
    if (tinx < tiny) {	/* Hits x first */
      tin1 = tinx;
      tin2 = tiny;
    } else {		/* Hits y first */
      tin1 = tiny;
      tin2 = tinx;
    }
	
    if (tin1 <= 1.0) {
      if (tin1 > 0.0) {
	AddVertex(xin, yin);
      }
      if (tin2 <= 1.0) {
	double toutx, touty, tout1;

	toutx = (xout - p->x) / dx;
	touty = (yout - p->y) / dy;
	tout1 = MIN(toutx, touty);
		
	if ((tin2 > 0.0) || (tout1 > 0.0)) {
	  if (tin2 <= tout1) {
	    if (tin2 > 0.0) {
	      if (tinx > tiny) {
		AddVertex(xin, p->y + tinx * dy);
	      } else {
		AddVertex(p->x + tiny * dx, yin);
	      }
	    }
	    if (tout1 < 1.0) {
	      if (toutx < touty) {
		AddVertex(xout, p->y + toutx * dy);
	      } else {
		AddVertex(p->x + touty * dx, yout);
	      }
	    } else {
	      AddVertex(q->x, q->y);
	    }
	  } else {
	    if (tinx > tiny) {
	      AddVertex(xin, yout);
	    } else {
	      AddVertex(xout, yin);
	    }

	  }
	}
      }
    }
  }
  if (count > 0) {
    LastVertex(clipPts[0].x, clipPts[0].y);
  }
  return count;
}

/*
 *---------------------------------------------------------------------------
 *	Computes the projection of a point on a line.  The line (given by two
 *	points), is assumed the be infinite.
 *
 *	Compute the slope (angle) of the line and rotate it 90 degrees.  Using
 *	the slope-intercept method (we know the second line from the sample
 *	test point and the computed slope), then find the intersection of both
 *	lines. This will be the projection of the sample point on the first
 *	line.
 *---------------------------------------------------------------------------
 */
Point2d Blt_GetProjection(int x, int y, Point2d *p, Point2d *q)
{
  double dx, dy;
  Point2d t;

  dx = p->x - q->x;
  dy = p->y - q->y;

  /* Test for horizontal and vertical lines */
  if (fabs(dx) < DBL_EPSILON) {
    t.x = p->x, t.y = (double)y;
  } else if (fabs(dy) < DBL_EPSILON) {
    t.x = (double)x, t.y = p->y;
  } else {
    double m1, m2;		/* Slope of both lines */
    double b1, b2;		/* y-intercepts */
    double midX, midY;	/* Midpoint of line segment. */
    double ax, ay, bx, by;

    /* Compute the slope and intercept of PQ. */
    m1 = (dy / dx);
    b1 = p->y - (p->x * m1);

    /* 
     * Compute the slope and intercept of a second line segment: one that
     * intersects through sample X-Y coordinate with a slope perpendicular
     * to original line.
     */

    /* Find midpoint of PQ. */
    midX = (p->x + q->x) * 0.5;
    midY = (p->y + q->y) * 0.5;

    /* Rotate the line 90 degrees */
    ax = midX - (0.5 * dy);
    ay = midY - (0.5 * -dx);
    bx = midX + (0.5 * dy);
    by = midY + (0.5 * -dx);

    m2 = (ay - by) / (ax - bx);
    b2 = y - (x * m2);

    /*
     * Given the equations of two lines which contain the same point,
     *
     *    y = m1 * x + b1
     *    y = m2 * x + b2
     *
     * solve for the intersection.
     *
     *    x = (b2 - b1) / (m1 - m2)
     *    y = m1 * x + b1
     *
     */

    t.x = (b2 - b1) / (m1 - m2);
    t.y = m1 * t.x + b1;
  }
  return t;
}

/*
 *---------------------------------------------------------------------------
 * 	Invoke a TCL command to the scrollbar, defining the new position and
 * 	length of the scroll. See the Tk documentation for further information
 * 	on the scrollbar.  It is assumed the scrollbar command prefix is
 * 	valid.
 *---------------------------------------------------------------------------
 */
void Blt_UpdateScrollbar(Tcl_Interp* interp, Tcl_Obj *scrollCmdObjPtr,
			 int first, int last, int width)
{
  Tcl_Obj *cmdObjPtr;
  double firstFract, lastFract;

  firstFract = 0.0, lastFract = 1.0;
  if (width > 0) {
    firstFract = (double)first / (double)width;
    lastFract = (double)last / (double)width;
  }
  cmdObjPtr = Tcl_DuplicateObj(scrollCmdObjPtr);
  Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewDoubleObj(firstFract));
  Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewDoubleObj(lastFract));
  Tcl_IncrRefCount(cmdObjPtr);
  if (Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL) != TCL_OK) {
    Tcl_BackgroundError(interp);
  }
  Tcl_DecrRefCount(cmdObjPtr);
}

/*
 *---------------------------------------------------------------------------
 *      Like Tk_GetGC, but doesn't share the GC with any other widget.  This
 *      is needed because the certain GC parameters (like dashes) can not be
 *      set via XCreateGC, therefore there is no way for Tk's hashing
 *      mechanism to recognize that two such GCs differ.
 *---------------------------------------------------------------------------
 */
GC Blt_GetPrivateGC(Tk_Window tkwin, unsigned long gcMask, XGCValues *valuePtr)
{
  GC gc;
  Pixmap pixmap;
  Drawable drawable;
  Display *display;

  pixmap = None;
  drawable = Tk_WindowId(tkwin);
  display = Tk_Display(tkwin);
  if (drawable == None)
    drawable = Tk_RootWindow(tkwin);

  gc = XCreateGC(display, drawable, gcMask, valuePtr);
  if (pixmap != None) {
    Tk_FreePixmap(display, pixmap);
  }
  return gc;
}

void Blt_FreePrivateGC(Display *display, GC gc)
{
  Tk_FreeXId(display, (XID) XGContextFromGC(gc));
  XFreeGC(display, gc);
}

void Blt_SetDashes(Display *display, GC gc, Blt_Dashes *dashesPtr)
{
  XSetDashes(display, gc, dashesPtr->offset, (const char *)dashesPtr->values,
	     (int)strlen((char *)dashesPtr->values));
}

void Blt_Draw2DSegments(Display *display, Drawable drawable, GC gc,
			Segment2d *segments, int nSegments)
{
  Segment2d *sp, *send;

  XSegment* xsegments = (XSegment*)malloc(nSegments * sizeof(XSegment));
  if (xsegments == NULL)
    return;

  XSegment* dp = xsegments;
  for (sp = segments, send = sp + nSegments; sp < send; sp++) {
    dp->x1 = (short int)sp->p.x;
    dp->y1 = (short int)sp->p.y;
    dp->x2 = (short int)sp->q.x;
    dp->y2 = (short int)sp->q.y;
    dp++;
  }
  XDrawSegments(display, drawable, gc, xsegments, nSegments);
  free(xsegments);
}

long Blt_MaxRequestSize(Display *display, size_t elemSize) 
{
  static long maxSizeBytes = 0L;

  if (maxSizeBytes == 0L) {
    long size;
    size = XExtendedMaxRequestSize(display);
    if (size == 0) {
      size = XMaxRequestSize(display);
    }
    size -= (4 * elemSize);
    /*	maxSizeBytes = (size * 4); */
    maxSizeBytes = size;
  }
  return (maxSizeBytes / elemSize);
}

