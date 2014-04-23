/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1991-2004 George A Howlett.
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

#include <tk.h>
#ifdef USE_TK_STUBS
#include <tkInt.h>
#endif

extern "C" {
#include "bltInt.h"
#include "bltList.h"
}

#include "bltGraphLine.h"
#include "bltGraphBar.h"
#include "bltGraphOp.h"

#include "bltGrAxis.h"
#include "bltGrAxisOp.h"
#include "bltGrElem.h"
#include "bltGrElemOp.h"
#include "bltGrHairs.h"
#include "bltGrHairsOp.h"
#include "bltGrLegd.h"
#include "bltGrLegdOp.h"
#include "bltGrMarker.h"
#include "bltGrMarkerOp.h"
#include "bltGrPageSetup.h"
#include "bltGrPageSetupOp.h"
#include "bltGrPen.h"
#include "bltGrPenOp.h"
#include "bltGrXAxisOp.h"

using namespace Blt;

static Tcl_ObjCmdProc BarchartObjCmd;
static Tcl_ObjCmdProc GraphObjCmd;
static Axis* GetFirstAxis(Blt_Chain chain);

#define ROUND(x) 	((int)((x) + (((x)<0.0) ? -0.5 : 0.5)))

int Blt_GraphCmdInitProc(Tcl_Interp* interp)
{
  static Blt_InitCmdSpec graphSpec = 
    {"graph", GraphObjCmd, NULL, NULL};
  static Blt_InitCmdSpec barchartSpec = 
    {"barchart", BarchartObjCmd, NULL, NULL};

  if (Blt_InitCmd(interp, "::blt", &graphSpec) != TCL_OK)
    return TCL_ERROR;
  if (Blt_InitCmd(interp, "::blt", &barchartSpec) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}

static int GraphObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, 
		       Tcl_Obj* const objv[])
{
  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
    return TCL_ERROR;
  }

  Graph* graphPtr = new LineGraph(clientData, interp, objc, objv);
  return graphPtr->valid_ ? TCL_OK : TCL_ERROR;
}

static int BarchartObjCmd(ClientData clientData, Tcl_Interp* interp, int objc, 
			  Tcl_Obj* const objv[])
{
  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "pathName ?options?");
    return TCL_ERROR;
  }

  Graph* graphPtr = new BarGraph(clientData, interp, objc, objv);
  return graphPtr->valid_ ? TCL_OK : TCL_ERROR;
}

int GraphObjConfigure(Graph* graphPtr, Tcl_Interp* interp,
		      int objc, Tcl_Obj* const objv[])
{
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)graphPtr->ops_, graphPtr->optionTable_, 
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
    graphPtr->configure();
    graphPtr->eventuallyRedraw();

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

