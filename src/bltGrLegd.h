/*
 * bltGrLegd.h --
 *
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

#ifndef _BLT_GR_LEGEND_H
#define _BLT_GR_LEGEND_H

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

BLT_EXTERN int Blt_CreateLegend(Graph *graphPtr);
BLT_EXTERN void Blt_DestroyLegend(Graph *graphPtr);
BLT_EXTERN void Blt_DrawLegend(Graph *graphPtr, Drawable drawable);
BLT_EXTERN void Blt_MapLegend(Graph *graphPtr, int width, int height);
BLT_EXTERN int Blt_LegendOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);
BLT_EXTERN int Blt_Legend_Site(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_Width(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_Height(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_IsHidden(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_IsRaised(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_X(Graph *graphPtr);
BLT_EXTERN int Blt_Legend_Y(Graph *graphPtr);
BLT_EXTERN void Blt_Legend_RemoveElement(Graph *graphPtr, Element *elemPtr);
BLT_EXTERN void Blt_Legend_EventuallyRedraw(Graph *graphPtr);

#endif /* BLT_GR_LEGEND_H */
