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

#include "bltMath.h"

extern "C" {
#include "bltGraph.h"
#include "bltOp.h"
}

#include "bltGrAxis.h"
#include "bltGrAxisOp.h"
#include "bltGrElem.h"

typedef int (GraphDefAxisProc)(Tcl_Interp* interp, Axis *axisPtr, 
			       int objc, Tcl_Obj* const objv[]);
typedef int (GraphAxisProc)(Tcl_Interp* interp, Graph* graphPtr, 
			    int objc, Tcl_Obj* const objv[]);

extern Tcl_FreeProc FreeAxis;
extern int ConfigureAxis(Axis *axisPtr);
extern int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr);
extern int GetAxisScrollInfo(Tcl_Interp* interp, 
			     int objc, Tcl_Obj* const objv[],
			     double *offsetPtr, double windowSize,
			     double scrollUnits, double scale);
extern int CreateAxis(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[]);

extern int AxisObjConfigure(Tcl_Interp* interp, Axis* axis,
			    int objc, Tcl_Obj* const objv[]);

static double Clamp(double x) 
{
  return (x < 0.0) ? 0.0 : (x > 1.0) ? 1.0 : x;
}

static int lastMargin;

// Default

int Blt_CreateAxes(Graph* graphPtr)
{
  for (int ii = 0; ii < 4; ii++) {
    int isNew;
    Tcl_HashEntry* hPtr = 
      Tcl_CreateHashEntry(&graphPtr->axes.table, axisNames[ii].name, &isNew);
    Blt_Chain chain = Blt_Chain_Create();

    Axis* axisPtr = new Axis(graphPtr, axisNames[ii].name, ii);
    if (!axisPtr)
      return TCL_ERROR;
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();

    axisPtr->hashPtr_ = hPtr;
    Tcl_SetHashValue(hPtr, axisPtr);

    axisPtr->refCount_ = 1;	/* Default axes are assumed in use. */
    axisPtr->margin_ = ii;
    axisPtr->use_ =1;
    
    axisPtr->setClass(!(ii&1) ? CID_AXIS_X : CID_AXIS_Y);

    if (Tk_InitOptions(graphPtr->interp, (char*)axisPtr->ops(), 
		       axisPtr->optionTable(), graphPtr->tkwin) != TCL_OK)
      return TCL_ERROR;

    if (ConfigureAxis(axisPtr) != TCL_OK)
      return TCL_ERROR;

    if ((axisPtr->margin_ == MARGIN_RIGHT) || (axisPtr->margin_ == MARGIN_TOP))
      ops->hide = 1;

    graphPtr->axisChain[ii] = chain;
    axisPtr->link = Blt_Chain_Append(chain, axisPtr);
    axisPtr->chain = chain;
  }
  return TCL_OK;
}

// Configure

static int CgetOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;

  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)axisPtr->ops(),
				      axisPtr->optionTable(),
				      objv[3], graphPtr->tkwin);
  if (!objPtr)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Tcl_Interp* interp, Axis *axisPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;

  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, (char*)axisPtr->ops(), 
				       axisPtr->optionTable(), 
				       (objc == 4) ? objv[3] : NULL, 
				       graphPtr->tkwin);
    if (!objPtr)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return AxisObjConfigure(interp, axisPtr, objc-3, objv+3);
}

// Ops

static int ActivateOp(Tcl_Interp* interp, Axis *axisPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;
  const char *string;

  string = Tcl_GetString(objv[2]);
  if (string[0] == 'a')
    axisPtr->flags |= ACTIVE;
  else
    axisPtr->flags &= ~ACTIVE;

  if (!ops->hide && axisPtr->use_) {
    graphPtr->flags |= DRAW_MARGINS | CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
  }

  return TCL_OK;
}

static int BindOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;

  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, Blt_MakeAxisTag(graphPtr, axisPtr->name()), objc-3, objv+3);
}
          
static int InvTransformOp(Tcl_Interp* interp, Axis *axisPtr, 
			  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

  int sy;
  if (Tcl_GetIntFromObj(interp, objv[3], &sy) != TCL_OK)
    return TCL_ERROR;

  // Is the axis vertical or horizontal?
  // Check the site where the axis was positioned.  If the axis is
  // virtual, all we have to go on is how it was mapped to an
  // element (using either -mapx or -mapy options).  
  double y = axisPtr->isHorizontal() ? 
    axisPtr->invHMap(sy) : axisPtr->invVMap(sy);

  Tcl_SetDoubleObj(Tcl_GetObjResult(interp), y);
  return TCL_OK;
}

