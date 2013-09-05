
/*
 * bltGrPen.c --
 *
 * This module implements pens for the BLT graph widget.
 *
 *	Copyright 1996-2004 George A Howlett.
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

#include <assert.h>
#include <X11/Xutil.h>

#include "bltGraph.h"
#include "bltOp.h"

typedef int (GraphPenProc)(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	Tcl_Obj *const *objv);

static Blt_OptionFreeProc FreeColor;
static Blt_OptionParseProc ObjToColor;
static Blt_OptionPrintProc ColorToObj;
Blt_CustomOption bltColorOption = {
    ObjToColor, ColorToObj, FreeColor, (ClientData)0
};

static Blt_OptionFreeProc FreePen;
static Blt_OptionParseProc ObjToPen;
static Blt_OptionPrintProc PenToObj;
Blt_CustomOption bltBarPenOption = {
    ObjToPen, PenToObj, FreePen, (ClientData)CID_ELEM_BAR
};
Blt_CustomOption bltLinePenOption = {
    ObjToPen, PenToObj, FreePen, (ClientData)CID_ELEM_LINE
};

/*ARGSUSED*/
static void
FreeColor(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    XColor *colorPtr = *(XColor **)(widgRec + offset);

    if ((colorPtr != NULL) && (colorPtr != COLOR_DEFAULT)) {
	Tk_FreeColor(colorPtr);
    }
}

/*
 *---------------------------------------------------------------------------

 * ObjToColor --
 *
 *	Convert the string representation of a color into a XColor pointer.
 *
 * Results:
 *	The return value is a standard TCL result.  The color pointer is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColor(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing color */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    XColor **colorPtrPtr = (XColor **)(widgRec + offset);
    XColor *colorPtr;
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];

    if ((c == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	if ((*colorPtrPtr != NULL) && (*colorPtrPtr != COLOR_DEFAULT)) {
	    Tk_FreeColor(*colorPtrPtr);
	}
	*colorPtrPtr = NULL;
	return TCL_OK;
    } 
    if ((c == 'd') && (strncmp(string, "defcolor", length) == 0)) {
	if ((*colorPtrPtr != NULL) && (*colorPtrPtr != COLOR_DEFAULT)) {
	    Tk_FreeColor(*colorPtrPtr);
	}
	*colorPtrPtr = COLOR_DEFAULT;
	return TCL_OK;
    } 
    colorPtr = Tk_AllocColorFromObj(interp, tkwin, objPtr);
    if (colorPtr == NULL) {
	return TCL_ERROR;
    }
    if ((*colorPtrPtr != NULL) && (*colorPtrPtr != COLOR_DEFAULT)) {
	Tk_FreeColor(*colorPtrPtr);
    }
    *colorPtrPtr = colorPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorToObj --
 *
 *	Convert the color value into a string.
 *
 * Results:
 *	The string representing the symbol color is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColorToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget information record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    XColor *colorPtr = *(XColor **)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (colorPtr == NULL) {
	objPtr = Tcl_NewStringObj("", -1);
    } else if (colorPtr == COLOR_DEFAULT) {
	objPtr = Tcl_NewStringObj("defcolor", -1);
    } else {
	objPtr = Tcl_NewStringObj(Tk_NameOfColor(colorPtr), -1);
    }
    return objPtr;
}

