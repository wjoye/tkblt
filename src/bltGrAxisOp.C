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
#include "bltGrLegd.h"

#define AXIS_PAD_TITLE 2
#define HORIZMARGIN(m)	(!((m)->site & 0x1)) /* Even sites are horizontal */

typedef int (GraphDefAxisProc)(Tcl_Interp* interp, Axis *axisPtr, 
			       int objc, Tcl_Obj* const objv[]);
typedef int (GraphAxisProc)(Tcl_Interp* interp, Graph* graphPtr, 
			    int objc, Tcl_Obj* const objv[]);

static void GetAxisGeometry(Graph* graphPtr, Axis *axisPtr);
static int GetMarginGeometry(Graph* graphPtr, Margin *marginPtr);
static int GetAxisScrollInfo(Tcl_Interp* interp, 
			     int objc, Tcl_Obj* const objv[],
			     double *offsetPtr, double windowSize,
			     double scrollUnits, double scale);
extern Tcl_FreeProc FreeAxis;
extern double AdjustViewport(double offset, double windowSize);
extern int ConfigureAxis(Axis *axisPtr);
extern int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr);
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

void Blt_DestroyAxes(Graph* graphPtr)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->hashPtr_ = NULL;
    delete axisPtr;
  }
  Tcl_DeleteHashTable(&graphPtr->axes.table);

  for (int ii=0; ii<4; ii++)
    Blt_Chain_Destroy(graphPtr->axisChain[ii]);

  Tcl_DeleteHashTable(&graphPtr->axes.tagTable);
  Blt_Chain_Destroy(graphPtr->axes.displayList);
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

void Blt_MapAxes(Graph* graphPtr)
{
  for (int margin = 0; margin < 4; margin++) {
    int count =0;
    int offset =0;
    Blt_Chain chain = graphPtr->margins[margin].axes;
    for (Blt_ChainLink link=Blt_Chain_FirstLink(chain); link; 
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!axisPtr->use_ || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (graphPtr->stackAxes) {
	if (ops->reqNumMajorTicks <= 0)
	  ops->reqNumMajorTicks = 4;

	axisPtr->mapStacked(count, margin);
      } 
      else {
	if (ops->reqNumMajorTicks <= 0)
	  ops->reqNumMajorTicks = 4;

	axisPtr->map(offset, margin);
      }

      if (ops->showGrid)
	axisPtr->mapGridlines();

      offset += axisPtr->isHorizontal() ? axisPtr->height_ : axisPtr->width_;
      count++;
    }
  }
}

void Blt_DrawAxes(Graph* graphPtr, Drawable drawable)
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_LastLink(graphPtr->margins[i].axes); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_ && !(axisPtr->flags & DELETE_PENDING))
	axisPtr->draw(drawable);
    }
  }
}

void Blt_DrawAxisLimits(Graph* graphPtr, Drawable drawable)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
  char minString[200], maxString[200];
  int vMin, hMin, vMax, hMax;

#define SPACING 8
  vMin = vMax = graphPtr->left + graphPtr->xPad + 2;
  hMin = hMax = graphPtr->bottom - graphPtr->yPad - 2;	/* Offsets */

  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Dim2D textDim;
    const char *minFmt, *maxFmt;
    char *minPtr, *maxPtr;
    int isHoriz;

    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    ops->limitsTextStyle.flags |= UPDATE_GC;

    if (axisPtr->flags & DELETE_PENDING)
      continue;

    if (!ops->limitsFormat)
      continue;

    isHoriz = axisPtr->isHorizontal();
    minPtr = maxPtr = NULL;
    minFmt = maxFmt = ops->limitsFormat;
    if (minFmt[0] != '\0') {
      minPtr = minString;
      sprintf_s(minString, 200, minFmt, axisPtr->axisRange_.min);
    }
    if (maxFmt[0] != '\0') {
      maxPtr = maxString;
      sprintf_s(maxString, 200, maxFmt, axisPtr->axisRange_.max);
    }
    if (ops->descending) {
      char *tmp;

      tmp = minPtr, minPtr = maxPtr, maxPtr = tmp;
    }
    if (maxPtr) {
      if (isHoriz) {
	ops->limitsTextStyle.angle = 90.0;
	ops->limitsTextStyle.anchor = TK_ANCHOR_SE;

	Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		      &ops->limitsTextStyle, graphPtr->right, 
		      hMax, &textDim);
	hMax -= (textDim.height + SPACING);
      } 
      else {
	ops->limitsTextStyle.angle = 0.0;
	ops->limitsTextStyle.anchor = TK_ANCHOR_NW;

	Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		      &ops->limitsTextStyle, vMax, 
		      graphPtr->top, &textDim);
	vMax += (textDim.width + SPACING);
      }
    }
    if (minPtr) {
      ops->limitsTextStyle.anchor = TK_ANCHOR_SW;

      if (isHoriz) {
	ops->limitsTextStyle.angle = 90.0;

	Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		      &ops->limitsTextStyle, graphPtr->left, 
		      hMin, &textDim);
	hMin -= (textDim.height + SPACING);
      } 
      else {
	ops->limitsTextStyle.angle = 0.0;

	Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		      &ops->limitsTextStyle, vMin, 
		      graphPtr->bottom, &textDim);
	vMin += (textDim.width + SPACING);
      }
    }
  }
}