static int LimitsOp(Tcl_Interp* interp, Axis *axisPtr, 
		    int objc, Tcl_Obj* const objv[])
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

  double min, max;
  if (ops->logScale) {
    min = EXP10(axisPtr->axisRange_.min);
    max = EXP10(axisPtr->axisRange_.max);
  } 
  else {
    min = axisPtr->axisRange_.min;
    max = axisPtr->axisRange_.max;
  }

  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(min));
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(max));

  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int MarginOp(Tcl_Interp* interp, Axis *axisPtr, 
		    int objc, Tcl_Obj* const objv[])
{
  const char *marginName = "";
  if (axisPtr->use_)
    marginName = axisNames[axisPtr->margin_].name;

  Tcl_SetStringObj(Tcl_GetObjResult(interp), marginName, -1);
  return TCL_OK;
}

static int TransformOp(Tcl_Interp* interp, Axis *axisPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

  double x;
  if (Blt_ExprDoubleFromObj(interp, objv[3], &x) != TCL_OK)
    return TCL_ERROR;

  if (axisPtr->isHorizontal())
    x = axisPtr->hMap(x);
  else
    x = axisPtr->vMap(x);

  Tcl_SetIntObj(Tcl_GetObjResult(interp), (int)x);
  return TCL_OK;
}

static int TypeOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  const char *typeName;

  typeName = "";
  if (axisPtr->use_) {
    if (axisNames[axisPtr->margin_].classId == CID_AXIS_X)
      typeName = "x";
    else if (axisNames[axisPtr->margin_].classId == CID_AXIS_Y)
      typeName = "y";
  }
  Tcl_SetStringObj(Tcl_GetObjResult(interp), typeName, -1);
  return TCL_OK;
}

