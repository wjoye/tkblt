
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

#include "bltGraph.h"
#include "bltOp.h"

typedef int (GraphCrosshairProc)(Graph *graphPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);

/*
 *---------------------------------------------------------------------------
 *
 * Crosshairs
 *
 *	Contains the line segments positions and graphics context used
 *	to simulate crosshairs (by XORing) on the graph.
 *
 *---------------------------------------------------------------------------
 */

struct _Crosshairs {

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

#define DEF_HAIRS_DASHES	(char *)NULL
#define DEF_HAIRS_FOREGROUND	RGB_BLACK
#define DEF_HAIRS_LINE_WIDTH	"0"
#define DEF_HAIRS_HIDE		"yes"
#define DEF_HAIRS_POSITION	(char *)NULL

BLT_EXTERN Blt_CustomOption bltPointOption;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_COLOR, "-color", "color", "Color", DEF_HAIRS_FOREGROUND, 
	Blt_Offset(Crosshairs, colorPtr), 0},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_HAIRS_DASHES, 
	Blt_Offset(Crosshairs, dashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", DEF_HAIRS_HIDE, 
	Blt_Offset(Crosshairs, hidden), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "Linewidth",
	DEF_HAIRS_LINE_WIDTH, Blt_Offset(Crosshairs, lineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-position", "position", "Position", 
	DEF_HAIRS_POSITION, Blt_Offset(Crosshairs, hotSpot), 0, &bltPointOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/*
 *---------------------------------------------------------------------------
 *
 * TurnOffHairs --
 *
 *	XOR's the existing line segments (representing the crosshairs),
 *	thereby erasing them.  The internal state of the crosshairs is
 *	tracked.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Crosshairs are erased.
 *
 *---------------------------------------------------------------------------
 */
static void
TurnOffHairs(Tk_Window tkwin, Crosshairs *chPtr)
{
    if (Tk_IsMapped(tkwin) && (chPtr->visible)) {
	XDrawSegments(Tk_Display(tkwin), Tk_WindowId(tkwin), chPtr->gc,
	    chPtr->segArr, 2);
	chPtr->visible = FALSE;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TurnOnHairs --
 *
 *	Draws (by XORing) new line segments, creating the effect of
 *	crosshairs. The internal state of the crosshairs is tracked.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Crosshairs are displayed.
 *
 *---------------------------------------------------------------------------
 */
static void
TurnOnHairs(Graph *graphPtr, Crosshairs *chPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureCrosshairs --
 *
 *	Configures attributes of the crosshairs such as line width,
 *	dashes, and position.  The crosshairs are first turned off
 *	before any of the attributes changes.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Crosshair GC is allocated.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ConfigureCrosshairs(Graph *graphPtr)
{
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;
    unsigned long int pixel;
    Crosshairs *chPtr = graphPtr->crosshairs;

    /*
     * Turn off the crosshairs temporarily. This is in case the new
     * configuration changes the size, style, or position of the lines.
     */
    TurnOffHairs(graphPtr->tkwin, chPtr);

    gcValues.function = GXxor;

    if (graphPtr->plotBg == NULL) {
	/* The graph's color option may not have been set yet */
	pixel = WhitePixelOfScreen(Tk_Screen(graphPtr->tkwin));
    } else {
	pixel = Blt_BackgroundBorderColor(graphPtr->plotBg)->pixel;
    }
    gcValues.background = pixel;
    gcValues.foreground = (pixel ^ chPtr->colorPtr->pixel);

    gcValues.line_width = LineWidth(chPtr->lineWidth);
    gcMask = (GCForeground | GCBackground | GCFunction | GCLineWidth);
    if (LineIsDashed(chPtr->dashes)) {
	gcValues.line_style = LineOnOffDash;
	gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(chPtr->dashes)) {
	Blt_SetDashes(graphPtr->display, newGC, &chPtr->dashes);
    }
    if (chPtr->gc != NULL) {
	Blt_FreePrivateGC(graphPtr->display, chPtr->gc);
    }
    chPtr->gc = newGC;

    /*
     * Are the new coordinates on the graph?
     */
    chPtr->segArr[0].x2 = chPtr->segArr[0].x1 = chPtr->hotSpot.x;
    chPtr->segArr[0].y1 = graphPtr->bottom;
    chPtr->segArr[0].y2 = graphPtr->top;
    chPtr->segArr[1].y2 = chPtr->segArr[1].y1 = chPtr->hotSpot.y;
    chPtr->segArr[1].x1 = graphPtr->left;
    chPtr->segArr[1].x2 = graphPtr->right;

    if (!chPtr->hidden) {
	TurnOnHairs(graphPtr, chPtr);
    }
}

void
Blt_EnableCrosshairs(Graph *graphPtr)
{
    if (!graphPtr->crosshairs->hidden) {
	TurnOnHairs(graphPtr, graphPtr->crosshairs);
    }
}

void
Blt_DisableCrosshairs(Graph *graphPtr)
{
    if (!graphPtr->crosshairs->hidden) {
	TurnOffHairs(graphPtr->tkwin, graphPtr->crosshairs);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * UpdateCrosshairs --
 *
 *	Update the length of the hairs (not the hot spot).
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UpdateCrosshairs(Graph *graphPtr)
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    chPtr->segArr[0].y1 = graphPtr->bottom;
    chPtr->segArr[0].y2 = graphPtr->top;
    chPtr->segArr[1].x1 = graphPtr->left;
    chPtr->segArr[1].x2 = graphPtr->right;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyCrosshairs --
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Crosshair GC is allocated.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyCrosshairs(Graph *graphPtr)
{
    if (graphPtr->crosshairs != NULL) {
	Crosshairs *chPtr = graphPtr->crosshairs;

	Blt_FreeOptions(configSpecs, (char *)chPtr, graphPtr->display, 0);
	if (chPtr->gc != NULL) {
	    Blt_FreePrivateGC(graphPtr->display, chPtr->gc);
	}
	free(chPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateCrosshairs --
 *
 *	Creates and initializes a new crosshair structure.
 *
 * Results:
 *	Returns TCL_ERROR if the crosshair structure can't be created,
 *	otherwise TCL_OK.
 *
 * Side Effects:
 *	Crosshair GC is allocated.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreateCrosshairs(Graph *graphPtr)
{
    Crosshairs *chPtr;

    chPtr = calloc(1, sizeof(Crosshairs));
    chPtr->hidden = TRUE;
    chPtr->hotSpot.x = chPtr->hotSpot.y = -1;
    graphPtr->crosshairs = chPtr;

    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
	    "crosshairs", "Crosshairs", configSpecs, 0, (Tcl_Obj **)NULL,
	    (char *)chPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Queries configuration attributes of the crosshairs such as
 *	line width, dashes, and position.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)chPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Queries or resets configuration attributes of the crosshairs
 * 	such as line width, dashes, and position.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Crosshairs are reset.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)chPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)chPtr, objv[3], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	objc - 3, objv + 3, (char *)chPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_ConfigureCrosshairs(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OnOp --
 *
 *	Maps the crosshairs.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Crosshairs are reset if necessary.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OnOp(
    Graph *graphPtr,
    Tcl_Interp *interp,		/* Not used. */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)	/* Not used. */
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    if (chPtr->hidden) {
	TurnOnHairs(graphPtr, chPtr);
	chPtr->hidden = FALSE;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OffOp --
 *
 *	Unmaps the crosshairs.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Crosshairs are reset if necessary.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OffOp(
    Graph *graphPtr,
    Tcl_Interp *interp,		/* Not used. */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)	/* Not used. */
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    if (!chPtr->hidden) {
	TurnOffHairs(graphPtr->tkwin, chPtr);
	chPtr->hidden = TRUE;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ToggleOp --
 *
 *	Toggles the state of the crosshairs.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Crosshairs are reset.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ToggleOp(
    Graph *graphPtr,
    Tcl_Interp *interp,		/* Not used. */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)	/* Not used. */
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    chPtr->hidden = (chPtr->hidden == 0);
    if (chPtr->hidden) {
	TurnOffHairs(graphPtr->tkwin, chPtr);
    } else {
	TurnOnHairs(graphPtr, chPtr);
    }
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

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CrosshairsOp --
 *
 *	User routine to configure crosshair simulation.  Crosshairs
 *	are simulated by drawing line segments parallel to both axes
 *	using the XOR drawing function. The allows the lines to be
 *	erased (by drawing them again) without redrawing the entire
 *	graph.  Care must be taken to erase crosshairs before redrawing
 *	the graph and redraw them after the graph is redraw.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Crosshairs may be drawn in the plotting area.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CrosshairsOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    GraphCrosshairProc *proc;

    proc = Blt_GetOpFromObj(interp, nXhairOps, xhairOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (graphPtr, interp, objc, objv);
}
