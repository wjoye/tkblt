/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrHairs.c --
 *
 * This module implements crosshairs for the BLT graph widget.
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

#include "bltInt.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltConfig.h"

struct _Crosshairs {
  Tk_OptionTable optionTable;
  XPoint hotSpot;		/* Hot spot for crosshairs */
  int visible;		/* Internal state of crosshairs. If non-zero,
			 * crosshairs are displayed. */
  int hidden;			/* If non-zero, crosshairs are not displayed.
				 * This is not necessarily consistent with the
				 * internal state variable.  This is true when
				 * the hot spot is off the graph.  */
  Blt_Dashes dashes;		/* Dashstyle of the crosshairs. This represents
				 * an array of alternatingly drawn pixel
				 * values. If NULL, the hairs are drawn as a
				 * solid line */
  int lineWidth;		/* Width of the simulated crosshair lines */
  XSegment segArr[2];		/* Positions of line segments representing the
				 * simulated crosshairs. */
  XColor *colorPtr;		/* Foreground color of crosshairs */
  GC gc;			/* Graphics context for crosshairs. Set to
				 * GXxor to not require redraws of graph */
};

#define DEF_HAIRS_DASHES	NULL
#define DEF_HAIRS_FOREGROUND	black
#define DEF_HAIRS_LINE_WIDTH	"0"
#define DEF_HAIRS_HIDE		"yes"
#define DEF_HAIRS_POSITION	NULL

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_COLOR, "-color", "color", "Color", 
   DEF_HAIRS_FOREGROUND, 
   -1, Tk_Offset(Crosshairs, colorPtr), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   DEF_HAIRS_HIDE, 
   -1, Tk_Offset(Crosshairs, hidden), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "Linewidth",
   DEF_HAIRS_LINE_WIDTH, 
   -1, Tk_Offset(Crosshairs, lineWidth), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-position", "position", "Position", 
   DEF_HAIRS_POSITION, 
   -1, Tk_Offset(Crosshairs, hotSpot), 0, &pointObjOption, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

static int CrosshairsObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
				  int objc, Tcl_Obj* const objv[]);
static void ConfigureCrosshairs(Graph *graphPtr);
static void TurnOffHairs(Tk_Window tkwin, Crosshairs *chPtr);
static void TurnOnHairs(Graph *graphPtr, Crosshairs *chPtr);

int Blt_CreateCrosshairs(Graph* graphPtr)
{
  Crosshairs* chPtr = calloc(1, sizeof(Crosshairs));
  chPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);
  chPtr->hidden = TRUE;
  chPtr->hotSpot.x = chPtr->hotSpot.y = -1;
  graphPtr->crosshairs = chPtr;
  return TCL_OK;
}

int Blt_ConfigureObjCrosshairs(Graph* graphPtr,
			       int objc, Tcl_Obj* const objv[])
{
  Crosshairs* chPtr = graphPtr->crosshairs;
  return Tk_InitOptions(graphPtr->interp, (char*)chPtr, chPtr->optionTable, 
			graphPtr->tkwin);
}

static Blt_OpSpec xhairOps[];
static int nXhairOps;
typedef int (GraphCrosshairProc)(Graph* graphPtr, Tcl_Interp* interp, 
	int objc, Tcl_Obj* const objv[]);

int Blt_CrosshairsOp(Graph* graphPtr, Tcl_Interp* interp,
		     int objc, Tcl_Obj* const objv[])
{
  GraphCrosshairProc* proc = Blt_GetOpFromObj(interp, nXhairOps, xhairOps, 
					      BLT_OP_ARG2, objc, objv, 0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(graphPtr, interp, objc, objv);
}

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Crosshairs* chPtr = graphPtr->crosshairs;
  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)chPtr, 
				      chPtr->optionTable,
				      objv[3], graphPtr->tkwin);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Graph *graphPtr, Tcl_Interp *interp,
		       int objc, Tcl_Obj* const objv[])
{
  Crosshairs* chPtr = graphPtr->crosshairs;
  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, (char*)chPtr, 
				       chPtr->optionTable, 
				       (objc == 4) ? objv[3] : NULL, 
				       graphPtr->tkwin);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return CrosshairsObjConfigure(interp, graphPtr, objc-3, objv+3);
}

static int CrosshairsObjConfigure(Tcl_Interp *interp, Graph* graphPtr,
				  int objc, Tcl_Obj* const objv[])
{
  Crosshairs* chPtr = graphPtr->crosshairs;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)chPtr, chPtr->optionTable, 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    ConfigureCrosshairs(graphPtr);
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

