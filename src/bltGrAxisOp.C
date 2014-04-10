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

typedef int (GraphDefAxisProc)(Tcl_Interp* interp, Axis *axisPtr, 
			       int objc, Tcl_Obj* const objv[]);
typedef int (GraphAxisProc)(Tcl_Interp* interp, Graph* graphPtr, 
			    int objc, Tcl_Obj* const objv[]);

extern Tcl_FreeProc FreeAxis;
extern int ConfigureAxis(Axis *axisPtr);
extern int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr);
extern int AxisIsHorizontal(Axis *axisPtr);
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

static int CgetOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)axisPtr, 
				      axisPtr->optionTable,
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
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, 
				       (char*)axisPtr, 
				       axisPtr->optionTable, 
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
  Graph* graphPtr = axisPtr->obj.graphPtr;
  const char *string;

  string = Tcl_GetString(objv[2]);
  if (string[0] == 'a')
    axisPtr->flags |= ACTIVE;
  else
    axisPtr->flags &= ~ACTIVE;

  if (!axisPtr->hide && axisPtr->use) {
    graphPtr->flags |= DRAW_MARGINS | CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
  }

  return TCL_OK;
}

static int BindOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, Blt_MakeAxisTag(graphPtr, axisPtr->obj.name), objc-3, objv+3);
}
          
static int InvTransformOp(Tcl_Interp* interp, Axis *axisPtr, 
			  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

  int sy;
  if (Tcl_GetIntFromObj(interp, objv[3], &sy) != TCL_OK)
    return TCL_ERROR;

  /*
   * Is the axis vertical or horizontal?
   *
   * Check the site where the axis was positioned.  If the axis is
   * virtual, all we have to go on is how it was mapped to an
   * element (using either -mapx or -mapy options).  
   */
  double y;
  if (AxisIsHorizontal(axisPtr))
    y = Blt_InvHMap(axisPtr, (double)sy);
  else
    y = Blt_InvVMap(axisPtr, (double)sy);

  Tcl_SetDoubleObj(Tcl_GetObjResult(interp), y);
  return TCL_OK;
}