static int UseOp(Tcl_Interp* interp, Axis *axisPtr, 
		 int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph *)axisPtr;

  Blt_Chain chain = graphPtr->margins[lastMargin].axes;
  if (objc == 3) {
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (Blt_ChainLink link = Blt_Chain_FirstLink(chain); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      Tcl_ListObjAppendElement(interp, listObjPtr,
			       Tcl_NewStringObj(axisPtr->name(), -1));
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  ClassId classId;
  if ((lastMargin == MARGIN_BOTTOM) || (lastMargin == MARGIN_TOP))
    classId = (graphPtr->inverted) ? CID_AXIS_Y : CID_AXIS_X;
  else
    classId = (graphPtr->inverted) ? CID_AXIS_X : CID_AXIS_Y;

  int axisObjc;  
  Tcl_Obj **axisObjv;
  if (Tcl_ListObjGetElements(interp, objv[3], &axisObjc, &axisObjv) 
      != TCL_OK) {
    return TCL_ERROR;
  }
  for (Blt_ChainLink link = Blt_Chain_FirstLink(chain); link!= NULL; 
       link = Blt_Chain_NextLink(link)) {
    Axis *axisPtr;

    axisPtr = (Axis*)Blt_Chain_GetValue(link);
    axisPtr->link = NULL;
    axisPtr->use_ =0;
    /* Clear the axis type if it's not currently used.*/
    if (axisPtr->refCount_ == 0)
      axisPtr->setClass(CID_NONE);
  }
  Blt_Chain_Reset(chain);
  for (int i=0; i<axisObjc; i++) {
    Axis *axisPtr;
    if (GetAxisFromObj(interp, graphPtr, axisObjv[i], &axisPtr) != TCL_OK)
      return TCL_ERROR;

    if (axisPtr->classId() == CID_NONE)
      axisPtr->setClass(classId);
    else if (axisPtr->classId() != classId) {
      Tcl_AppendResult(interp, "wrong type axis \"", 
		       axisPtr->name(), "\": can't use ", 
		       axisPtr->className(), " type axis.", NULL); 
      return TCL_ERROR;
    }
    if (axisPtr->link) {
      /* Move the axis from the old margin's "use" list to the new. */
      Blt_Chain_UnlinkLink(axisPtr->chain, axisPtr->link);
      Blt_Chain_AppendLink(chain, axisPtr->link);
    } else {
      axisPtr->link = Blt_Chain_Append(chain, axisPtr);
    }
    axisPtr->chain = chain;
    axisPtr->use_ =1;
  }
  graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
  /* When any axis changes, we need to layout the entire graph.  */
  graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static int ViewOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;
  double worldMin = axisPtr->valueRange_.min;
  double worldMax = axisPtr->valueRange_.max;
  /* Override data dimensions with user-selected limits. */
  if (!isnan(axisPtr->scrollMin_))
    worldMin = axisPtr->scrollMin_;

  if (!isnan(axisPtr->scrollMax_))
    worldMax = axisPtr->scrollMax_;

  double viewMin = axisPtr->min_;
  double viewMax = axisPtr->max_;
  /* Bound the view within scroll region. */ 
  if (viewMin < worldMin)
    viewMin = worldMin;

  if (viewMax > worldMax)
    viewMax = worldMax;

  if (ops->logScale) {
    worldMin = log10(worldMin);
    worldMax = log10(worldMax);
    viewMin  = log10(viewMin);
    viewMax  = log10(viewMax);
  }
  double worldWidth = worldMax - worldMin;
  double viewWidth  = viewMax - viewMin;

  /* Unlike horizontal axes, vertical axis values run opposite of the
   * scrollbar first/last values.  So instead of pushing the axis minimum
   * around, we move the maximum instead. */
  double axisOffset;
  double axisScale;
  if (axisPtr->isHorizontal() != ops->descending) {
    axisOffset  = viewMin - worldMin;
    axisScale = graphPtr->hScale;
  } else {
    axisOffset  = worldMax - viewMax;
    axisScale = graphPtr->vScale;
  }
  if (objc == 4) {
    double first = Clamp(axisOffset / worldWidth);
    double last = Clamp((axisOffset + viewWidth) / worldWidth);
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(first));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(last));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  double fract = axisOffset / worldWidth;
  if (GetAxisScrollInfo(interp, objc, objv, &fract, viewWidth / worldWidth, 
			ops->scrollUnits, axisScale) != TCL_OK)
    return TCL_ERROR;

  if (axisPtr->isHorizontal() != ops->descending) {
    ops->reqMin = (fract * worldWidth) + worldMin;
    ops->reqMax = ops->reqMin + viewWidth;
  }
  else {
    ops->reqMax = worldMax - (fract * worldWidth);
    ops->reqMin = ops->reqMax - viewWidth;
  }
  if (ops->logScale) {
    ops->reqMin = EXP10(ops->reqMin);
    ops->reqMax = EXP10(ops->reqMax);
  }
  graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static Blt_OpSpec defAxisOps[] = {
  {"activate",     1, (void*)ActivateOp,     3, 3, "",},
  {"bind",         1, (void*)BindOp,         2, 5, "sequence command",},
  {"cget",         2, (void*)CgetOp,         4, 4, "option",},
  {"configure",    2, (void*)ConfigureOp,    3, 0, "?option value?...",},
  {"deactivate",   1, (void*)ActivateOp,     3, 3, "",},
  {"invtransform", 1, (void*)InvTransformOp, 4, 4, "value",},
  {"limits",       1, (void*)LimitsOp,       3, 3, "",},
  {"transform",    1, (void*)TransformOp,    4, 4, "value",},
  {"use",          1, (void*)UseOp,          3, 4, "?axisName?",},
  {"view",         1, (void*)ViewOp,         3, 6, "?moveto fract? ",},
};

static int nDefAxisOps = sizeof(defAxisOps) / sizeof(Blt_OpSpec);

int Blt_DefAxisOp(Tcl_Interp* interp, Graph* graphPtr, int margin, 
		  int objc, Tcl_Obj* const objv[])
{
  GraphDefAxisProc* proc = (GraphDefAxisProc*)Blt_GetOpFromObj(interp, nDefAxisOps, defAxisOps, BLT_OP_ARG2, objc, objv, 0);
  if (!proc)
    return TCL_ERROR;

  if (proc == UseOp) {
    // Set global variable to the margin in the argument list
    lastMargin = margin;
    return (*proc)(interp, (Axis*)graphPtr, objc, objv);
  } 
  else {
    Axis* axisPtr = Blt_GetFirstAxis(graphPtr->margins[margin].axes);
    if (!axisPtr)
      return TCL_OK;
    return (*proc)(interp, axisPtr, objc, objv);
  }
}

// Axis

int CreateAxis(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  char *string = Tcl_GetString(objv[3]);
  if (string[0] == '-') {
    Tcl_AppendResult(graphPtr->interp, "name of axis \"", string, 
		     "\" can't start with a '-'", NULL);
    return TCL_ERROR;
  }

  int isNew;
  Tcl_HashEntry* hPtr = 
    Tcl_CreateHashEntry(&graphPtr->axes.table, string, &isNew);
  if (!isNew) {
    Tcl_AppendResult(graphPtr->interp, "axis \"", string,
		     "\" already exists in \"", Tcl_GetString(objv[0]),
		     "\"", NULL);
    return TCL_ERROR;
  }

  Axis* axisPtr = new Axis(graphPtr, Tcl_GetString(objv[3]), MARGIN_NONE);
  if (!axisPtr)
    return TCL_ERROR;
  axisPtr->hashPtr_ = hPtr;
  Tcl_SetHashValue(hPtr, axisPtr);

  if ((Tk_InitOptions(graphPtr->interp, (char*)axisPtr->ops(), axisPtr->optionTable(), graphPtr->tkwin) != TCL_OK) || (AxisObjConfigure(interp, axisPtr, objc-4, objv+4) != TCL_OK)) {
    delete axisPtr;
    return TCL_ERROR;
  }

  return TCL_OK;
}

static int AxisCgetOp(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return CgetOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisConfigureOp(Tcl_Interp* interp, Graph* graphPtr, int objc, 
			   Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return ConfigureOp(interp, axisPtr, objc-1, objv+1);
}

int AxisObjConfigure(Tcl_Interp* interp, Axis* axisPtr,
			    int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->graphPtr_;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)axisPtr->ops(), axisPtr->optionTable(), 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    graphPtr->flags |= mask;
    graphPtr->flags |= CACHE_DIRTY;
    if (ConfigureAxis(axisPtr) != TCL_OK)
      return TCL_ERROR;
    Blt_EventuallyRedrawGraph(graphPtr);

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


static int AxisActivateOp(Tcl_Interp* interp, Graph* graphPtr, 
			  int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return ActivateOp(interp, axisPtr, objc, objv);
}

static int AxisBindOp(Tcl_Interp* interp, Graph* graphPtr, int objc, 
		      Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_HashSearch cursor;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.tagTable, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
      const char *tagName = (const char*)
	Tcl_GetHashKey(&graphPtr->axes.tagTable, hPtr);
      Tcl_Obj *objPtr = Tcl_NewStringObj(tagName, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  else
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, Blt_MakeAxisTag(graphPtr, Tcl_GetString(objv[3])), objc-4, objv+4);
}

static int AxisCreateOp(Tcl_Interp* interp, Graph* graphPtr, 
			int objc, Tcl_Obj* const objv[])
{
  if (CreateAxis(interp, graphPtr, objc, objv) != TCL_OK)
    return TCL_ERROR;
  Tcl_SetObjResult(interp, objv[3]);

  return TCL_OK;
}

static int AxisDeleteOp(Tcl_Interp* interp, Graph* graphPtr, 
			int objc, Tcl_Obj* const objv[])
{
  if (objc<4)
    return TCL_ERROR;
    
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
    Tcl_AppendResult(interp, "can't find axis \"", 
		     Tcl_GetString(objv[3]), "\" in \"", 
		     Tk_PathName(graphPtr->tkwin), "\"", NULL);
    return TCL_ERROR;
  }

  axisPtr->flags |= DELETE_PENDING;
  if (axisPtr->refCount_ == 0) {
    Tcl_EventuallyFree(axisPtr, FreeAxis);
    Blt_EventuallyRedrawGraph(graphPtr);
  }

  return TCL_OK;
}

static int AxisFocusOp(Tcl_Interp* interp, Graph* graphPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  graphPtr->focusPtr = NULL;
  if (objc == 4) {
    Axis *axisPtr;
    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
      return TCL_ERROR;

    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    if (axisPtr && !ops->hide && axisPtr->use_)
      graphPtr->focusPtr = axisPtr;
  }

  Blt_SetFocusItem(graphPtr->bindTable, graphPtr->focusPtr, NULL);

  if (graphPtr->focusPtr)
    Tcl_SetStringObj(Tcl_GetObjResult(interp), graphPtr->focusPtr->name(),-1);

  return TCL_OK;
}

static int AxisGetOp(Tcl_Interp* interp, Graph* graphPtr, 
		     int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr = (Axis*)Blt_GetCurrentItem(graphPtr->bindTable);

  // Report only on axes
  if ((axisPtr) && 
      ((axisPtr->classId() == CID_AXIS_X) || 
       (axisPtr->classId() == CID_AXIS_Y) || 
       (axisPtr->classId() == CID_NONE))) {

    char  *string = Tcl_GetString(objv[3]);
    if (!strcmp(string, "current"))
      Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->name(),-1);
    else if (!strcmp(string, "detail"))
      Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->detail_, -1);
  }

  return TCL_OK;
}