static void ConfigureCrosshairs(Graph *graphPtr)
{
  Crosshairs *chPtr = graphPtr->crosshairs;

  // Turn off the crosshairs temporarily. This is in case the new
  // configuration changes the size, style, or position of the lines.
  TurnOffHairs(graphPtr->tkwin, chPtr);

  XGCValues gcValues;
  gcValues.function = GXxor;

  unsigned long int pixel;
  if (graphPtr->plotBg == NULL) {
    // The graph's color option may not have been set yet
    pixel = WhitePixelOfScreen(Tk_Screen(graphPtr->tkwin));
  } else {
    pixel = Blt_BackgroundBorderColor(graphPtr->plotBg)->pixel;
  }
  gcValues.background = pixel;
  gcValues.foreground = (pixel ^ chPtr->colorPtr->pixel);

  gcValues.line_width = LineWidth(chPtr->lineWidth);
  unsigned long gcMask = 
    (GCForeground | GCBackground | GCFunction | GCLineWidth);
  if (LineIsDashed(chPtr->dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  GC newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(chPtr->dashes))
    Blt_SetDashes(graphPtr->display, newGC, &chPtr->dashes);

  if (chPtr->gc != NULL)
    Blt_FreePrivateGC(graphPtr->display, chPtr->gc);

  chPtr->gc = newGC;

  // Are the new coordinates on the graph?
  chPtr->segArr[0].x2 = chPtr->segArr[0].x1 = chPtr->hotSpot.x;
  chPtr->segArr[0].y1 = graphPtr->bottom;
  chPtr->segArr[0].y2 = graphPtr->top;
  chPtr->segArr[1].y2 = chPtr->segArr[1].y1 = chPtr->hotSpot.y;
  chPtr->segArr[1].x1 = graphPtr->left;
  chPtr->segArr[1].x2 = graphPtr->right;

  if (!chPtr->hidden)
    TurnOnHairs(graphPtr, chPtr);
}

void Blt_DeleteCrosshairs(Graph* graphPtr)
{
  Crosshairs *chPtr = graphPtr->crosshairs;
  if (chPtr != NULL)
    Tk_FreeConfigOptions((char*)chPtr, chPtr->optionTable, graphPtr->tkwin);
}

void Blt_DestroyCrosshairs(Graph* graphPtr)
{
  Crosshairs *chPtr = graphPtr->crosshairs;
  if (chPtr != NULL) {
    if (chPtr->gc != NULL)
      Blt_FreePrivateGC(graphPtr->display, chPtr->gc);

    free(chPtr);
  }
}

// Widget commands

static int OnOp(Graph *graphPtr, Tcl_Interp *interp, 
		int objc, Tcl_Obj *const *objv)
{
  Crosshairs *chPtr = graphPtr->crosshairs;

  if (chPtr->hidden) {
    TurnOnHairs(graphPtr, chPtr);
    chPtr->hidden = FALSE;
  }
  return TCL_OK;
}

static int OffOp(Graph *graphPtr, Tcl_Interp *interp,
		 int objc, Tcl_Obj *const *objv)
{
  Crosshairs *chPtr = graphPtr->crosshairs;

  if (!chPtr->hidden) {
    TurnOffHairs(graphPtr->tkwin, chPtr);
    chPtr->hidden = TRUE;
  }
  return TCL_OK;
}

static int ToggleOp(Graph *graphPtr, Tcl_Interp *interp,
		    int objc, Tcl_Obj *const *objv)
{
  Crosshairs *chPtr = graphPtr->crosshairs;

  chPtr->hidden = (chPtr->hidden == 0);
  if (chPtr->hidden)
    TurnOffHairs(graphPtr->tkwin, chPtr);
  else
    TurnOnHairs(graphPtr, chPtr);

  return TCL_OK;
}

static Blt_OpSpec xhairOps[] =
{
    {"cget", 2, CgetOp, 4, 4, "option",},
    {"configure", 2, ConfigureOp, 3, 0, "?options...?",},
    {"off", 2, OffOp, 3, 3, "",},
    {"on", 2, OnOp, 3, 3, "",},
    {"toggle", 1, ToggleOp, 3, 3, "",},
};
static int nXhairOps = sizeof(xhairOps) / sizeof(Blt_OpSpec);

// Support

static void TurnOffHairs(Tk_Window tkwin, Crosshairs *chPtr)
{
  if (Tk_IsMapped(tkwin) && (chPtr->visible)) {
    XDrawSegments(Tk_Display(tkwin), Tk_WindowId(tkwin), chPtr->gc,
		  chPtr->segArr, 2);
    chPtr->visible = FALSE;
  }
}

static void TurnOnHairs(Graph *graphPtr, Crosshairs *chPtr)
{
  if (Tk_IsMapped(graphPtr->tkwin) && (!chPtr->visible)) {
    if (!PointInGraph(graphPtr, chPtr->hotSpot.x, chPtr->hotSpot.y)) {
      return;		/* Coordinates are off the graph */
    }
    XDrawSegments(graphPtr->display, Tk_WindowId(graphPtr->tkwin),
		  chPtr->gc, chPtr->segArr, 2);
    chPtr->visible = TRUE;
  }
}

void Blt_EnableCrosshairs(Graph *graphPtr)
{
  if (!graphPtr->crosshairs->hidden) {
    TurnOnHairs(graphPtr, graphPtr->crosshairs);
  }
}

void Blt_DisableCrosshairs(Graph *graphPtr)
{
  if (!graphPtr->crosshairs->hidden) {
    TurnOffHairs(graphPtr->tkwin, graphPtr->crosshairs);
  }
}