static int LimitsOp(Tcl_Interp* interp, Axis *axisPtr, 
		    int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

  double min, max;
  if (axisPtr->logScale) {
    min = EXP10(axisPtr->axisRange.min);
    max = EXP10(axisPtr->axisRange.max);
  } 
  else {
    min = axisPtr->axisRange.min;
    max = axisPtr->axisRange.max;
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
  if (axisPtr->use)
    marginName = axisNames[axisPtr->margin].name;

  Tcl_SetStringObj(Tcl_GetObjResult(interp), marginName, -1);
  return TCL_OK;
}

static int TransformOp(Tcl_Interp* interp, Axis *axisPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

  double x;
  if (Blt_ExprDoubleFromObj(interp, objv[3], &x) != TCL_OK)
    return TCL_ERROR;

  if (AxisIsHorizontal(axisPtr))
    x = Blt_HMap(axisPtr, x);
  else
    x = Blt_VMap(axisPtr, x);

  Tcl_SetIntObj(Tcl_GetObjResult(interp), (int)x);
  return TCL_OK;
}

static int TypeOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  const char *typeName;

  typeName = "";
  if (axisPtr->use) {
    if (axisNames[axisPtr->margin].classId == CID_AXIS_X)
      typeName = "x";
    else if (axisNames[axisPtr->margin].classId == CID_AXIS_Y)
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
			       Tcl_NewStringObj(axisPtr->obj.name, -1));
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
    axisPtr->use =0;
    /* Clear the axis type if it's not currently used.*/
    if (axisPtr->refCount == 0)
      Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
  }
  Blt_Chain_Reset(chain);
  for (int i=0; i<axisObjc; i++) {
    Axis *axisPtr;
    if (GetAxisFromObj(interp, graphPtr, axisObjv[i], &axisPtr) != TCL_OK)
      return TCL_ERROR;

    if (axisPtr->obj.classId == CID_NONE)
      Blt_GraphSetObjectClass(&axisPtr->obj, classId);
    else if (axisPtr->obj.classId != classId) {
      Tcl_AppendResult(interp, "wrong type axis \"", 
		       axisPtr->obj.name, "\": can't use ", 
		       axisPtr->obj.className, " type axis.", NULL); 
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
    axisPtr->use =1;
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
  Graph* graphPtr = axisPtr->obj.graphPtr;
  double worldMin = axisPtr->valueRange.min;
  double worldMax = axisPtr->valueRange.max;
  /* Override data dimensions with user-selected limits. */
  if (!isnan(axisPtr->scrollMin))
    worldMin = axisPtr->scrollMin;

  if (!isnan(axisPtr->scrollMax))
    worldMax = axisPtr->scrollMax;

  double viewMin = axisPtr->min;
  double viewMax = axisPtr->max;
  /* Bound the view within scroll region. */ 
  if (viewMin < worldMin)
    viewMin = worldMin;

  if (viewMax > worldMax)
    viewMax = worldMax;

  if (axisPtr->logScale) {
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
  if (AxisIsHorizontal(axisPtr) != axisPtr->descending) {
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
  if (GetAxisScrollInfo(interp, objc, objv, &fract, viewWidth / worldWidth, axisPtr->scrollUnits, axisScale) != TCL_OK)
    return TCL_ERROR;

  if (AxisIsHorizontal(axisPtr) != axisPtr->descending) {
    axisPtr->reqMin = (fract * worldWidth) + worldMin;
    axisPtr->reqMax = axisPtr->reqMin + viewWidth;
  }
  else {
    axisPtr->reqMax = worldMax - (fract * worldWidth);
    axisPtr->reqMin = axisPtr->reqMax - viewWidth;
  }
  if (axisPtr->logScale) {
    axisPtr->reqMin = EXP10(axisPtr->reqMin);
    axisPtr->reqMax = EXP10(axisPtr->reqMax);
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
  Graph* graphPtr = axisPtr->obj.graphPtr;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)axisPtr, axisPtr->optionTable, 
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
  if (axisPtr->refCount == 0) {
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

    if (axisPtr && !axisPtr->hide && axisPtr->use)
      graphPtr->focusPtr = axisPtr;
  }

  Blt_SetFocusItem(graphPtr->bindTable, graphPtr->focusPtr, NULL);

  if (graphPtr->focusPtr)
    Tcl_SetStringObj(Tcl_GetObjResult(interp), graphPtr->focusPtr->obj.name,-1);

  return TCL_OK;
}

static int AxisGetOp(Tcl_Interp* interp, Graph* graphPtr, 
		     int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr = (Axis*)Blt_GetCurrentItem(graphPtr->bindTable);
  /* Report only on axes. */
  if ((axisPtr) && 
      ((axisPtr->obj.classId == CID_AXIS_X) || 
       (axisPtr->obj.classId == CID_AXIS_Y) || 
       (axisPtr->obj.classId == CID_NONE))) {
    char  *string = Tcl_GetString(objv[3]);
    char c = string[0];
    if ((c == 'c') && (strcmp(string, "current") == 0))
      Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->obj.name,-1);
    else if ((c == 'd') && (strcmp(string, "detail") == 0))
      Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->detail, -1);
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

      Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(axisPtr->obj.name, -1));
    }
  } 
  else {
    Tcl_HashSearch cursor;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
      Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
      for (int ii=3; ii<objc; ii++) {
	const char *pattern = (const char*)Tcl_GetString(objv[ii]);
	if (Tcl_StringMatch(axisPtr->obj.name, pattern)) {
	  Tcl_ListObjAppendElement(interp, listObjPtr, 
				   Tcl_NewStringObj(axisPtr->obj.name, -1));
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