static int AxisInvTransformOp(Tcl_Interp* interp, Graph* graphPtr, 
			      int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return InvTransformOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisLimitsOp(Tcl_Interp* interp, Graph* graphPtr, 
			int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return LimitsOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisMarginOp(Tcl_Interp* interp, Graph* graphPtr, 
			int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return MarginOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisNamesOp(Tcl_Interp* interp, Graph* graphPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    Tcl_HashSearch cursor;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
      Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
      if (axisPtr->flags & DELETE_PENDING)
	continue;

      Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(axisPtr->name(), -1));
    }
  } 
  else {
    Tcl_HashSearch cursor;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
      Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
      for (int ii=3; ii<objc; ii++) {
	const char *pattern = (const char*)Tcl_GetString(objv[ii]);
	if (Tcl_StringMatch(axisPtr->name(), pattern)) {
	  Tcl_ListObjAppendElement(interp, listObjPtr, 
				   Tcl_NewStringObj(axisPtr->name(), -1));
	  break;
	}
      }
    }
  }
  Tcl_SetObjResult(interp, listObjPtr);

  return TCL_OK;
}

static int AxisTransformOp(Tcl_Interp* interp, Graph* graphPtr, 
			   int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return TransformOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisTypeOp(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return TypeOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisViewOp(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return ViewOp(interp, axisPtr, objc-1, objv+1);
}

static Blt_OpSpec axisOps[] = {
  {"activate",     1, (void*)AxisActivateOp,     4, 4, "axisName"},
  {"bind",         1, (void*)AxisBindOp,         3, 6, "axisName sequence command"},
  {"cget",         2, (void*)AxisCgetOp,         5, 5, "axisName option"},
  {"configure",    2, (void*)AxisConfigureOp,    4, 0, "axisName ?axisName?... "
   "?option value?..."},
  {"create",       2, (void*)AxisCreateOp,       4, 0, "axisName ?option value?..."},
  {"deactivate",   3, (void*)AxisActivateOp,     4, 4, "axisName"},
  {"delete",       3, (void*)AxisDeleteOp,       3, 0, "?axisName?..."},
  {"focus",        1, (void*)AxisFocusOp,        3, 4, "?axisName?"},
  {"get",          1, (void*)AxisGetOp,          4, 4, "name"},
  {"invtransform", 1, (void*)AxisInvTransformOp, 5, 5, "axisName value"},
  {"limits",       1, (void*)AxisLimitsOp,       4, 4, "axisName"},
  {"margin",       1, (void*)AxisMarginOp,       4, 4, "axisName"},
  {"names",        1, (void*)AxisNamesOp,        3, 0, "?pattern?..."},
  {"transform",    2, (void*)AxisTransformOp,    5, 5, "axisName value"},
  {"type",         2, (void*)AxisTypeOp,       4, 4, "axisName"},
  {"view",         1, (void*)AxisViewOp,         4, 7, "axisName ?moveto fract? "
   "?scroll number what?"},
};
static int nAxisOps = sizeof(axisOps) / sizeof(Blt_OpSpec);

int Blt_AxisOp(Graph* graphPtr, Tcl_Interp* interp, 
	       int objc, Tcl_Obj* const objv[])
{
  GraphAxisProc* proc = (GraphAxisProc*)Blt_GetOpFromObj(interp, nAxisOps, axisOps, BLT_OP_ARG2, objc, objv, 0);
  if (!proc)
    return TCL_ERROR;

  return (*proc)(interp, graphPtr, objc, objv);
}

// Support

int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr)
{
  Tcl_HashEntry *hPtr;
  const char *name;

  *axisPtrPtr = NULL;
  name = Tcl_GetString(objPtr);
  hPtr = Tcl_FindHashEntry(&graphPtr->axes.table, name);
  if (hPtr) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    if ((axisPtr->flags & DELETE_PENDING) == 0) {
      *axisPtrPtr = axisPtr;
      return TCL_OK;
    }
  }
  if (interp)
    Tcl_AppendResult(interp, "can't find axis \"", name, "\" in \"", 
		     Tk_PathName(graphPtr->tkwin), "\"", NULL);

  return TCL_ERROR;
}