/*ARGSUSED*/
static void
FreePen(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    Pen **penPtrPtr = (Pen **)(widgRec + offset);

    if (*penPtrPtr != NULL) {
	Blt_FreePen(*penPtrPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPen --
 *
 *	Convert the color value into a string.
 *
 * Results:
 *	The string representing the symbol color is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPen(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing pen */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Pen **penPtrPtr = (Pen **)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	Blt_FreePen(*penPtrPtr);
	*penPtrPtr = NULL;
    } else {
	Pen *penPtr;
	Graph *graphPtr;
	ClassId classId = (ClassId)clientData; /* Element type. */

	graphPtr = Blt_GetGraphFromWindowData(tkwin);
	assert(graphPtr);

	if (classId == CID_NONE) {	
	    classId = graphPtr->classId;
	}
	if (Blt_GetPenFromObj(interp, graphPtr, objPtr, classId, &penPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_FreePen(*penPtrPtr);
	*penPtrPtr = penPtr;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PenToObj --
 *
 *	Parse the name of the name.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PenToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget information record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    Pen *penPtr = *(Pen **)(widgRec + offset);

    if (penPtr == NULL) {
	return Tcl_NewStringObj("", -1);
    } else {
	return Tcl_NewStringObj(penPtr->name, -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPenFromObj --
 *
 *	Find and return the pen style from a given name.
 *
 * Results:
 *     	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetPenFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
	      Pen **penPtrPtr)
{
    Blt_HashEntry *hPtr;
    Pen *penPtr;
    const char *name;

    penPtr = NULL;
    name = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->penTable, name);
    if (hPtr != NULL) {
	penPtr = Blt_GetHashValue(hPtr);
	if (penPtr->flags & DELETE_PENDING) {
	    penPtr = NULL;
	}
    }
    if (penPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find pen \"", name, "\" in \"", 
		Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *penPtrPtr = penPtr;
    return TCL_OK;
}

static void
DestroyPen(Pen *penPtr)
{
    Graph *graphPtr = penPtr->graphPtr;

    Blt_FreeOptions(penPtr->configSpecs, (char *)penPtr, graphPtr->display, 0);
    (*penPtr->destroyProc) (graphPtr, penPtr);
    if ((penPtr->name != NULL) && (penPtr->name[0] != '\0')) {
      free((void*)(penPtr->name));
    }
    if (penPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&graphPtr->penTable, penPtr->hashPtr);
    }
    free(penPtr);
}

void
Blt_FreePen(Pen *penPtr)
{
    if (penPtr != NULL) {
	penPtr->refCount--;
	if ((penPtr->refCount == 0) && (penPtr->flags & DELETE_PENDING)) {
	    DestroyPen(penPtr);
	}
    }
}

Pen *
Blt_CreatePen(Graph *graphPtr, const char *penName, ClassId classId,
	      int objc, Tcl_Obj *const *objv)
{
    Pen *penPtr;
    Blt_HashEntry *hPtr;
    unsigned int configFlags;
    int isNew;
    int i;

    /*
     * Scan the option list for a "-type" entry.  This will indicate what type
     * of pen we are creating. Otherwise we'll default to the suggested type.
     * Last -type option wins.
     */
    for (i = 0; i < objc; i += 2) {
	char *string;
	int length;

	string = Tcl_GetStringFromObj(objv[i],  &length);
	if ((length > 2) && (strncmp(string, "-type", length) == 0)) {
	    char *arg;

	    arg = Tcl_GetString(objv[i + 1]);
	    if (strcmp(arg, "bar") == 0) {
		classId = CID_ELEM_BAR;
	    } else if (strcmp(arg, "line") == 0) {
		classId = CID_ELEM_LINE;
	    } else if (strcmp(arg, "strip") == 0) {
		classId = CID_ELEM_LINE;
	    } else if (strcmp(arg, "contour") == 0) {
		classId = CID_ELEM_CONTOUR;
	    } else {
		Tcl_AppendResult(graphPtr->interp, "unknown pen type \"",
		    arg, "\" specified", (char *)NULL);
		return NULL;
	    }
	}
    }
    if (classId == CID_ELEM_STRIP) {
	classId = CID_ELEM_LINE;
    }
    hPtr = Blt_CreateHashEntry(&graphPtr->penTable, penName, &isNew);
    if (!isNew) {
	penPtr = Blt_GetHashValue(hPtr);
	if ((penPtr->flags & DELETE_PENDING) == 0) {
	    Tcl_AppendResult(graphPtr->interp, "pen \"", penName,
		"\" already exists in \"", Tk_PathName(graphPtr->tkwin), "\"",
		(char *)NULL);
	    return NULL;
	}
	if (penPtr->classId != classId) {
	    Tcl_AppendResult(graphPtr->interp, "pen \"", penName,
		"\" in-use: can't change pen type from \"", 
		Blt_GraphClassName(penPtr->classId), "\" to \"", 
		Blt_GraphClassName(classId), "\"", (char *)NULL);
	    return NULL;
	}
	penPtr->flags &= ~DELETE_PENDING; /* Undelete the pen. */
    } else {
	if (classId == CID_ELEM_BAR) {
	    penPtr = Blt_BarPen(penName);
	} else {
	    penPtr = Blt_LinePen(penName);
	}
	penPtr->classId = classId;
	penPtr->hashPtr = hPtr;
	penPtr->graphPtr = graphPtr;
	Blt_SetHashValue(hPtr, penPtr);
    }
    configFlags = (penPtr->flags & (ACTIVE_PEN | NORMAL_PEN));
    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
	    penPtr->name, "Pen", penPtr->configSpecs, objc, objv,
	    (char *)penPtr, configFlags) != TCL_OK) {
	if (isNew) {
	    DestroyPen(penPtr);
	}
	return NULL;
    }
    (*penPtr->configProc) (graphPtr, penPtr);
    return penPtr;
}

int
Blt_GetPenFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
		  ClassId classId, Pen **penPtrPtr)
{
    Blt_HashEntry *hPtr;
    Pen *penPtr;
    const char *name;
    
    penPtr = NULL;
    name = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->penTable, name);
    if (hPtr != NULL) {
	penPtr = Blt_GetHashValue(hPtr);
	if (penPtr->flags & DELETE_PENDING) {
	    penPtr = NULL;
	}
    }
    if (penPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find pen \"", name, "\" in \"", 
		Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (classId == CID_ELEM_STRIP) {
	classId = CID_ELEM_LINE;
    }
    if (penPtr->classId != classId) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "pen \"", name, 
		"\" is the wrong type (is \"", 
		Blt_GraphClassName(penPtr->classId), "\"", ", wanted \"", 
		Blt_GraphClassName(classId), "\")", (char *)NULL);
	}
	return TCL_ERROR;
    }
    penPtr->refCount++;
    *penPtrPtr = penPtr;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyPens --
 *
 *	Release memory and resources allocated for the style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the pen style is freed up.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyPens(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&graphPtr->penTable, &iter);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	Pen *penPtr;

	penPtr = Blt_GetHashValue(hPtr);
	penPtr->hashPtr = NULL;
	DestroyPen(penPtr);
    }
    Blt_DeleteHashTable(&graphPtr->penTable);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Queries axis attributes (font, line width, label, etc).
 *
 * Results:
 *	A standard TCL result.  If querying configuration values,
 *	interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Pen *penPtr;
    unsigned int configFlags;

    if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    configFlags = (penPtr->flags & (ACTIVE_PEN | NORMAL_PEN));
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, 
	penPtr->configSpecs, (char *)penPtr, objv[4], configFlags);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Queries or resets pen attributes (font, line width, color, etc).
 *
 * Results:
 *	A standard TCL result.  If querying configuration values,
 *	interp->result will contain the results.
 *
 * Side Effects:
 *	Pen resources are possibly allocated (GC, font).
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Pen *penPtr;
    int nNames, nOpts;
    int redraw;
    Tcl_Obj *const *options;
    int i;

    /* Figure out where the option value pairs begin */
    objc -= 3;
    objv += 3;
    for (i = 0; i < objc; i++) {
	char *string;

	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
	if (GetPenFromObj(interp, graphPtr, objv[i], &penPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    nNames = i;			/* Number of pen names specified */
    nOpts = objc - i;		/* Number of options specified */
    options = objv + i;		/* Start of options in objv  */

    redraw = 0;
    for (i = 0; i < nNames; i++) {
    int flags;

	if (GetPenFromObj(interp, graphPtr, objv[i], &penPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	flags = BLT_CONFIG_OBJV_ONLY | (penPtr->flags&(ACTIVE_PEN|NORMAL_PEN));
	if (nOpts == 0) {
	    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		penPtr->configSpecs, (char *)penPtr, (Tcl_Obj *)NULL, flags);
	} else if (nOpts == 1) {
	    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		    penPtr->configSpecs, (char *)penPtr, options[0], flags);
	}
	if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, 
		penPtr->configSpecs, nOpts, options, (char *)penPtr, flags) 
		!= TCL_OK) {
	    break;
	}
	(*penPtr->configProc) (graphPtr, penPtr);
	if (penPtr->refCount > 0) {
	    redraw++;
	}
    }
    if (redraw) {
	graphPtr->flags |= CACHE_DIRTY;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    if (i < nNames) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	Adds a new penstyle to the graph.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Pen *penPtr;

    penPtr = Blt_CreatePen(graphPtr, Tcl_GetString(objv[3]), graphPtr->classId,
	objc - 4, objv + 4);
    if (penPtr == NULL) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Delete the given pen.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is a list of the
 *	graph axis limits.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
DeleteOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    int i;

    for (i = 3; i < objc; i++) {
	Pen *penPtr;

	if (GetPenFromObj(interp, graphPtr, objv[i], &penPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (penPtr->flags & DELETE_PENDING) {
	    Tcl_AppendResult(interp, "can't find pen \"", 
		Tcl_GetString(objv[i]), "\" in \"", 
		Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	penPtr->flags |= DELETE_PENDING;
	if (penPtr->refCount == 0) {
	    DestroyPen(penPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Return a list of the names of all the axes.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->penTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Pen *penPtr;

	    penPtr = Blt_GetHashValue(hPtr);
	    if ((penPtr->flags & DELETE_PENDING) == 0) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewStringObj(penPtr->name, -1));
	    }
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->penTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Pen *penPtr;

	    penPtr = Blt_GetHashValue(hPtr);
	    if ((penPtr->flags & DELETE_PENDING) == 0) {
		int i;

		for (i = 3; i < objc; i++) {
		    char *pattern;

		    pattern = Tcl_GetString(objv[i]);
		    if (Tcl_StringMatch(penPtr->name, pattern)) {
			Tcl_ListObjAppendElement(interp, listObjPtr, 
				Tcl_NewStringObj(penPtr->name, -1));
			break;
		    }
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *	Return the type of pen.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Pen *penPtr;

    if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		     Blt_GraphClassName(penPtr->classId), -1);
    return TCL_OK;
}

static Blt_OpSpec penOps[] =
{
    {"cget", 2, CgetOp, 5, 5, "penName option",},
    {"configure", 2, ConfigureOp, 4, 0,
	"penName ?penName?... ?option value?...",},
    {"create", 2, CreateOp, 4, 0, "penName ?option value?...",},
    {"delete", 2, DeleteOp, 3, 0, "?penName?...",},
    {"names", 1, NamesOp, 3, 0, "?pattern?...",},
    {"type", 1, TypeOp, 4, 4, "penName",},
};
static int nPenOps = sizeof(penOps) / sizeof(Blt_OpSpec);

int
Blt_PenOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    GraphPenProc *proc;

    proc = Blt_GetOpFromObj(interp, nPenOps, penOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (interp, graphPtr, objc, objv);
}
