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

#ifndef _BLT_PS_H
#define _BLT_PS_H

#include "bltConfig.h"
#include "bltText.h"

#define POSTSCRIPT_BUFSIZ	((BUFSIZ*2)-1)
struct _Blt_Ps {
    Tcl_Interp* interp;
    Tcl_DString dString;
    PageSetup *setupPtr;
    char scratchArr[POSTSCRIPT_BUFSIZ+1];
};

typedef struct _Blt_Ps PostScript;

#define PS_MAXPECT	(1<<4)

typedef struct _Blt_Ps *Blt_Ps;

extern Blt_Ps Blt_Ps_Create(Tcl_Interp* interp, PageSetup *setupPtr);

extern void Blt_Ps_Free(Blt_Ps ps);

extern const char *Blt_Ps_GetValue(Blt_Ps ps, int *lengthPtr);

extern char *Blt_Ps_GetScratchBuffer(Blt_Ps ps);

extern void Blt_Ps_Append(Blt_Ps ps, const char *string);

extern void Blt_Ps_VarAppend TCL_VARARGS(Blt_Ps, ps);

extern void Blt_Ps_Format TCL_VARARGS(Blt_Ps, ps);

extern void Blt_Ps_SetClearBackground(Blt_Ps ps);

extern int Blt_Ps_IncludeFile(Tcl_Interp* interp, Blt_Ps ps, 
			      const char *fileName);

extern int Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int w, int h);

extern void Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int w, int h);



extern void Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth);

extern void Blt_Ps_XSetBackground(Blt_Ps ps, XColor* colorPtr);

extern void Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display, 
				  Pixmap bitmap, int width, int height);

extern void Blt_Ps_XSetForeground(Blt_Ps ps, XColor* colorPtr);

extern void Blt_Ps_XSetFont(Blt_Ps ps, Tk_Font font);

extern void Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr);

extern void Blt_Ps_XSetLineAttributes(Blt_Ps ps, XColor* colorPtr,
				      int lineWidth, Blt_Dashes *dashesPtr, int capStyle, int joinStyle);

extern void Blt_Ps_XSetStipple(Blt_Ps ps, Display *display, Pixmap bitmap);

extern void Blt_Ps_Polyline(Blt_Ps ps, Point2d *screenPts, int nScreenPts);

extern void Blt_Ps_XDrawLines(Blt_Ps ps, XPoint *points, int n);

extern void Blt_Ps_XDrawSegments(Blt_Ps ps, XSegment *segments, 
				 int nSegments);

extern void Blt_Ps_DrawPolyline(Blt_Ps ps, Point2d *points, int n);

extern void Blt_Ps_Draw2DSegments(Blt_Ps ps, Segment2d *segments,
				  int nSegments);

extern void Blt_Ps_Draw3DRectangle(Blt_Ps ps, Tk_3DBorder border, 
				   double x, double y, int width, int height, int borderWidth, int relief);

extern void Blt_Ps_Fill3DRectangle(Blt_Ps ps, Tk_3DBorder border, double x,
				   double y, int width, int height, int borderWidth, int relief);

extern void Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y, 
				  int width, int height);

extern void Blt_Ps_XFillRectangles(Blt_Ps ps, XRectangle *rects, int n);

extern void Blt_Ps_XFillPolygon(Blt_Ps ps, Point2d *screenPts, 
				int nScreenPts);

extern void Blt_Ps_DrawPhoto(Blt_Ps ps, Tk_PhotoHandle photoToken,
			     double x, double y);

extern void Blt_Ps_XDrawWindow(Blt_Ps ps, Tk_Window tkwin, 
			       double x, double y);

extern void Blt_Ps_DrawText(Blt_Ps ps, const char *string, 
			    TextStyle *attrPtr, double x, double y);

extern void Blt_Ps_DrawBitmap(Blt_Ps ps, Display *display, Pixmap bitmap, 
			      double scaleX, double scaleY);

extern void Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle);

extern void Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle);

extern void Blt_Ps_PolylineFromXPoints(Blt_Ps ps, XPoint *points, int n);

extern void Blt_Ps_Polygon(Blt_Ps ps, Point2d *screenPts, int nScreenPts);

extern void Blt_Ps_SetPrinting(Blt_Ps ps, int value);

#endif /* BLT_PS_H */