void Blt_AxisLimitsToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
  double vMin, hMin, vMax, hMax;
  char string[200];

#define SPACING 8
  vMin = vMax = graphPtr->left + graphPtr->xPad + 2;
  hMin = hMax = graphPtr->bottom - graphPtr->yPad - 2;	/* Offsets */
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    const char *minFmt, *maxFmt;
    unsigned int textWidth, textHeight;

    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();

    if (axisPtr->flags & DELETE_PENDING)
      continue;

    if (!ops->limitsFormat)
      continue;

    minFmt = maxFmt = ops->limitsFormat;
    if (*maxFmt != '\0') {
      sprintf_s(string, 200, maxFmt, axisPtr->axisRange_.max);
      Blt_GetTextExtents(ops->tickFont, 0, string, -1, &textWidth,
			 &textHeight);
      if ((textWidth > 0) && (textHeight > 0)) {
	if (axisPtr->classId() == CID_AXIS_X) {
	  ops->limitsTextStyle.angle = 90.0;
	  ops->limitsTextStyle.anchor = TK_ANCHOR_SE;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle, 
			  (double)graphPtr->right, hMax);
	  hMax -= (textWidth + SPACING);
	} 
	else {
	  ops->limitsTextStyle.angle = 0.0;
	  ops->limitsTextStyle.anchor = TK_ANCHOR_NW;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle,
			  vMax, (double)graphPtr->top);
	  vMax += (textWidth + SPACING);
	}
      }
    }
    if (*minFmt != '\0') {
      sprintf_s(string, 200, minFmt, axisPtr->axisRange_.min);
      Blt_GetTextExtents(ops->tickFont, 0, string, -1, &textWidth,
			 &textHeight);
      if ((textWidth > 0) && (textHeight > 0)) {
	ops->limitsTextStyle.anchor = TK_ANCHOR_SW;
	if (axisPtr->classId() == CID_AXIS_X) {
	  ops->limitsTextStyle.angle = 90.0;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle, 
			  (double)graphPtr->left, hMin);
	  hMin -= (textWidth + SPACING);
	}
	else {
	  ops->limitsTextStyle.angle = 0.0;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle, 
			  vMin, (double)graphPtr->bottom);
	  vMin += (textWidth + SPACING);
	}
      }
    }
  }
}

