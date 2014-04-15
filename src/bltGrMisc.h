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

#include <tcl.h>
#ifdef USE_TCL_STUBS
#include <tclInt.h>
#endif

#include <tk.h>
#ifdef USE_TK_STUBS
#include <tkInt.h>
#endif

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

typedef struct {
  short int width;
  short int height;
} Dim2D;

extern char* dupstr(const char*);

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

#endif
