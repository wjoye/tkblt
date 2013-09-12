

/*
 * bltGrMisc.c --
 *
 * This module implements miscellaneous routines for the BLT graph widget.
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

#include <assert.h>
#include <stdarg.h>
#include <X11/Xutil.h>

#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"

#define ARROW_LEFT		(0)
#define ARROW_UP		(1)
#define ARROW_RIGHT		(2)
#define ARROW_DOWN		(3)
#define ARROW_OFFSET		4
#define STD_ARROW_HEIGHT	3
#define STD_ARROW_WIDTH		((2 * (ARROW_OFFSET - 1)) + 1)

#define BLT_SCROLL_MODE_CANVAS	(1<<0)
#define BLT_SCROLL_MODE_LISTBOX	(1<<1)
#define BLT_SCROLL_MODE_HIERBOX	(1<<2)

static Blt_OptionParseProc ObjToPoint;
static Blt_OptionPrintProc PointToObj;
Blt_CustomOption bltPointOption =
{
    ObjToPoint, PointToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToLimitsProc;
static Blt_OptionPrintProc LimitsToObjProc;
Blt_CustomOption bltLimitsOption =
{
    ObjToLimitsProc, LimitsToObjProc, NULL, (ClientData)0
};


/*
 *---------------------------------------------------------------------------
 * Custom option parse and print procedures
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetXY --
 *
 *	Converts a string in the form "@x,y" into an XPoint structure of the x
 *	and y coordinates.
 *
 * Results:
 *	A standard TCL result. If the string represents a valid position
 *	*pointPtr* will contain the converted x and y coordinates and TCL_OK
 *	is returned.  Otherwise, TCL_ERROR is returned and interp->result will
 *	contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetXY(Tcl_Interp *interp, Tk_Window tkwin, const char *string, 
	  int *xPtr, int *yPtr)
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
    comma = strchr(string + 1, ',');
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

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPoint --
 *
 *	Convert the string representation of a legend XY position into window
 *	coordinates.  The form of the string must be "@x,y" or none.
 *
 * Results:
 *	A standard TCL result.  The symbol type is written into the
 *	widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPoint(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* New legend position string */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    XPoint *pointPtr = (XPoint *)(widgRec + offset);
    int x, y;

    if (Blt_GetXY(interp, tkwin, Tcl_GetString(objPtr), &x, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    pointPtr->x = x, pointPtr->y = y;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PointToObj --
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
PointToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    XPoint *pointPtr = (XPoint *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if ((pointPtr->x != -SHRT_MAX) && (pointPtr->y != -SHRT_MAX)) {
	char string[200];

	sprintf_s(string, 200, "@%d,%d", pointPtr->x, pointPtr->y);
	objPtr = Tcl_NewStringObj(string, -1);
    } else { 
	objPtr = Tcl_NewStringObj("", -1);
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimitsProc --
 *
 *	Converts the list of elements into zero or more pixel values which
 *	determine the range of pixel values possible.  An element can be in any
 *	form accepted by Tk_GetPixels. The list has a different meaning based
 *	upon the number of elements.
 *
 *	    # of elements:
 *
 *	    0 - the limits are reset to the defaults.
 *	    1 - the minimum and maximum values are set to this
 *		value, freezing the range at a single value.
 *	    2 - first element is the minimum, the second is the
 *		maximum.
 *	    3 - first element is the minimum, the second is the
 *		maximum, and the third is the nominal value.
 *
 *	Any element may be the empty string which indicates the default.
 *
 * Results:
 *	The return value is a standard TCL result.  The min and max fields
 *	of the range are set.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimitsProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Widget of paneset */
    Tcl_Obj *objPtr,			/* New width list */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_Limits *limitsPtr = (Blt_Limits *)(widgRec + offset);

    if (Blt_GetLimitsFromObj(interp, tkwin, objPtr, limitsPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * LimitsToObjProc --
 *
 *	Convert the limits of the pixel values allowed into a list.
 *
 * Results:
 *	The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitsToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Row/column structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_Limits *limitsPtr = (Blt_Limits *)(widgRec + offset);
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (limitsPtr->flags & LIMITS_MIN_SET) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewIntObj(limitsPtr->min));
    } else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (limitsPtr->flags & LIMITS_MAX_SET) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewIntObj(limitsPtr->max));
    } else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (limitsPtr->flags & LIMITS_NOM_SET) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewIntObj(limitsPtr->nom));
    } else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    return listObjPtr;
}