void FreeAxis(char* data)
{
  Axis* axisPtr = (Axis*)data;
  delete axisPtr;
}

void Blt_ResetAxes(Graph* graphPtr)
{
  Blt_ChainLink link;
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;

  /* FIXME: This should be called whenever the display list of
   *	      elements change. Maybe yet another flag INIT_STACKS to
   *	      indicate that the element display list has changed.
   *	      Needs to be done before the axis limits are set.
   */
  Blt_InitBarSetTable(graphPtr);
  if ((graphPtr->barMode == BARS_STACKED) && (graphPtr->nBarGroups > 0))
    Blt_ComputeBarStacks(graphPtr);

  /*
   * Step 1:  Reset all axes. Initialize the data limits of the axis to
   *		impossible values.
   */
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->min_ = axisPtr->valueRange_.min = DBL_MAX;
    axisPtr->max_ = axisPtr->valueRange_.max = -DBL_MAX;
  }

  /*
   * Step 2:  For each element that's to be displayed, get the smallest
   *		and largest data values mapped to each X and Y-axis.  This
   *		will be the axis limits if the user doesn't override them 
   *		with -min and -max options.
   */
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Region2d exts;

    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemops = (ElementOptions*)elemPtr->ops();
    elemPtr->extents(&exts);
    elemops->axes.x->getDataLimits(exts.left, exts.right);
    elemops->axes.y->getDataLimits(exts.top, exts.bottom);
  }
  /*
   * Step 3:  Now that we know the range of data values for each axis,
   *		set axis limits and compute a sweep to generate tick values.
   */
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    double min, max;

    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    axisPtr->fixRange();

    /* Calculate min/max tick (major/minor) layouts */
    min = axisPtr->min_;
    max = axisPtr->max_;
    if ((!isnan(axisPtr->scrollMin_)) && (min < axisPtr->scrollMin_)) {
      min = axisPtr->scrollMin_;
    }
    if ((!isnan(axisPtr->scrollMax_)) && (max > axisPtr->scrollMax_)) {
      max = axisPtr->scrollMax_;
    }
    if (ops->logScale)
      axisPtr->logScale(min, max);
    else
      axisPtr->linearScale(min, max);

    if (axisPtr->use_ && (axisPtr->flags & DIRTY))
      graphPtr->flags |= CACHE_DIRTY;
  }

  graphPtr->flags &= ~RESET_AXES;

  /*
   * When any axis changes, we need to layout the entire graph.
   */
  graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | MAP_ALL | 
		      REDRAW_WORLD);
}

Point2d Blt_Map2D(Graph* graphPtr, double x, double y, Axis2d *axesPtr)
{
  Point2d point;

  if (graphPtr->inverted) {
    point.x = axesPtr->y->hMap(y);
    point.y = axesPtr->x->vMap(x);
  }
  else {
    point.x = axesPtr->x->hMap(x);
    point.y = axesPtr->y->vMap(y);
  }
  return point;
}

Point2d Blt_InvMap2D(Graph* graphPtr, double x, double y, Axis2d *axesPtr)
{
  Point2d point;

  if (graphPtr->inverted) {
    point.x = axesPtr->x->invVMap(y);
    point.y = axesPtr->y->invHMap(x);
  }
  else {
    point.x = axesPtr->x->invHMap(x);
    point.y = axesPtr->y->invVMap(y);
  }
  return point;
}