static int CgetOp(ClientData clientData, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 1, objv, "cget option");
    return TCL_ERROR;
  }
  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)graphPtr->ops_, 
				      graphPtr->optionTable_,
				      objv[2], graphPtr->tkwin_);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(ClientData clientData, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  if (objc <= 3) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(interp, (char*)graphPtr->ops_, 
				       graphPtr->optionTable_, 
				       (objc == 3) ? objv[2] : NULL, 
				       graphPtr->tkwin_);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return GraphObjConfigure(graphPtr, interp, objc-2, objv+2);
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtentsOp --
 *
 *	Reports the size of one of several items within the graph.  The
 *	following are valid items:
 *
 *	  "bottommargin"	Height of the bottom margin
 *	  "leftmargin"		Width of the left margin
 *	  "legend"		x y w h of the legend
 *	  "plotarea"		x y w h of the plotarea
 *	  "plotheight"		Height of the plot area
 *	  "rightmargin"		Width of the right margin
 *	  "topmargin"		Height of the top margin
 *        "plotwidth"		Width of the plot area
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */

static int ExtentsOp(ClientData clientData, Tcl_Interp* interp,
		     int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  GraphOptions* ops = (GraphOptions*)graphPtr->ops_;
  int length;
  const char* string = Tcl_GetStringFromObj(objv[2], &length);
  char c = string[0];
  if ((c == 'p') && (length > 4) && 
      (strncmp("plotheight", string, length) == 0)) {
    int height = graphPtr->bottom_ - graphPtr->top_ + 1;
    Tcl_SetIntObj(Tcl_GetObjResult(interp), height);
  }
  else if ((c == 'p') && (length > 4) &&
	     (strncmp("plotwidth", string, length) == 0)) {
    int width = graphPtr->right_ - graphPtr->left_ + 1;
    Tcl_SetIntObj(Tcl_GetObjResult(interp), width);
  }
  else if ((c == 'p') && (length > 4) &&
	     (strncmp("plotarea", string, length) == 0)) {
    Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->left_));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->top_));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(graphPtr->right_ - graphPtr->left_+1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(graphPtr->bottom_ - graphPtr->top_+1));
    Tcl_SetObjResult(interp, listObjPtr);
  }
  else if ((c == 'l') && (length > 2) &&
	     (strncmp("legend", string, length) == 0)) {
    Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->legend_->x()));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->legend_->y()));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->legend_->width()));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
			     Tcl_NewIntObj(graphPtr->legend_->height()));
    Tcl_SetObjResult(interp, listObjPtr);
  }
  else if ((c == 'l') && (length > 2) &&
	   (strncmp("leftmargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), ops->leftMargin.width);
  }
  else if ((c == 'r') && (length > 1) &&
	     (strncmp("rightmargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), ops->rightMargin.width);
  }
  else if ((c == 't') && (length > 1) &&
	     (strncmp("topmargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), ops->topMargin.height);
  }
  else if ((c == 'b') && (length > 1) &&
	     (strncmp("bottommargin", string, length) == 0)) {
    Tcl_SetIntObj(Tcl_GetObjResult(interp), ops->bottomMargin.height);
  }
  else {
    Tcl_AppendResult(interp, "bad extent item \"", objv[2],
		     "\": should be plotheight, plotwidth, leftmargin, rightmargin, \
topmargin, bottommargin, plotarea, or legend", (char*)NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

#define PointInRegion(e,x,y)			\
  (((x) <= (e)->right) && ((x) >= (e)->left) && \
   ((y) <= (e)->bottom) && ((y) >= (e)->top))

static int InsideOp(ClientData clientData, Tcl_Interp* interp, int objc, 
		    Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  int x;
  if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK)
    return TCL_ERROR;

  int y;
  if (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)
    return TCL_ERROR;

  Region2d exts;
  graphPtr->extents(&exts);

  int result = PointInRegion(&exts, x, y);
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), result);

  return TCL_OK;
}

static int InvtransformOp(ClientData clientData, Tcl_Interp* interp, int objc, 
			  Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  double x, y;
  if ((Blt_ExprDoubleFromObj(interp, objv[2], &x) != TCL_OK) ||
      (Blt_ExprDoubleFromObj(interp, objv[3], &y) != TCL_OK))
    return TCL_ERROR;

  if (graphPtr->flags & RESET_AXES)
    graphPtr->resetAxes();

  // Perform the reverse transformation, converting from window coordinates
  // to graph data coordinates.  Note that the point is always mapped to the
  // bottom and left axes (which may not be what the user wants)
  Axis* xAxis = GetFirstAxis(graphPtr->axisChain_[0]);
  Axis* yAxis = GetFirstAxis(graphPtr->axisChain_[1]);
  Point2d point = graphPtr->invMap2D(x, y, xAxis, yAxis);

  Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(point.x));
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(point.y));
  Tcl_SetObjResult(interp, listObjPtr);

  return TCL_OK;
}

static int TransformOp(ClientData clientData, Tcl_Interp* interp, int objc, 
		       Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  double x, y;
  if ((Blt_ExprDoubleFromObj(interp, objv[2], &x) != TCL_OK) ||
      (Blt_ExprDoubleFromObj(interp, objv[3], &y) != TCL_OK))
    return TCL_ERROR;

  if (graphPtr->flags & RESET_AXES)
    graphPtr->resetAxes();

  /*
   * Perform the transformation from window to graph coordinates.  Note that
   * the points are always mapped onto the bottom and left axes (which may
   * not be the what the user wants).
   */
  Axis* xAxis = GetFirstAxis(graphPtr->axisChain_[0]);
  Axis* yAxis = GetFirstAxis(graphPtr->axisChain_[1]);

  Point2d point = graphPtr->map2D(x, y, xAxis, yAxis);

  Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(ROUND(point.x)));
  Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(ROUND(point.y)));
  Tcl_SetObjResult(interp, listObjPtr);

  return TCL_OK;
}

static const TkEnsemble graphEnsemble[] = {
  {"axis",        0, axisEnsemble},
  {"bar",         0, elementEnsemble},
  {"cget", 	  CgetOp, 0},
  {"configure",   ConfigureOp, 0},
  {"crosshairs",  0, crosshairsEnsemble},
  {"element",     0, elementEnsemble},
  {"extents",     ExtentsOp, 0},
  {"inside",      InsideOp, 0},
  {"invtransform",InvtransformOp, 0},
  {"legend",      0, legendEnsemble},
  {"line",        0, elementEnsemble},
  {"marker",      0, markerEnsemble},
  {"pen",         0, penEnsemble},
  {"postscript",  0, pageSetupEnsemble},
  {"transform",   TransformOp, 0},
  {"xaxis",       0, xaxisEnsemble},
  {"yaxis",       0, xaxisEnsemble},
  {"x2axis",      0, xaxisEnsemble},
  {"y2axis",      0, xaxisEnsemble},
  { 0,0,0 }
};

static int InvokeEnsemble(const TkEnsemble* ensemble, int cmdIndex,
			  void* clientData, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  while (cmdIndex < objc) {
    int index;
    if (Tcl_GetIndexFromObjStruct(interp, objv[cmdIndex], ensemble, sizeof(ensemble[0]), "command", 0, &index) != TCL_OK)
      return TCL_ERROR;

    if (ensemble[index].proc)
      return ensemble[index].proc(clientData, interp, objc, objv);

    ensemble = ensemble[index].subensemble;
    ++cmdIndex;
  }

  Tcl_WrongNumArgs(interp, cmdIndex, objv, "option ?arg ...?");
  return TCL_ERROR;
}

int GraphInstCmdProc(ClientData clientData, Tcl_Interp* interp, 
		     int objc, Tcl_Obj* const objv[])
{
  Tcl_Preserve(clientData);
  int result = InvokeEnsemble(graphEnsemble, 1, clientData, interp, objc, objv);
  Tcl_Release(clientData);
  return result;
}

/*
static Blt_OpSpec graphOps[] =
  {
    {"axis",         1, (void*)Blt_AxisOp,        2, 0, "oper ?args?",},
    {"bar",          2, (void*)ElementOp,         2, 0, "oper ?args?",},
    {"cget",         2, (void*)CgetOp,            3, 3, "option",},
    {"configure",    2, (void*)ConfigureOp,       2, 0, "?option value?...",},
    {"crosshairs",   2, (void*)Blt_CrosshairsOp,  2, 0, "oper ?args?",},
    {"element",      2, (void*)ElementOp,         2, 0, "oper ?args?",},
    {"extents",      2, (void*)ExtentsOp,         3, 3, "item",},
    {"inside",       3, (void*)InsideOp,          4, 4, "winX winY",},
    {"invtransform", 3, (void*)InvtransformOp,    4, 4, "winX winY",},
    {"legend",       2, (void*)Blt_LegendOp,      2, 0, "oper ?args?",},
    {"line",         2, (void*)ElementOp,         2, 0, "oper ?args?",},
    {"marker",       2, (void*)Blt::MarkerOp,     2, 0, "oper ?args?",},
    {"pen",          2, (void*)Blt_PenOp,         2, 0, "oper ?args?",},
    {"postscript",   2, (void*)Blt_PageSetupOp,   2, 0, "oper ?args?",},
    {"transform",    1, (void*)TransformOp,       4, 4, "x y",},
    {"x2axis",       2, (void*)X2AxisOp,          2, 0, "oper ?args?",},
    {"xaxis",        2, (void*)XAxisOp,           2, 0, "oper ?args?",},
    {"y2axis",       2, (void*)Y2AxisOp,          2, 0, "oper ?args?",},
    {"yaxis",        2, (void*)YAxisOp,           2, 0, "oper ?args?",},
  };
static int nGraphOps = sizeof(graphOps) / sizeof(Blt_OpSpec);

typedef int (GraphCmdProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			   Tcl_Obj* const objv[]);

int GraphInstCmdProc(ClientData clientData, Tcl_Interp* interp, 
		     int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  GraphCmdProc* proc = (GraphCmdProc*)Blt_GetOpFromObj(interp, nGraphOps, graphOps, BLT_OP_ARG1, objc, objv, 0);
  if (!proc)
    return TCL_ERROR;

  Tcl_Preserve(graphPtr);
  int result = (*proc)(graphPtr, interp, objc, objv);
  Tcl_Release(graphPtr);
  return result;
}
*/

// called by Tcl_DeleteCommand
void GraphInstCmdDeleteProc(ClientData clientData)
{
  Graph* graphPtr = (Graph*)clientData;
  if (!(graphPtr->flags & GRAPH_DELETED))
    Tk_DestroyWindow(graphPtr->tkwin_);
}

void GraphEventProc(ClientData clientData, XEvent* eventPtr)
{
  Graph* graphPtr = (Graph*)clientData;

  if (eventPtr->type == Expose) {
    if (eventPtr->xexpose.count == 0) {
      graphPtr->flags |= REDRAW_WORLD;
      graphPtr->eventuallyRedraw();
    }

  }
  else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
    if (eventPtr->xfocus.detail != NotifyInferior) {
      if (eventPtr->type == FocusIn)
	graphPtr->flags |= FOCUS;
      else
	graphPtr->flags &= ~FOCUS;
      graphPtr->flags |= REDRAW_WORLD;
      graphPtr->eventuallyRedraw();
    }

  }
  else if (eventPtr->type == DestroyNotify) {
    if (!(graphPtr->flags & GRAPH_DELETED)) {
      graphPtr->flags |= GRAPH_DELETED;
      Tcl_DeleteCommandFromToken(graphPtr->interp_, graphPtr->cmdToken_);
      if (graphPtr->flags & REDRAW_PENDING)
	Tcl_CancelIdleCall(DisplayGraph, graphPtr);
      Tcl_EventuallyFree(graphPtr, DestroyGraph);
    }
  }
  else if (eventPtr->type == ConfigureNotify) {
    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
    graphPtr->eventuallyRedraw();
  }
}

void DisplayGraph(ClientData clientData)
{
  Graph* graphPtr = (Graph*)clientData;
  graphPtr->draw();
}

// called by Tcl_EventuallyFree and others
void DestroyGraph(char* dataPtr)
{
  Graph* graphPtr = (Graph*)dataPtr;
  delete graphPtr;
}

static Axis* GetFirstAxis(Blt_Chain chain)
{
  Blt_ChainLink link = Blt_Chain_FirstLink(chain);
  if (!link)
    return NULL;

  return (Axis*)Blt_Chain_GetValue(link);
}