int
Blt_PointInSegments(
    Point2d *samplePtr,
    Segment2d *segments,
    int nSegments,
    double halo)
{
    Segment2d *sp, *send;
    double minDist;

    minDist = DBL_MAX;
    for (sp = segments, send = sp + nSegments; sp < send; sp++) {
	double dist;
	double left, right, top, bottom;
	Point2d p, t;

	t = Blt_GetProjection((int)samplePtr->x, (int)samplePtr->y, 
			      &sp->p, &sp->q);
	if (sp->p.x > sp->q.x) {
	    right = sp->p.x, left = sp->q.x;
	} else {
	    right = sp->q.x, left = sp->p.x;
	}
	if (sp->p.y > sp->q.y) {
	    bottom = sp->p.y, top = sp->q.y;
	} else {
	    bottom = sp->q.y, top = sp->p.y;
	}
	p.x = BOUND(t.x, left, right);
	p.y = BOUND(t.y, top, bottom);
	dist = hypot(p.x - samplePtr->x, p.y - samplePtr->y);
	if (dist < minDist) {
	    minDist = dist;
	}
    }
    return (minDist < halo);
}

int
Blt_PointInPolygon(
    Point2d *s,			/* Sample point. */
    Point2d *points,		/* Points representing the polygon. */
    int nPoints)		/* # of points in above array. */
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

