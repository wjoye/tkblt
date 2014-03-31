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

#ifndef __bltgrelemop_h__
#define __bltgrelemop_h__

extern "C" {
#include <bltVector.h>
};

#include "bltGrPen.h"

extern void Blt_FreePen(Pen* penPtr);


#define SHOW_NONE	0
#define SHOW_X		1
#define SHOW_Y		2
#define SHOW_BOTH	3

#define SEARCH_POINTS	0	/* Search for closest data point. */
#define SEARCH_TRACES	1	/* Search for closest point on trace.
				 * Interpolate the connecting line segments if
				 * necessary. */
#define SEARCH_AUTO	2	/* Automatically determine whether to search
				 * for data points or traces.  Look for traces
				 * if the linewidth is > 0 and if there is
				 * more than one data point. */

#define	LABEL_ACTIVE 	(1<<9)	/* Non-zero indicates that the element's entry
				 * in the legend should be drawn in its active
				 * foreground and background colors. */
#define SCALE_SYMBOL	(1<<10)

#define NUMBEROFPOINTS(e) MIN( \
			      (e)->coords.x ? (e)->coords.x->nValues : 0, \
			      (e)->coords.y ? (e)->coords.y->nValues : 0 \
			       )

#define NORMALPEN(e) ((((e)->normalPenPtr == NULL) ? (e)->builtinPenPtr : (e)->normalPenPtr))

#define SetRange(l)							\
  ((l).range = ((l).max > (l).min) ? ((l).max - (l).min) : DBL_EPSILON)
#define SetScale(l)				\
  ((l).scale = 1.0 / (l).range)
#define SetWeight(l, lo, hi)			\
  ((l).min = (lo), (l).max = (hi), SetRange(l))

extern const char* fillObjOption[];
extern Tk_CustomOptionSetProc StyleSetProc;
extern Tk_CustomOptionGetProc StyleGetProc;
extern Tk_CustomOptionRestoreProc StyleRestoreProc;
extern Tk_CustomOptionFreeProc StyleFreeProc;

extern int Blt_GetElement(Tcl_Interp* interp, Graph *graphPtr, 
			  Tcl_Obj *objPtr, Element **elemPtrPtr);
#endif