int GetAxisScrollInfo(Tcl_Interp* interp, 
			     int objc, Tcl_Obj* const objv[],
			     double *offsetPtr, double windowSize,
			     double scrollUnits, double scale)
{
  const char *string;
  char c;
  double offset;
  int length;

  offset = *offsetPtr;
  string = Tcl_GetStringFromObj(objv[0], &length);
  c = string[0];
  scrollUnits *= scale;
  if ((c == 's') && (strncmp(string, "scroll", length) == 0)) {
    int count;
    double fract;

    /* Scroll number unit/page */
    if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK)
      return TCL_ERROR;

    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    if ((c == 'u') && (strncmp(string, "units", length) == 0))
      fract = count * scrollUnits;
    else if ((c == 'p') && (strncmp(string, "pages", length) == 0))
      /* A page is 90% of the view-able window. */
      fract = (int)(count * windowSize * 0.9 + 0.5);
    else if ((c == 'p') && (strncmp(string, "pixels", length) == 0))
      fract = count * scale;
    else {
      Tcl_AppendResult(interp, "unknown \"scroll\" units \"", string,
		       "\"", NULL);
      return TCL_ERROR;
    }
    offset += fract;
  } 
  else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
    double fract;

    /* moveto fraction */
    if (Tcl_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
      return TCL_ERROR;
    }
    offset = fract;
  } 
  else {
    int count;
    double fract;

    /* Treat like "scroll units" */
    if (Tcl_GetIntFromObj(interp, objv[0], &count) != TCL_OK) {
      return TCL_ERROR;
    }
    fract = (double)count * scrollUnits;
    offset += fract;
    /* CHECK THIS: return TCL_OK; */
  }
  *offsetPtr = AdjustViewport(offset, windowSize);
  return TCL_OK;
}

void Blt_ConfigureAxes(Graph* graphPtr)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    ConfigureAxis(axisPtr);
  }
}

void Blt_DrawGrids(Graph* graphPtr, Drawable drawable) 
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->margins[i].axes); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (ops->hide || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (axisPtr->use_ && ops->showGrid) {
	Blt_Draw2DSegments(graphPtr->display, drawable, 
			   ops->major.gc, ops->major.segments, 
			   ops->major.nUsed);

	if (ops->showGridMinor)
	  Blt_Draw2DSegments(graphPtr->display, drawable, 
			     ops->minor.gc, ops->minor.segments, 
			     ops->minor.nUsed);
      }
    }
  }
}

void Blt_AxesToPostScript(Graph* graphPtr, Blt_Ps ps) 
{
  Margin *mp, *mend;

  for (mp = graphPtr->margins, mend = mp + 4; mp < mend; mp++) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(mp->axes); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_ && !(axisPtr->flags & DELETE_PENDING))
	axisPtr->print(ps);
    }
  }
}

void Blt_GridsToPostScript(Graph* graphPtr, Blt_Ps ps) 
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->margins[i].axes); link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (ops->hide || !ops->showGrid || !axisPtr->use_ || 
	  (axisPtr->flags & DELETE_PENDING))
	continue;

      Blt_Ps_Format(ps, "%% Axis %s: grid line attributes\n",
		    axisPtr->name());
      Blt_Ps_XSetLineAttributes(ps, ops->major.color, 
				ops->major.lineWidth, 
				&ops->major.dashes, CapButt, 
				JoinMiter);
      Blt_Ps_Format(ps, "%% Axis %s: major grid line segments\n",
		    axisPtr->name());
      Blt_Ps_Draw2DSegments(ps, ops->major.segments, 
			    ops->major.nUsed);
      if (ops->showGridMinor) {
	Blt_Ps_XSetLineAttributes(ps, ops->minor.color, 
				  ops->minor.lineWidth, 
				  &ops->minor.dashes, CapButt, 
				  JoinMiter);
	Blt_Ps_Format(ps, "%% Axis %s: minor grid line segments\n",
		      axisPtr->name());
	Blt_Ps_Draw2DSegments(ps, ops->minor.segments, 
			      ops->minor.nUsed);
      }
    }
  }
}

Axis *Blt_GetFirstAxis(Blt_Chain chain)
{
  Blt_ChainLink link = Blt_Chain_FirstLink(chain);
  if (!link)
    return NULL;

  return (Axis*)Blt_Chain_GetValue(link);
}

