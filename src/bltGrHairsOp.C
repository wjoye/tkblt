/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
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

extern "C" {
#include "bltGraph.h"
#include "bltOp.h"
}

#include "bltGrHairs.h"

static int CrosshairsObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
				  int objc, Tcl_Obj* const objv[])
{
  Crosshairs* chPtr = graphPtr->crosshairs_;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)chPtr->ops_, chPtr->optionTable_, 
			objc, objv, graphPtr->tkwin_, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    graphPtr->flags |= mask;
    chPtr->configure();

    break; 
  }

  if (!error) {
    Tk_FreeSavedOptions(&savedOptions);
    return TCL_OK;
  }
  else {
    Tcl_SetObjResult(interp, errorResult);
    Tcl_DecrRefCount(errorResult);
    return TCL_ERROR;
  }
}

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Crosshairs* chPtr = graphPtr->crosshairs_;
  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)chPtr->ops_, 
				      chPtr->optionTable_,
				      objv[3], graphPtr->tkwin_);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp,
		       int objc, Tcl_Obj* const objv[])
{
  Crosshairs* chPtr = graphPtr->crosshairs_;
  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(interp, (char*)chPtr->ops_, 
				       chPtr->optionTable_, 
				       (objc == 4) ? objv[3] : NULL, 
				       graphPtr->tkwin_);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return CrosshairsObjConfigure(interp, graphPtr, objc-3, objv+3);
}

static int OnOp(Graph* graphPtr, Tcl_Interp* interp, 
		int objc, Tcl_Obj* const objv[])
{
  Crosshairs *chPtr = graphPtr->crosshairs_;
  CrosshairsOptions* ops = (CrosshairsOptions*)chPtr->ops_;

  if (ops->hide) {
    chPtr->on();
    ops->hide = 0;
  }
  return TCL_OK;
}

static int OffOp(Graph* graphPtr, Tcl_Interp* interp,
		 int objc, Tcl_Obj* const objv[])
{
  Crosshairs *chPtr = graphPtr->crosshairs_;
  CrosshairsOptions* ops = (CrosshairsOptions*)chPtr->ops_;

  if (!ops->hide) {
    chPtr->off();
    ops->hide = 1;
  }
  return TCL_OK;
}

static int ToggleOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  Crosshairs *chPtr = graphPtr->crosshairs_;
  CrosshairsOptions* ops = (CrosshairsOptions*)chPtr->ops_;

  ops->hide = (ops->hide == 0);
  if (ops->hide)
    chPtr->off();
  else
    chPtr->on();

  return TCL_OK;
}

static Blt_OpSpec xhairOps[] =
  {
    {"cget", 2, (void*)CgetOp, 4, 4, "option",},
    {"configure", 2, (void*)ConfigureOp, 3, 0, "?options...?",},
    {"off", 2, (void*)OffOp, 3, 3, "",},
    {"on", 2, (void*)OnOp, 3, 3, "",},
    {"toggle", 1, (void*)ToggleOp, 3, 3, "",},
  };
static int nXhairOps = sizeof(xhairOps) / sizeof(Blt_OpSpec);

typedef int (GraphCrosshairProc)(Graph* graphPtr, Tcl_Interp* interp, 
				 int objc, Tcl_Obj* const objv[]);

int Blt_CrosshairsOp(Graph* graphPtr, Tcl_Interp* interp,
		     int objc, Tcl_Obj* const objv[])
{
  GraphCrosshairProc* proc = (GraphCrosshairProc*)Blt_GetOpFromObj(interp, nXhairOps, xhairOps, BLT_OP_ARG2, objc, objv, 0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(graphPtr, interp, objc, objv);
}

void Blt_EnableCrosshairs(Graph* graphPtr)
{
  Crosshairs* chPtr = graphPtr->crosshairs_;
  CrosshairsOptions* ops = (CrosshairsOptions*)chPtr->ops_;
  if (!ops->hide)
    graphPtr->crosshairs_->on();
}

void Blt_DisableCrosshairs(Graph* graphPtr)
{
  Crosshairs* chPtr = graphPtr->crosshairs_;
  CrosshairsOptions* ops = (CrosshairsOptions*)chPtr->ops_;
  if (!ops->hide)
    graphPtr->crosshairs_->off();
}