int
Blt_RegionInPolygon(
    Region2d *regionPtr,
    Point2d *points,
    int nPoints,
    int enclosed)
{
    Point2d *pp, *pend;

    if (enclosed) {
	/*  
	 * All points of the polygon must be inside the rectangle.
	 */
	for (pp = points, pend = pp + nPoints; pp < pend; pp++) {
	    if ((pp->x < regionPtr->left) || (pp->x > regionPtr->right) ||
		(pp->y < regionPtr->top) || (pp->y > regionPtr->bottom)) {
		return FALSE;	/* One point is exterior. */
	    }
	}
	return TRUE;
    } else {
	Point2d r;
	/*
	 * If any segment of the polygon clips the bounding region, the
	 * polygon overlaps the rectangle.
	 */
	points[nPoints] = points[0];
	for (pp = points, pend = pp + nPoints; pp < pend; pp++) {
	    Point2d p, q;

	    p = *pp;
	    q = *(pp + 1);
	    if (Blt_LineRectClip(regionPtr, &p, &q)) {
		return TRUE;
	    }
	}
	/* 
	 * Otherwise the polygon and rectangle are either disjoint or
	 * enclosed.  Check if one corner of the rectangle is inside the
	 * polygon.
	 */
	r.x = regionPtr->left;
	r.y = regionPtr->top;
	return Blt_PointInPolygon(&r, points, nPoints);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GraphExtents --
 *
 *	Generates a bounding box representing the plotting area of the
 *	graph. This data structure is used to clip the points and line
 *	segments of the line element.
 *
 *	The clip region is the plotting area plus such arbitrary extra space.
 *	The reason we clip with a bounding box larger than the plot area is so
 *	that symbols will be drawn even if their center point isn't in the
 *	plotting area.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The bounding box is filled with the dimensions of the plotting area.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_GraphExtents(Graph *graphPtr, Region2d *regionPtr)
{
    regionPtr->left = (double)(graphPtr->hOffset - graphPtr->xPad.side1);
    regionPtr->top = (double)(graphPtr->vOffset - graphPtr->yPad.side1);
    regionPtr->right = (double)(graphPtr->hOffset + graphPtr->hRange + 
	graphPtr->xPad.side2);
    regionPtr->bottom = (double)(graphPtr->vOffset + graphPtr->vRange + 
	graphPtr->yPad.side2);
}

static int 
ClipTest (double ds, double dr, double *t1, double *t2)
{
  double t;

  if (ds < 0.0) {
      t = dr / ds;
      if (t > *t2) {
	  return FALSE;
      } 
      if (t > *t1) {
	  *t1 = t;
      }
  } else if (ds > 0.0) {
      t = dr / ds;
      if (t < *t1) {
	  return FALSE;
      } 
      if (t < *t2) {
	  *t2 = t;
      }
  } else {
      /* d = 0, so line is parallel to this clipping edge */
      if (dr < 0.0) {		/* Line is outside clipping edge */
	  return FALSE;
      }
  }
  return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LineRectClip --
 *
 *	Clips the given line segment to a rectangular region.  The coordinates
 *	of the clipped line segment are returned.  The original coordinates
 *	are overwritten.
 *
 *	Reference: 
 *	  Liang, Y-D., and B. Barsky, A new concept and method for
 *	  Line Clipping, ACM, TOG,3(1), 1984, pp.1-22.
 *
 * Results:
 *	Returns if line segment is visible within the region. The coordinates
 *	of the original line segment are overwritten by the clipped
 *	coordinates.
 *
 *---------------------------------------------------------------------------
 */
int 
Blt_LineRectClip(
    Region2d *regionPtr,	/* Rectangular region to clip. */
    Point2d *p, Point2d *q)	/* (in/out) Coordinates of original and
				 * clipped line segment. */
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
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PolyRectClip --
 *
 *	Clips the given polygon to a rectangular region.  The resulting
 *	polygon is returned. Note that the resulting polyon may be complex,
 *	connected by zero width/height segments.  The drawing routine (such as
 *	XFillPolygon) will not draw a connecting segment.
 *
 *	Reference:  
 *	  Liang Y. D. and Brian A. Barsky, "Analysis and Algorithm for
 *	  Polygon Clipping", Communications of ACM, Vol. 26,
 *	  p.868-877, 1983
 *
 * Results:
 *	Returns the number of points in the clipped polygon. The points of the
 *	clipped polygon are stored in *outputPts*.
 *
 *---------------------------------------------------------------------------
 */
#define EPSILON  FLT_EPSILON
#define AddVertex(vx, vy)	    r->x=(vx), r->y=(vy), r++, count++ 
#define LastVertex(vx, vy)	    r->x=(vx), r->y=(vy), count++ 

int 
Blt_PolyRectClip(
    Region2d *regionPtr,	/* Rectangular region clipping the polygon. */
    Point2d *points,		/* Points of polygon to be clipped. */
    int nPoints,		/* # of points in polygon. */
    Point2d *clipPts)		/* (out) Points of clipped polygon. */
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
 *
 * Blt_GetProjection --
 *
 *	Computes the projection of a point on a line.  The line (given by two
 *	points), is assumed the be infinite.
 *
 *	Compute the slope (angle) of the line and rotate it 90 degrees.  Using
 *	the slope-intercept method (we know the second line from the sample
 *	test point and the computed slope), then find the intersection of both
 *	lines. This will be the projection of the sample point on the first
 *	line.
 *
 * Results:
 *	Returns the coordinates of the projection on the line.
 *
 *---------------------------------------------------------------------------
 */
Point2d
Blt_GetProjection(
    int x, int y,		/* Screen coordinates of the sample point. */
    Point2d *p, Point2d *q)	/* Line segment to project point onto */
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

#define SetColor(c,r,g,b) ((c)->red = (int)((r) * 65535.0), \
			   (c)->green = (int)((g) * 65535.0), \
			   (c)->blue = (int)((b) * 65535.0))

/*
 *---------------------------------------------------------------------------
 *
 * Blt_AdjustViewport --
 *
 *	Adjusts the offsets of the viewport according to the scroll mode.
 *	This is to accommodate both "listbox" and "canvas" style scrolling.
 *
 *	"canvas"	The viewport scrolls within the range of world
 *			coordinates.  This way the viewport always displays
 *			a full page of the world.  If the world is smaller
 *			than the viewport, then (bizarrely) the world and
 *			viewport are inverted so that the world moves up
 *			and down within the viewport.
 *
 *	"listbox"	The viewport can scroll beyond the range of world
 *			coordinates.  Every entry can be displayed at the
 *			top of the viewport.  This also means that the
 *			scrollbar thumb weirdly shrinks as the last entry
 *			is scrolled upward.
 *
 * Results:
 *	The corrected offset is returned.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_AdjustViewport(int offset, int worldSize, int windowSize, int scrollUnits, 
		   int scrollMode)
{
    switch (scrollMode) {
    case BLT_SCROLL_MODE_CANVAS:

	/*
	 * Canvas-style scrolling allows the world to be scrolled within the
	 * window.
	 */
	if (worldSize < windowSize) {
	    if ((worldSize - offset) > windowSize) {
		offset = worldSize - windowSize;
	    }
	    if (offset > 0) {
		offset = 0;
	    }
	} else {
	    if ((offset + windowSize) > worldSize) {
		offset = worldSize - windowSize;
	    }
	    if (offset < 0) {
		offset = 0;
	    }
	}
	break;

    case BLT_SCROLL_MODE_LISTBOX:
	if (offset < 0) {
	    offset = 0;
	}
	if (offset >= worldSize) {
	    offset = worldSize - scrollUnits;
	}
	break;

    case BLT_SCROLL_MODE_HIERBOX:

	/*
	 * Hierbox-style scrolling allows the world to be scrolled within the
	 * window.
	 */
	if ((offset + windowSize) > worldSize) {
	    offset = worldSize - windowSize;
	}
	if (offset < 0) {
	    offset = 0;
	}
	break;
    }
    return offset;
}