Axis *Blt_NearestAxis(Graph* graphPtr, int x, int y)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
    
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); 
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    if (ops->hide || !axisPtr->use_ || (axisPtr->flags & DELETE_PENDING))
      continue;

    if (ops->showTicks) {
      Blt_ChainLink link;

      for (link = Blt_Chain_FirstLink(axisPtr->tickLabels_); link != NULL; 
	   link = Blt_Chain_NextLink(link)) {	
	Point2d t;
	double rw, rh;
	Point2d bbox[5];

	TickLabel *labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
	Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			   ops->tickAngle, &rw, &rh, bbox);
	t = Blt_AnchorPoint(labelPtr->anchorPos.x, 
			    labelPtr->anchorPos.y, rw, rh, 
			    axisPtr->tickAnchor_);
	t.x = x - t.x - (rw * 0.5);
	t.y = y - t.y - (rh * 0.5);

	bbox[4] = bbox[0];
	if (Blt_PointInPolygon(&t, bbox, 5)) {
	  axisPtr->detail_ = "label";
	  return axisPtr;
	}
      }
    }
    if (ops->title) {	/* and then the title string. */
      Point2d bbox[5];
      Point2d t;
      double rw, rh;
      unsigned int w, h;

      Blt_GetTextExtents(ops->titleFont, 0, ops->title,-1,&w,&h);
      Blt_GetBoundingBox(w, h, axisPtr->titleAngle_, &rw, &rh, bbox);
      t = Blt_AnchorPoint(axisPtr->titlePos_.x, axisPtr->titlePos_.y, 
			  rw, rh, axisPtr->titleAnchor_);
      /* Translate the point so that the 0,0 is the upper left 
       * corner of the bounding box.  */
      t.x = x - t.x - (rw * 0.5);
      t.y = y - t.y - (rh * 0.5);
	    
      bbox[4] = bbox[0];
      if (Blt_PointInPolygon(&t, bbox, 5)) {
	axisPtr->detail_ = "title";
	return axisPtr;
      }
    }
    if (ops->lineWidth > 0) {	/* Check for the axis region */
      if ((x <= axisPtr->right_) && (x >= axisPtr->left_) && 
	  (y <= axisPtr->bottom_) && (y >= axisPtr->top_)) {
	axisPtr->detail_ = "line";
	return axisPtr;
      }
    }
  }
  return NULL;
}
 
ClientData Blt_MakeAxisTag(Graph* graphPtr, const char *tagName)
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&graphPtr->axes.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->axes.tagTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LayoutGraph --
 *
 *	Calculate the layout of the graph.  Based upon the data, axis limits,
 *	X and Y titles, and title height, determine the cavity left which is
 *	the plotting surface.  The first step get the data and axis limits for
 *	calculating the space needed for the top, bottom, left, and right
 *	margins.
 *
 * 	1) The LEFT margin is the area from the left border to the Y axis 
 *	   (not including ticks). It composes the border width, the width an 
 *	   optional Y axis label and its padding, and the tick numeric labels. 
 *	   The Y axis label is rotated 90 degrees so that the width is the 
 *	   font height.
 *
 * 	2) The RIGHT margin is the area from the end of the graph
 *	   to the right window border. It composes the border width,
 *	   some padding, the font height (this may be dubious. It
 *	   appears to provide a more even border), the max of the
 *	   legend width and 1/2 max X tick number. This last part is
 *	   so that the last tick label is not clipped.
 *
 *           Window Width
 *      ___________________________________________________________
 *      |          |                               |               |
 *      |          |   TOP  height of title        |               |
 *      |          |                               |               |
 *      |          |           x2 title            |               |
 *      |          |                               |               |
 *      |          |        height of x2-axis      |               |
 *      |__________|_______________________________|_______________|  W
 *      |          | -plotpady                     |               |  i
 *      |__________|_______________________________|_______________|  n
 *      |          | top                   right   |               |  d
 *      |          |                               |               |  o
 *      |   LEFT   |                               |     RIGHT     |  w
 *      |          |                               |               |
 *      | y        |     Free area = 104%          |      y2       |  H
 *      |          |     Plotting surface = 100%   |               |  e
 *      | t        |     Tick length = 2 + 2%      |      t        |  i
 *      | i        |                               |      i        |  g
 *      | t        |                               |      t  legend|  h
 *      | l        |                               |      l   width|  t
 *      | e        |                               |      e        |
 *      |    height|                               |height         |
 *      |       of |                               | of            |
 *      |    y-axis|                               |y2-axis        |
 *      |          |                               |               |
 *      |          |origin 0,0                     |               |
 *      |__________|_left_________________bottom___|_______________|
 *      |          |-plotpady                      |               |
 *      |__________|_______________________________|_______________|
 *      |          | (xoffset, yoffset)            |               |
 *      |          |                               |               |
 *      |          |       height of x-axis        |               |
 *      |          |                               |               |
 *      |          |   BOTTOM   x title            |               |
 *      |__________|_______________________________|_______________|
 *
 * 3) The TOP margin is the area from the top window border to the top
 *    of the graph. It composes the border width, twice the height of
 *    the title font (if one is given) and some padding between the
 *    title.
 *
 * 4) The BOTTOM margin is area from the bottom window border to the
 *    X axis (not including ticks). It composes the border width, the height
 *    an optional X axis label and its padding, the height of the font
 *    of the tick labels.
 *
 * The plotting area is between the margins which includes the X and Y axes
 * including the ticks but not the tick numeric labels. The length of the
 * ticks and its padding is 5% of the entire plotting area.  Hence the entire
 * plotting area is scaled as 105% of the width and height of the area.
 *
 * The axis labels, ticks labels, title, and legend may or may not be
 * displayed which must be taken into account.
 *
 * if reqWidth > 0 : set outer size
 * if reqPlotWidth > 0 : set plot size
 *---------------------------------------------------------------------------
 */

