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

extern "C" {
#include "bltOp.h"
#include "bltBind.h"
};

#include "bltGraph.h"
#include "bltGrXAxisOp.h"
#include "bltGrAxis.h"
#include "bltGrAxisOp.h"

using namespace Blt;

extern int AxisCgetOp(Tcl_Interp* interp, Axis* axisPtr, 
		      int objc, Tcl_Obj* const objv[]);
extern int AxisConfigureOp(Tcl_Interp* interp, Axis* axisPtr, 
			   int objc, Tcl_Obj* const objv[]);
extern int AxisActivateOp(Tcl_Interp* interp, Axis* axisPtr, 
			  int objc, Tcl_Obj* const objv[]);
extern int AxisInvTransformOp(Tcl_Interp* interp, Axis* axisPtr, 
			      int objc, Tcl_Obj* const objv[]);
extern int AxisLimitsOp(Tcl_Interp* interp, Axis* axisPtr, 
			int objc, Tcl_Obj* const objv[]);
extern int AxisMarginOp(Tcl_Interp* interp, Axis* axisPtr, 
			int objc, Tcl_Obj* const objv[]);
extern int AxisTransformOp(Tcl_Interp* interp, Axis* axisPtr, 
			   int objc, Tcl_Obj* const objv[]);
extern int AxisTypeOp(Tcl_Interp* interp, Axis* axisPtr, 
		      int objc, Tcl_Obj* const objv[]);
extern int AxisViewOp(Tcl_Interp* interp, Axis* axisPtr, 
		      int objc, Tcl_Obj* const objv[]);

static int lastMargin;


// Ops

static int BindOp(Tcl_Interp* interp, Axis* axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;

  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable_, graphPtr->axisTag(axisPtr->name_), objc-3, objv+3);
}
          
static int UseOp(Tcl_Interp* interp, Axis* axisPtr, 
		 int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph *)axisPtr;
  GraphOptions* gops = (GraphOptions*)graphPtr->ops_;

  Blt_Chain chain = gops->margins[lastMargin].axes;
  if (objc == 3) {
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (Blt_ChainLink link = Blt_Chain_FirstLink(chain); link;
	 link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      Tcl_ListObjAppendElement(interp, listObjPtr,
			       Tcl_NewStringObj(axisPtr->name_, -1));
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }

  ClassId classId;
  if ((lastMargin == MARGIN_BOTTOM) || (lastMargin == MARGIN_TOP))
    classId = (gops->inverted) ? CID_AXIS_Y : CID_AXIS_X;
  else
    classId = (gops->inverted) ? CID_AXIS_X : CID_AXIS_Y;

  int axisObjc;  
  Tcl_Obj **axisObjv;
  if (Tcl_ListObjGetElements(interp, objv[3], &axisObjc, &axisObjv) != TCL_OK)
    return TCL_ERROR;

  for (Blt_ChainLink link = Blt_Chain_FirstLink(chain); link; 
       link = Blt_Chain_NextLink(link)) {
    Axis* axisPtr;

    axisPtr = (Axis*)Blt_Chain_GetValue(link);
    axisPtr->link = NULL;
    axisPtr->use_ =0;
    // Clear the axis type if it's not currently used
    if (axisPtr->refCount_ == 0)
      axisPtr->setClass(CID_NONE);
  }

  Blt_Chain_Reset(chain);
  for (int ii=0; ii<axisObjc; ii++) {
    Axis* axisPtr;
    if (graphPtr->getAxis(axisObjv[ii], &axisPtr) != TCL_OK)
      return TCL_ERROR;

    if (axisPtr->classId_ == CID_NONE)
      axisPtr->setClass(classId);
    else if (axisPtr->classId_ != classId) {
      Tcl_AppendResult(interp, "wrong type axis \"", 
		       axisPtr->name_, "\": can't use ", 
		       axisPtr->className_, " type axis.", NULL); 
      return TCL_ERROR;
    }
    if (axisPtr->link) {
      /* Move the axis from the old margin's "use" list to the new. */
      Blt_Chain_UnlinkLink(axisPtr->chain, axisPtr->link);
      Blt_Chain_AppendLink(chain, axisPtr->link);
    }
    else
      axisPtr->link = Blt_Chain_Append(chain, axisPtr);

    axisPtr->chain = chain;
    axisPtr->use_ =1;
  }

  graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
  // When any axis changes, we need to layout the entire graph.
  graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
  graphPtr->eventuallyRedraw();

  return TCL_OK;
}

static int CgetOp(ClientData clientData,Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  GraphOptions* gops = (GraphOptions*)graphPtr->ops_;
  int margin = (gops->inverted) ? MARGIN_LEFT : MARGIN_BOTTOM;
  Axis* axisPtr = Blt_GetFirstAxis(gops->margins[margin].axes);

  return AxisCgetOp(interp, axisPtr, objc, objv);
}

static int ConfigureOp(ClientData clientData, Tcl_Interp* interp,
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  GraphOptions* gops = (GraphOptions*)graphPtr->ops_;
  int margin = (gops->inverted) ? MARGIN_LEFT : MARGIN_BOTTOM;
  Axis* axisPtr = Blt_GetFirstAxis(gops->margins[margin].axes);

  return AxisConfigureOp(interp, axisPtr, objc, objv);
}

const TkEnsemble xaxisEnsemble[] = {
    { "cget", 		CgetOp,0 },
    { "configure", 	ConfigureOp,0 },
    { 0,0,0 }
};

/*
static Blt_OpSpec axisOps[] = {
  {"activate",     1, (void*)AxisActivateOp,     3, 3, "",},
  {"bind",         1, (void*)BindOp,         2, 5, "sequence command",},
  {"cget",         2, (void*)AxisCgetOp,         4, 4, "option",},
  {"configure",    2, (void*)AxisConfigureOp,    3, 0, "?option value?...",},
  {"deactivate",   1, (void*)AxisActivateOp,     3, 3, "",},
  {"invtransform", 1, (void*)AxisInvTransformOp, 4, 4, "value",},
  {"limits",       1, (void*)AxisLimitsOp,       3, 3, "",},
  {"transform",    1, (void*)AxisTransformOp,    4, 4, "value",},
  {"use",          1, (void*)UseOp,          3, 4, "?axisName?",},
  {"view",         1, (void*)AxisViewOp,         3, 6, "?moveto fract? ",},
};

static int nAxisOps = sizeof(axisOps) / sizeof(Blt_OpSpec);

typedef int (GraphAxisProc)(Tcl_Interp* interp, Axis* axisPtr, 
			    int objc, Tcl_Obj* const objv[]);

int Blt_XAxisOp(Tcl_Interp* interp, Graph* graphPtr, int margin, 
		  int objc, Tcl_Obj* const objv[])
{
  GraphOptions* gops = (GraphOptions*)graphPtr->ops_;
  GraphAxisProc* proc = (GraphAxisProc*)Blt_GetOpFromObj(interp, nAxisOps, axisOps, BLT_OP_ARG2, objc, objv, 0);
  if (!proc)
    return TCL_ERROR;

  if (proc == UseOp) {
    lastMargin = margin;
    return (*proc)(interp, (Axis*)graphPtr, objc, objv);
  } 
  else {
    Axis* axisPtr = Blt_GetFirstAxis(gops->margins[margin].axes);
    if (!axisPtr)
      return TCL_OK;
    return (*proc)(interp, axisPtr, objc, objv);
  }
}
*/
