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

#include "bltGraph.h"
#include "bltGrXAxisOp.h"
#include "bltGrAxis.h"
#include "bltGrAxisOp.h"

using namespace Blt;

static int lastMargin;

static Axis* GetAxisFromCmd(ClientData clientData, Tcl_Obj* obj)
{
  Graph* graphPtr = (Graph*)clientData;
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;

  int margin;
  const char* name = Tcl_GetString(obj);
  if (!strcmp(name,"xaxis"))
    margin = (ops->inverted) ? MARGIN_LEFT : MARGIN_BOTTOM;
  else if (!strcmp(name,"yaxis"))
    margin = (ops->inverted) ? MARGIN_BOTTOM : MARGIN_LEFT;
  else if (!strcmp(name,"x2axis"))
    margin = (ops->inverted) ? MARGIN_RIGHT : MARGIN_TOP;
  else if (!strcmp(name,"y2axis"))
    margin = (ops->inverted) ? MARGIN_TOP : MARGIN_RIGHT;
  else
    return NULL;

  return Blt_GetFirstAxis(ops->margins[margin].axes);
}

static int CgetOp(ClientData clientData, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisCgetOp(axisPtr, interp, objc, objv);
}

static int ConfigureOp(ClientData clientData, Tcl_Interp* interp,
		       int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisConfigureOp(axisPtr, interp, objc, objv);
}

static int ActivateOp(ClientData clientData, Tcl_Interp* interp, 
		      int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisActivateOp(axisPtr, interp, objc, objv);
}

static int BindOp(ClientData clientData, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable_, graphPtr->axisTag(axisPtr->name_), objc-3, objv+3);
}

static int InvTransformOp(ClientData clientData, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisInvTransformOp(axisPtr, interp, objc-1, objv+1);
}

static int LimitsOp(ClientData clientData, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisLimitsOp(axisPtr, interp, objc-1, objv+1);
}

static int TransformOp(ClientData clientData, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisTransformOp(axisPtr, interp, objc-1, objv+1);
}

static int UseOp(ClientData clientData, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
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

static int ViewOp(ClientData clientData, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Axis* axisPtr = GetAxisFromCmd(clientData, objv[1]);
  return AxisViewOp(axisPtr, interp, objc-1, objv+1);
}

const TkEnsemble xaxisEnsemble[] = {
  {"activate",     ActivateOp, 0},
  {"bind",         BindOp, 0},
  {"cget",         CgetOp, 0},
  {"configure",    ConfigureOp, 0},
  {"deactivate",   ActivateOp, 0},
  {"invtransform", InvTransformOp, 0},
  {"limits",       LimitsOp, 0},
  {"transform",    TransformOp, 0},
  {"use",          UseOp, 0},
  {"view",         ViewOp, 0},
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