void Blt_LayoutGraph(Graph* graphPtr)
{
  int titleY;
  int plotWidth, plotHeight;
  int inset, inset2;

  int width = graphPtr->width;
  int height = graphPtr->height;

  /* 
   * Step 1:  Compute the amount of space needed to display the axes
   *		associated with each margin.  They can be overridden by 
   *		-leftmargin, -rightmargin, -bottommargin, and -topmargin
   *		graph options, respectively.
   */
  int left   = GetMarginGeometry(graphPtr, &graphPtr->leftMargin);
  int right  = GetMarginGeometry(graphPtr, &graphPtr->rightMargin);
  int top    = GetMarginGeometry(graphPtr, &graphPtr->topMargin);
  int bottom = GetMarginGeometry(graphPtr, &graphPtr->bottomMargin);

  int pad = graphPtr->bottomMargin.maxTickWidth;
  if (pad < graphPtr->topMargin.maxTickWidth)
    pad = graphPtr->topMargin.maxTickWidth;

  pad = pad / 2 + 3;
  if (right < pad)
    right = pad;

  if (left < pad)
    left = pad;

  pad = graphPtr->leftMargin.maxTickHeight;
  if (pad < graphPtr->rightMargin.maxTickHeight)
    pad = graphPtr->rightMargin.maxTickHeight;

  pad = pad / 2;
  if (top < pad)
    top = pad;

  if (bottom < pad)
    bottom = pad;

  if (graphPtr->leftMargin.reqSize > 0)
    left = graphPtr->leftMargin.reqSize;

  if (graphPtr->rightMargin.reqSize > 0)
    right = graphPtr->rightMargin.reqSize;

  if (graphPtr->topMargin.reqSize > 0)
    top = graphPtr->topMargin.reqSize;

  if (graphPtr->bottomMargin.reqSize > 0)
    bottom = graphPtr->bottomMargin.reqSize;

  /* 
   * Step 2:  Add the graph title height to the top margin. 
   */
  if (graphPtr->title)
    top += graphPtr->titleHeight + 6;

  inset = (graphPtr->inset + graphPtr->plotBW);
  inset2 = 2 * inset;

  /* 
   * Step 3: Estimate the size of the plot area from the remaining
   *	       space.  This may be overridden by the -plotwidth and
   *	       -plotheight graph options.  We use this to compute the
   *	       size of the legend. 
   */
  if (width == 0)
    width = 400;

  if (height == 0)
    height = 400;

  plotWidth  = (graphPtr->reqPlotWidth > 0) ? graphPtr->reqPlotWidth :
    width - (inset2 + left + right); /* Plot width. */
  plotHeight = (graphPtr->reqPlotHeight > 0) ? graphPtr->reqPlotHeight : 
    height - (inset2 + top + bottom); /* Plot height. */
  graphPtr->legend->map(plotWidth, plotHeight);

  /* 
   * Step 2:  Add the legend to the appropiate margin. 
   */
  if (!graphPtr->legend->isHidden()) {
    switch (graphPtr->legend->position()) {
    case Legend::RIGHT:
      right += graphPtr->legend->width() + 2;
      break;
    case Legend::LEFT:
      left += graphPtr->legend->width() + 2;
      break;
    case Legend::TOP:
      top += graphPtr->legend->height() + 2;
      break;
    case Legend::BOTTOM:
      bottom += graphPtr->legend->height() + 2;
      break;
    case Legend::XY:
    case Legend::PLOT:
      /* Do nothing. */
      break;
    }
  }

  /* 
   * Recompute the plotarea or graph size, now accounting for the legend. 
   */
  if (graphPtr->reqPlotWidth == 0) {
    plotWidth = width  - (inset2 + left + right);
    if (plotWidth < 1)
      plotWidth = 1;
  }
  if (graphPtr->reqPlotHeight == 0) {
    plotHeight = height - (inset2 + top + bottom);
    if (plotHeight < 1)
      plotHeight = 1;
  }

  /*
   * Step 5: If necessary, correct for the requested plot area aspect
   *	       ratio.
   */
  if ((graphPtr->reqPlotWidth == 0) && (graphPtr->reqPlotHeight == 0) && 
      (graphPtr->aspect > 0.0f)) {
    float ratio;

    /* 
     * Shrink one dimension of the plotarea to fit the requested
     * width/height aspect ratio.
     */
    ratio = (float)plotWidth / (float)plotHeight;
    if (ratio > graphPtr->aspect) {
      // Shrink the width
      int scaledWidth = (int)(plotHeight * graphPtr->aspect);
      if (scaledWidth < 1)
	scaledWidth = 1;

      // Add the difference to the right margin.
      // CHECK THIS: w = scaledWidth
      right += (plotWidth - scaledWidth);
    }
    else {
      // Shrink the height
      int scaledHeight = (int)(plotWidth / graphPtr->aspect);
      if (scaledHeight < 1)
	scaledHeight = 1;

      // Add the difference to the top margin
      // CHECK THIS: h = scaledHeight;
      top += (plotHeight - scaledHeight); 
    }
  }

  /* 
   * Step 6: If there's multiple axes in a margin, the axis titles will be
   *	       displayed in the adjoining margins.  Make sure there's room 
   *	       for the longest axis titles.
   */

  if (top < graphPtr->leftMargin.axesTitleLength)
    top = graphPtr->leftMargin.axesTitleLength;

  if (right < graphPtr->bottomMargin.axesTitleLength)
    right = graphPtr->bottomMargin.axesTitleLength;

  if (top < graphPtr->rightMargin.axesTitleLength)
    top = graphPtr->rightMargin.axesTitleLength;

  if (right < graphPtr->topMargin.axesTitleLength)
    right = graphPtr->topMargin.axesTitleLength;

  /* 
   * Step 7: Override calculated values with requested margin sizes.
   */
  if (graphPtr->leftMargin.reqSize > 0)
    left = graphPtr->leftMargin.reqSize;

  if (graphPtr->rightMargin.reqSize > 0)
    right = graphPtr->rightMargin.reqSize;

  if (graphPtr->topMargin.reqSize > 0)
    top = graphPtr->topMargin.reqSize;

  if (graphPtr->bottomMargin.reqSize > 0)
    bottom = graphPtr->bottomMargin.reqSize;

  if (graphPtr->reqPlotWidth > 0) {	
    /* 
     * Width of plotarea is constained.  If there's extra space, add it to
     * th left and/or right margins.  If there's too little, grow the
     * graph width to accomodate it.
     */
    int w = plotWidth + inset2 + left + right;
    if (width > w) {		/* Extra space in window. */
      int extra = (width - w) / 2;
      if (graphPtr->leftMargin.reqSize == 0) { 
	left += extra;
	if (graphPtr->rightMargin.reqSize == 0) { 
	  right += extra;
	}
	else {
	  left += extra;
	}
      }
      else if (graphPtr->rightMargin.reqSize == 0) {
	right += extra + extra;
      }
    }
    else if (width < w) {
      width = w;
    }
  } 
  if (graphPtr->reqPlotHeight > 0) {	/* Constrain the plotarea height. */
    /* 
     * Height of plotarea is constained.  If there's extra space, 
     * add it to th top and/or bottom margins.  If there's too little,
     * grow the graph height to accomodate it.
     */
    int h = plotHeight + inset2 + top + bottom;
    if (height > h) {		/* Extra space in window. */
      int extra;

      extra = (height - h) / 2;
      if (graphPtr->topMargin.reqSize == 0) { 
	top += extra;
	if (graphPtr->bottomMargin.reqSize == 0) { 
	  bottom += extra;
	}
	else {
	  top += extra;
	}
      }
      else if (graphPtr->bottomMargin.reqSize == 0) {
	bottom += extra + extra;
      }
    }
    else if (height < h) {
      height = h;
    }
  }	
  graphPtr->width  = width;
  graphPtr->height = height;
  graphPtr->left   = left + inset;
  graphPtr->top    = top + inset;
  graphPtr->right  = width - right - inset;
  graphPtr->bottom = height - bottom - inset;

  graphPtr->leftMargin.width    = left   + graphPtr->inset;
  graphPtr->rightMargin.width   = right  + graphPtr->inset;
  graphPtr->topMargin.height    = top    + graphPtr->inset;
  graphPtr->bottomMargin.height = bottom + graphPtr->inset;
	    
  graphPtr->vOffset = graphPtr->top + graphPtr->yPad;
  graphPtr->vRange  = plotHeight - 2*graphPtr->yPad;
  graphPtr->hOffset = graphPtr->left + graphPtr->xPad;
  graphPtr->hRange  = plotWidth  - 2*graphPtr->xPad;

  if (graphPtr->vRange < 1)
    graphPtr->vRange = 1;

  if (graphPtr->hRange < 1)
    graphPtr->hRange = 1;

  graphPtr->hScale = 1.0f / (float)graphPtr->hRange;
  graphPtr->vScale = 1.0f / (float)graphPtr->vRange;

  // Calculate the placement of the graph title so it is centered within the
  // space provided for it in the top margin
  titleY = graphPtr->titleHeight;
  graphPtr->titleY = 3 + graphPtr->inset;
  graphPtr->titleX = (graphPtr->right + graphPtr->left) / 2;
}