int
Blt_GetScrollInfoFromObj(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv,
			 int *offsetPtr, int worldSize, int windowSize, 
			 int scrollUnits, int scrollMode)
{
    char c;
    const char *string;
    int length;
    int offset;

    offset = *offsetPtr;
    string = Tcl_GetStringFromObj(objv[0], &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "scroll", length) == 0)) {
	double fract;
	int count;

	if (objc != 3) {
	    return TCL_ERROR;
	}
	/* Scroll number unit/page */
	if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	string = Tcl_GetStringFromObj(objv[2], &length);
	c = string[0];
	if ((c == 'u') && (strncmp(string, "units", length) == 0)) {
	    fract = (double)count *scrollUnits;
	} else if ((c == 'p') && (strncmp(string, "pages", length) == 0)) {
	    /* A page is 90% of the view-able window. */
	    fract = (double)count * windowSize * 0.9;
	} else {
	    Tcl_AppendResult(interp, "unknown \"scroll\" units \"", 
		     Tcl_GetString(objv[2]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	offset += (int)fract;
    } else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
	double fract;

	if (objc != 2) {
	    return TCL_ERROR;
	}
	/* moveto fraction */
	if (Tcl_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
	    return TCL_ERROR;
	}
	offset = (int)(worldSize * fract);
    } else {
	double fract;
	int count;

	/* Treat like "scroll units" */
	if (Tcl_GetIntFromObj(interp, objv[0], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	fract = (double)count *scrollUnits;
	offset += (int)fract;
    }
    *offsetPtr = Blt_AdjustViewport(offset, worldSize, windowSize, scrollUnits,
	scrollMode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UpdateScrollbar --
 *
 * 	Invoke a TCL command to the scrollbar, defining the new position and
 * 	length of the scroll. See the Tk documentation for further information
 * 	on the scrollbar.  It is assumed the scrollbar command prefix is
 * 	valid.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Scrollbar is commanded to change position and/or size.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UpdateScrollbar(
    Tcl_Interp *interp,
    Tcl_Obj *scrollCmdObjPtr,		/* Scrollbar command prefix. May be
					 * several words */
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

/* -------------------------------------------------------------------------- */
/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPrivateGCFromDrawable --
 *
 *      Like Tk_GetGC, but doesn't share the GC with any other widget.  This
 *      is needed because the certain GC parameters (like dashes) can not be
 *      set via XCreateGC, therefore there is no way for Tk's hashing
 *      mechanism to recognize that two such GCs differ.
 *
 * Results:
 *      A new GC is returned.
 *
 *---------------------------------------------------------------------------
 */
GC
Blt_GetPrivateGCFromDrawable(
    Display *display,
    Drawable drawable,
    unsigned long gcMask,
    XGCValues *valuePtr)
{
    GC newGC;

    newGC = XCreateGC(display, drawable, gcMask, valuePtr);
    return newGC;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPrivateGC --
 *
 *      Like Tk_GetGC, but doesn't share the GC with any other widget.  This
 *      is needed because the certain GC parameters (like dashes) can not be
 *      set via XCreateGC, therefore there is no way for Tk's hashing
 *      mechanism to recognize that two such GCs differ.
 *
 * Results:
 *      A new GC is returned.
 *
 *---------------------------------------------------------------------------
 */
GC
Blt_GetPrivateGC(
    Tk_Window tkwin,
    unsigned long gcMask,
    XGCValues *valuePtr)
{
    GC gc;
    Pixmap pixmap;
    Drawable drawable;
    Display *display;

    pixmap = None;
    drawable = Tk_WindowId(tkwin);
    display = Tk_Display(tkwin);
    if (drawable == None) {
	Drawable root;
	int depth;

	root = Tk_RootWindow(tkwin);
	depth = Tk_Depth(tkwin);

	if (depth == DefaultDepth(display, Tk_ScreenNumber(tkwin))) {
	    drawable = root;
	} else {
	    pixmap = Tk_GetPixmap(display, root, 1, 1, depth);
	    drawable = pixmap;
	    Blt_SetDrawableAttribs(display, drawable, 1, 1, depth, 
		Tk_Colormap(tkwin), Tk_Visual(tkwin));
	}
    }
    gc = Blt_GetPrivateGCFromDrawable(display, drawable, gcMask, valuePtr);
    if (pixmap != None) {
	Tk_FreePixmap(display, pixmap);
    }
    return gc;
}

void
Blt_FreePrivateGC(Display *display, GC gc)
{
    Tk_FreeXId(display, (XID) XGContextFromGC(gc));
    XFreeGC(display, gc);
}

void
Blt_SetDashes(Display *display, GC gc, Blt_Dashes *dashesPtr)
{
    XSetDashes(display, gc, dashesPtr->offset, (const char *)dashesPtr->values,
	(int)strlen((char *)dashesPtr->values));
}

void
Blt_ScreenDPI(Tk_Window tkwin, unsigned int *xPtr, unsigned int *yPtr) 
{
    Screen *screen;

#define MM_INCH		25.4
    screen = Tk_Screen(tkwin);
    *xPtr = (unsigned int)((WidthOfScreen(screen) * MM_INCH) / 
			   WidthMMOfScreen(screen));
    *yPtr = (unsigned int)((HeightOfScreen(screen) * MM_INCH) / 
			   HeightMMOfScreen(screen));
}

void
Blt_Draw2DSegments(
    Display *display,
    Drawable drawable,
    GC gc,
    Segment2d *segments,
    int nSegments)
{
    XSegment *dp, *xsegments;
    Segment2d *sp, *send;

    xsegments = malloc(nSegments * sizeof(XSegment));
    if (xsegments == NULL) {
	return;
    }
    dp = xsegments;
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

long 
Blt_MaxRequestSize(Display *display, size_t elemSize) 
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

