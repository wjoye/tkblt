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

#ifndef __BltGrAxisOp_h__
#define __BltGrAxisOp_h__

extern int Blt_AxisOp(Graph* graphPtr, Tcl_Interp* interp, 
		      int objc, Tcl_Obj* const objv[]);

extern Tcl_FreeProc FreeAxis;
extern int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr);
extern Point2d Blt_InvMap2D(Graph* graphPtr, double x, double y, 
			    Axis2d *pairPtr);
extern Point2d Blt_Map2D(Graph* graphPtr, double x, double y, 
			 Axis2d *pairPtr);

extern void Blt_AdjustAxisPointers(Graph* graphPtr);
extern void Blt_UpdateAxisBackgrounds(Graph* graphPtr);
extern void Blt_GridsToPostScript(Graph* graphPtr, Blt_Ps ps);
extern Axis *Blt_GetFirstAxis(Blt_Chain chain);
extern Axis *Blt_NearestAxis(Graph* graphPtr, int x, int y);

#endif