static int GetMarginGeometry(Graph* graphPtr, Margin *marginPtr)
{
  int isHoriz = HORIZMARGIN(marginPtr);

  // Count the visible axes.
  unsigned int nVisible = 0;
  unsigned int l =0;
  int w =0;
  int h =0;

  marginPtr->maxTickWidth =0;
  marginPtr->maxTickHeight =0;

  if (graphPtr->stackAxes) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(marginPtr->axes);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_) {
	nVisible++;
	if (graphPtr->flags & GET_AXIS_GEOMETRY)
	  GetAxisGeometry(graphPtr, axisPtr);

	if (isHoriz) {
	  if (h < axisPtr->height_)
	    h = axisPtr->height_;
	}
	else {
	  if (w < axisPtr->width_)
	    w = axisPtr->width_;
	}
	if (axisPtr->maxTickWidth_ > marginPtr->maxTickWidth)
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth_;

	if (axisPtr->maxTickHeight_ > marginPtr->maxTickHeight)
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight_;
      }
    }
  }
  else {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(marginPtr->axes);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_) {
	nVisible++;
	if (graphPtr->flags & GET_AXIS_GEOMETRY)
	  GetAxisGeometry(graphPtr, axisPtr);

	if ((ops->titleAlternate) && (l < axisPtr->titleWidth_))
	  l = axisPtr->titleWidth_;

	if (isHoriz)
	  h += axisPtr->height_;
	else
	  w += axisPtr->width_;

	if (axisPtr->maxTickWidth_ > marginPtr->maxTickWidth)
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth_;

	if (axisPtr->maxTickHeight_ > marginPtr->maxTickHeight)
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight_;
      }
    }
  }
  // Enforce a minimum size for margins.
  if (w < 3)
    w = 3;

  if (h < 3)
    h = 3;

  marginPtr->nAxes = nVisible;
  marginPtr->axesTitleLength = l;
  marginPtr->width = w;
  marginPtr->height = h;
  marginPtr->axesOffset = (isHoriz) ? h : w;
  return marginPtr->axesOffset;
}

static void GetAxisGeometry(Graph* graphPtr, Axis *axisPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  axisPtr->freeTickLabels();

  // Leave room for axis baseline and padding
  unsigned int y =0;
  if (ops->exterior && (graphPtr->plotRelief != TK_RELIEF_SOLID))
    y += ops->lineWidth + 2;

  axisPtr->maxTickHeight_ = axisPtr->maxTickWidth_ = 0;

  if (axisPtr->t1Ptr_)
    free(axisPtr->t1Ptr_);
  axisPtr->t1Ptr_ = axisPtr->generateTicks(&axisPtr->majorSweep_);
  if (axisPtr->t2Ptr_)
    free(axisPtr->t2Ptr_);
  axisPtr->t2Ptr_ = axisPtr->generateTicks(&axisPtr->minorSweep_);

  if (ops->showTicks) {
    Ticks* t1Ptr = ops->t1UPtr ? ops->t1UPtr : axisPtr->t1Ptr_;
	
    int nTicks =0;
    if (t1Ptr)
      nTicks = t1Ptr->nTicks;
	
    unsigned int nLabels =0;
    for (int ii=0; ii<nTicks; ii++) {
      double x = t1Ptr->values[ii];
      double x2 = t1Ptr->values[ii];
      if (ops->labelOffset)
	x2 += axisPtr->majorSweep_.step * 0.5;

      if (!axisPtr->inRange(x2, &axisPtr->axisRange_))
	continue;

      TickLabel* labelPtr = axisPtr->makeLabel(x);
      Blt_Chain_Append(axisPtr->tickLabels_, labelPtr);
      nLabels++;
      /* 
       * Get the dimensions of each tick label.  Remember tick labels
       * can be multi-lined and/or rotated.
       */
      unsigned int lw, lh;	/* Label width and height. */
      Blt_GetTextExtents(ops->tickFont, 0, labelPtr->string, -1, &lw, &lh);
      labelPtr->width  = lw;
      labelPtr->height = lh;

      if (ops->tickAngle != 0.0f) {
	double rlw, rlh;	/* Rotated label width and height. */
	Blt_GetBoundingBox(lw, lh, ops->tickAngle, &rlw, &rlh, NULL);
	lw = ROUND(rlw), lh = ROUND(rlh);
      }
      if (axisPtr->maxTickWidth_ < int(lw))
	axisPtr->maxTickWidth_ = lw;

      if (axisPtr->maxTickHeight_ < int(lh))
	axisPtr->maxTickHeight_ = lh;
    }
	
    unsigned int pad =0;
    if (ops->exterior) {
      /* Because the axis cap style is "CapProjecting", we need to
       * account for an extra 1.5 linewidth at the end of each line.  */
      pad = ((ops->lineWidth * 12) / 8);
    }
    if (axisPtr->isHorizontal())
      y += axisPtr->maxTickHeight_ + pad;
    else {
      y += axisPtr->maxTickWidth_ + pad;
      if (axisPtr->maxTickWidth_ > 0)
	// Pad either size of label.
	y += 5;
    }
    y += 2 * AXIS_PAD_TITLE;
    if ((ops->lineWidth > 0) && ops->exterior)
      // Distance from axis line to tick label.
      y += ops->tickLength;

  } // showTicks

  if (ops->title) {
    if (ops->titleAlternate) {
      if (y < axisPtr->titleHeight_)
	y = axisPtr->titleHeight_;
    } 
    else
      y += axisPtr->titleHeight_ + AXIS_PAD_TITLE;
  }

  // Correct for orientation of the axis
  if (axisPtr->isHorizontal())
    axisPtr->height_ = y;
  else
    axisPtr->width_ = y;
}

double AdjustViewport(double offset, double windowSize)
{
  // Canvas-style scrolling allows the world to be scrolled within the window.
  if (windowSize > 1.0) {
    if (windowSize < (1.0 - offset))
      offset = 1.0 - windowSize;

    if (offset > 0.0)
      offset = 0.0;
  }
  else {
    if ((offset + windowSize) > 1.0)
      offset = 1.0 - windowSize;

    if (offset < 0.0)
      offset = 0.0;
  }
  return offset;
}

