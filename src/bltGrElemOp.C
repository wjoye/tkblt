
/*
 * bltGrElem.c --
 *
 * This module implements generic elements for the BLT graph widget.
 *
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

#include <assert.h>
#include <X11/Xutil.h>

#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltChain.h"

#define GRAPH_KEY		"BLT Graph Data"

/* Ignore elements that aren't in the display list or have been deleted. */
#define IGNORE_ELEMENT(e) (((e)->link == NULL) || ((e)->flags & DELETE_PENDING))

typedef struct {
    int refCount;
} TableClient;

static Blt_OptionParseProc ObjToAlong;
static Blt_OptionPrintProc AlongToObj;
static Blt_CustomOption alongOption =
{
    ObjToAlong, AlongToObj, NULL, (ClientData)0
};
static Blt_OptionFreeProc FreeValues;
static Blt_OptionParseProc ObjToValues;
static Blt_OptionPrintProc ValuesToObj;
Blt_CustomOption bltValuesOption =
{
    ObjToValues, ValuesToObj, FreeValues, (ClientData)0
};
static Blt_OptionFreeProc FreeValuePairs;
static Blt_OptionParseProc ObjToValuePairs;
static Blt_OptionPrintProc ValuePairsToObj;
Blt_CustomOption bltValuePairsOption =
{
    ObjToValuePairs, ValuePairsToObj, FreeValuePairs, (ClientData)0
};

static Blt_OptionFreeProc  FreeStyles;
static Blt_OptionParseProc ObjToStyles;
static Blt_OptionPrintProc StylesToObj;
Blt_CustomOption bltLineStylesOption =
{
    ObjToStyles, StylesToObj, FreeStyles, (ClientData)0,
};

Blt_CustomOption bltBarStylesOption =
{
    ObjToStyles, StylesToObj, FreeStyles, (ClientData)0,
};

#include "bltGrElem.h"

static Blt_VectorChangedProc VectorChangedProc;

static void FindRange(ElemValues *valuesPtr);
static void FreeDataValues(ElemValues *valuesPtr);
static Tcl_FreeProc FreeElement;

typedef int (GraphElementProc)(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyTableClients --
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
void
Blt_DestroyTableClients(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->dataTables, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	TableClient *clientPtr;

	clientPtr = Blt_GetHashValue(hPtr);
	/*
	if (clientPtr->table != NULL) {
	    Blt_Table_Close(clientPtr->table);
	}
	*/
	free(clientPtr);
    }
    Blt_DeleteHashTable(&graphPtr->dataTables);
}


/*
 *---------------------------------------------------------------------------
 * Custom option parse and print procedures
 *---------------------------------------------------------------------------
 */
static int
GetPenStyleFromObj(
    Tcl_Interp *interp,
    Graph *graphPtr,
    Tcl_Obj *objPtr,
    ClassId classId,
    PenStyle *stylePtr)
{
    Pen *penPtr;
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc != 1) && (objc != 3)) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "bad style entry \"", 
			Tcl_GetString(objPtr), 
			"\": should be \"penName\" or \"penName min max\"", 
			(char *)NULL);
	}
	return TCL_ERROR;
    }
    if (Blt_GetPenFromObj(interp, graphPtr, objv[0], classId, &penPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	double min, max;

	if ((Tcl_GetDoubleFromObj(interp, objv[1], &min) != TCL_OK) ||
	    (Tcl_GetDoubleFromObj(interp, objv[2], &max) != TCL_OK)) {
	    return TCL_ERROR;
	}
	SetWeight(stylePtr->weight, min, max);
    }
    stylePtr->penPtr = penPtr;
    return TCL_OK;
}

static void
FreeVectorSource(ElemValues *valuesPtr)
{
    if (valuesPtr->vectorSource.vector != NULL) { 
	Blt_SetVectorChangedProc(valuesPtr->vectorSource.vector, NULL, NULL);
	Blt_FreeVectorId(valuesPtr->vectorSource.vector); 
	valuesPtr->vectorSource.vector = NULL;
    }
}

static int
FetchVectorValues(Tcl_Interp *interp, ElemValues *valuesPtr, Blt_Vector *vector)
{
    double *array;
    
    if (valuesPtr->values == NULL) {
	array = malloc(Blt_VecLength(vector) * sizeof(double));
    } else {
	array = realloc(valuesPtr->values, 
			    Blt_VecLength(vector) * sizeof(double));
    }
    if (array == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't allocate new vector", (char *)NULL);
	}
	return TCL_ERROR;
    }
    memcpy(array, Blt_VecData(vector), sizeof(double) * Blt_VecLength(vector));
    valuesPtr->min = Blt_VecMin(vector);
    valuesPtr->max = Blt_VecMax(vector);
    valuesPtr->values = array;
    valuesPtr->nValues = Blt_VecLength(vector);
    /* FindRange(valuesPtr); */
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * VectorChangedProc --
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
VectorChangedProc(
    Tcl_Interp *interp, 
    ClientData clientData, 
    Blt_VectorNotify notify)
{
    ElemValues *valuesPtr = clientData;

    if (notify == BLT_VECTOR_NOTIFY_DESTROY) {
	FreeDataValues(valuesPtr);
    } else {
	Blt_Vector *vector;
	
	Blt_GetVectorById(interp, valuesPtr->vectorSource.vector, &vector);
	if (FetchVectorValues(NULL, valuesPtr, vector) != TCL_OK) {
	    return;
	}
    }
    {
	Element *elemPtr = valuesPtr->elemPtr;
	Graph *graphPtr;
	
	graphPtr = elemPtr->obj.graphPtr;
	graphPtr->flags |= RESET_AXES;
	elemPtr->flags |= MAP_ITEM;
	if (!IGNORE_ELEMENT(elemPtr)) {
	    graphPtr->flags |= CACHE_DIRTY;
	    Blt_EventuallyRedrawGraph(graphPtr);
	}
    }
}

static int 
GetVectorData(Tcl_Interp *interp, ElemValues *valuesPtr, const char *vecName)
{
    Blt_Vector *vecPtr;
    VectorDataSource *srcPtr;

    srcPtr = &valuesPtr->vectorSource;
    srcPtr->vector = Blt_AllocVectorId(interp, vecName);
    if (Blt_GetVectorById(interp, srcPtr->vector, &vecPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (FetchVectorValues(interp, valuesPtr, vecPtr) != TCL_OK) {
	FreeVectorSource(valuesPtr);
	return TCL_ERROR;
    }
    Blt_SetVectorChangedProc(srcPtr->vector, VectorChangedProc, valuesPtr);
    valuesPtr->type = ELEM_SOURCE_VECTOR;
    return TCL_OK;
}

static int
ParseValues(Tcl_Interp *interp, Tcl_Obj *objPtr, int *nValuesPtr,
	    double **arrayPtr)
{
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    *arrayPtr = NULL;
    *nValuesPtr = 0;
    if (objc > 0) {
	double *array;
	double *p;
	int i;

	array = malloc(sizeof(double) * objc);
	if (array == NULL) {
	    Tcl_AppendResult(interp, "can't allocate new vector", (char *)NULL);
	    return TCL_ERROR;
	}
	for (p = array, i = 0; i < objc; i++, p++) {
	    if (Blt_ExprDoubleFromObj(interp, objv[i], p) != TCL_OK) {
		free(array);
		return TCL_ERROR;
	    }
	}
	*arrayPtr = array;
	*nValuesPtr = objc;
    }
    return TCL_OK;
}

static void
FreeDataValues(ElemValues *valuesPtr)
{
    switch (valuesPtr->type) {
    case ELEM_SOURCE_VECTOR: 
	FreeVectorSource(valuesPtr);	break;
    case ELEM_SOURCE_VALUES:
					break;
    }
    if (valuesPtr->values != NULL) {
	free(valuesPtr->values);
    }
    valuesPtr->values = NULL;
    valuesPtr->nValues = 0;
    valuesPtr->type = ELEM_SOURCE_VALUES;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindRange --
 *
 *	Find the minimum, positive minimum, and maximum values in a given
 *	vector and store the results in the vector structure.
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Minimum, positive minimum, and maximum values are stored in the
 *	vector.
 *
 *---------------------------------------------------------------------------
 */
static void
FindRange(ElemValues *valuesPtr)
{
    int i;
    double *x;
    double min, max;

    if ((valuesPtr->nValues < 1) || (valuesPtr->values == NULL)) {
	return;			/* This shouldn't ever happen. */
    }
    x = valuesPtr->values;

    min = DBL_MAX, max = -DBL_MAX;
    for(i = 0; i < valuesPtr->nValues; i++) {
	if (isfinite(x[i])) {
	    min = max = x[i];
	    break;
	}
    }
    /*  Initialize values to track the vector range */
    for (/* empty */; i < valuesPtr->nValues; i++) {
	if (isfinite(x[i])) {
	    if (x[i] < min) {
		min = x[i];
	    } else if (x[i] > max) {
		max = x[i];
	    }
	}
    }
    valuesPtr->min = min, valuesPtr->max = max;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FindElemValuesMinimum --
 *
 *	Find the minimum, positive minimum, and maximum values in a given
 *	vector and store the results in the vector structure.
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Minimum, positive minimum, and maximum values are stored in the
 *	vector.
 *
 *---------------------------------------------------------------------------
 */
double
Blt_FindElemValuesMinimum(ElemValues *valuesPtr, double minLimit)
{
    int i;
    double min;

    min = DBL_MAX;
    for (i = 0; i < valuesPtr->nValues; i++) {
	double x;

	x = valuesPtr->values[i];
	if (x < 0.0) {
	    /* What do you do about negative values when using log
	     * scale values seems like a grey area.  Mirror. */
	    x = -x;
	}
	if ((x > minLimit) && (min > x)) {
	    min = x;
	}
    }
    if (min == DBL_MAX) {
	min = minLimit;
    }
    return min;
}

/*ARGSUSED*/
static void
FreeValues(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    ElemValues *valuesPtr = (ElemValues *)(widgRec + offset);

    FreeDataValues(valuesPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToValues --
 *
 *	Given a TCL list of numeric expression representing the element
 *	values, convert into an array of double precision values. In addition,
 *	the minimum and maximum values are saved.  Since elastic values are
 *	allow (values which translate to the min/max of the graph), we must
 *	try to get the non-elastic minimum and maximum.
 *
 * Results:
 *	The return value is a standard TCL result.  The vector is passed
 *	back via the valuesPtr.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToValues(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* TCL list of expressions */
    char *widgRec,		/* Element record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    ElemValues *valuesPtr = (ElemValues *)(widgRec + offset);
    Element *elemPtr = (Element *)widgRec;
    Tcl_Obj **objv;
    int objc;
    int result;
    const char *string;

    valuesPtr->elemPtr = elemPtr;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    elemPtr->flags |= MAP_ITEM;

    /* Release the current data sources. */
    FreeDataValues(valuesPtr);
    if (objc == 0) {
	return TCL_OK;			/* Empty list of values. */
    }
    string = Tcl_GetString(objv[0]);
    if ((objc == 1) && (Blt_VectorExists2(interp, string))) {
	result = GetVectorData(interp, valuesPtr, string);
    } else {
	double *values;
	int nValues;

	result = ParseValues(interp, objPtr, &nValues, &values);
	if (result != TCL_OK) {
	    return TCL_ERROR;		/* Can't parse the values as numbers. */
	}
	FreeDataValues(valuesPtr);
	if (nValues > 0) {
	    valuesPtr->values = values;
	}
	valuesPtr->nValues = nValues;
	FindRange(valuesPtr);
	valuesPtr->type = ELEM_SOURCE_VALUES;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValuesToObj --
 *
 *	Convert the vector of floating point values into a TCL list.
 *
 * Results:
 *	The string representation of the vector is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ValuesToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Element record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    ElemValues *valuesPtr = (ElemValues *)(widgRec + offset);

    switch (valuesPtr->type) {
    case ELEM_SOURCE_VECTOR:
	{
	    const char *vecName;
	    
	    vecName = Blt_NameOfVectorId(valuesPtr->vectorSource.vector);
	    return Tcl_NewStringObj(vecName, -1);
	}
    case ELEM_SOURCE_VALUES:
	{
	    Tcl_Obj *listObjPtr;
	    double *vp, *vend; 
	    
	    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    for (vp = valuesPtr->values, vend = vp + valuesPtr->nValues; 
		 vp < vend; vp++) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
					 Tcl_NewDoubleObj(*vp));
	    }
	    return listObjPtr;
	}
    default:
	abort();
    }
    return Tcl_NewStringObj("", 0);
}

/*ARGSUSED*/
static void
FreeValuePairs(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)			/* Not used. */
{
    Element *elemPtr = (Element *)widgRec;

    FreeDataValues(&elemPtr->x);
    FreeDataValues(&elemPtr->y);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToValuePairs --
 *
 *	This procedure is like ObjToValues except that it interprets
 *	the list of numeric expressions as X Y coordinate pairs.  The
 *	minimum and maximum for both the X and Y vectors are
 *	determined.
 *
 * Results:
 *	The return value is a standard TCL result.  The vectors are
 *	passed back via the widget record (elemPtr).
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToValuePairs(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* TCL list of numeric expressions */
    char *widgRec,		/* Element record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    Element *elemPtr = (Element *)widgRec;
    double *values;
    int nValues;
    size_t newSize;

    if (ParseValues(interp, objPtr, &nValues, &values) != TCL_OK) {
	return TCL_ERROR;
    }
    if (nValues & 1) {
	Tcl_AppendResult(interp, "odd number of data points", (char *)NULL);
	free(values);
	return TCL_ERROR;
    }
    nValues /= 2;
    newSize = nValues * sizeof(double);
    FreeDataValues(&elemPtr->x);	/* Release the current data sources. */
    FreeDataValues(&elemPtr->y);
    if (newSize > 0) {
	double *p;
	int i;

	elemPtr->x.values = malloc(newSize);
	elemPtr->y.values = malloc(newSize);
	elemPtr->x.nValues = elemPtr->y.nValues = nValues;
	for (p = values, i = 0; i < nValues; i++) {
	    elemPtr->x.values[i] = *p++;
	    elemPtr->y.values[i] = *p++;
	}
	free(values);
	FindRange(&elemPtr->x);
	FindRange(&elemPtr->y);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValuePairsToObj --
 *
 *	Convert pairs of floating point values in the X and Y arrays
 *	into a TCL list.
 *
 * Results:
 *	The return value is a string (Tcl list).
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ValuePairsToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Element information record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    Element *elemPtr = (Element *)widgRec;
    Tcl_Obj *listObjPtr;
    int i;
    int length;

    length = NUMBEROFPOINTS(elemPtr);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < length; i++) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewDoubleObj(elemPtr->x.values[i]));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewDoubleObj(elemPtr->y.values[i]));
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToAlong --
 *
 *	Given a TCL list of numeric expression representing the element
 *	values, convert into an array of double precision values. In
 *	addition, the minimum and maximum values are saved.  Since
 *	elastic values are allow (values which translate to the
 *	min/max of the graph), we must try to get the non-elastic
 *	minimum and maximum.
 *
 * Results:
 *	The return value is a standard TCL result.  The vector is passed
 *	back via the valuesPtr.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToAlong(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representation of value. */
    char *widgRec,		/* Widget record. */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    int *intPtr = (int *)(widgRec + offset);
    char *string;

    string = Tcl_GetString(objPtr);
    if ((string[0] == 'x') && (string[1] == '\0')) {
	*intPtr = SEARCH_X;
    } else if ((string[0] == 'y') && (string[1] == '\0')) { 
	*intPtr = SEARCH_Y;
    } else if ((string[0] == 'b') && (strcmp(string, "both") == 0)) {
	*intPtr = SEARCH_BOTH;
    } else {
	Tcl_AppendResult(interp, "bad along value \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AlongToObj --
 *
 *	Convert the vector of floating point values into a TCL list.
 *
 * Results:
 *	The string representation of the vector is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
AlongToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    int along = *(int *)(widgRec + offset);
    Tcl_Obj *objPtr;

    switch (along) {
    case SEARCH_X:
	objPtr = Tcl_NewStringObj("x", 1);
	break;
    case SEARCH_Y:
	objPtr = Tcl_NewStringObj("y", 1);
	break;
    case SEARCH_BOTH:
	objPtr = Tcl_NewStringObj("both", 4);
	break;
    default:
	objPtr = Tcl_NewStringObj("unknown along value", 4);
	break;
    }
    return objPtr;
}

void
Blt_FreeStylePalette(Blt_Chain stylePalette)
{
    Blt_ChainLink link;

    /* Skip the first slot. It contains the built-in "normal" pen of
     * the element.  */
    link = Blt_Chain_FirstLink(stylePalette);
    if (link != NULL) {
	Blt_ChainLink next;

	for (link = Blt_Chain_NextLink(link); link != NULL; link = next) {
	    PenStyle *stylePtr;

	    next = Blt_Chain_NextLink(link);
	    stylePtr = Blt_Chain_GetValue(link);
	    Blt_FreePen(stylePtr->penPtr);
	    Blt_Chain_DeleteLink(stylePalette, link);
	}
    }
}

/*ARGSUSED*/
static void
FreeStyles(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    Blt_Chain stylePalette = *(Blt_Chain *)(widgRec + offset);

    Blt_FreeStylePalette(stylePalette);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ObjToStyles --
 *
 *	Parse the list of style names.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStyles(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing style list */
    char *widgRec,		/* Element information record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    Blt_Chain stylePalette = *(Blt_Chain *)(widgRec + offset);
    Blt_ChainLink link;
    Element *elemPtr = (Element *)(widgRec);
    PenStyle *stylePtr;
    Tcl_Obj **objv;
    int objc;
    int i;
    size_t size = (size_t)clientData;


    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Reserve the first entry for the "normal" pen. We'll set the
     * style later */
    Blt_FreeStylePalette(stylePalette);
    link = Blt_Chain_FirstLink(stylePalette);
    if (link == NULL) {
	link = Blt_Chain_AllocLink(size);
	Blt_Chain_LinkAfter(stylePalette, link, NULL);
    }
    stylePtr = Blt_Chain_GetValue(link);
    stylePtr->penPtr = elemPtr->normalPenPtr;
    for (i = 0; i < objc; i++) {
	link = Blt_Chain_AllocLink(size);
	stylePtr = Blt_Chain_GetValue(link);
	stylePtr->weight.min = (double)i;
	stylePtr->weight.max = (double)i + 1.0;
	stylePtr->weight.range = 1.0;
	if (GetPenStyleFromObj(interp, elemPtr->obj.graphPtr, objv[i], 
		elemPtr->obj.classId, (PenStyle *)stylePtr) != TCL_OK) {
	    Blt_FreeStylePalette(stylePalette);
	    return TCL_ERROR;
	}
	Blt_Chain_LinkAfter(stylePalette, link, NULL);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StylesToObj --
 *
 *	Convert the style information into a Tcl_Obj.
 *
 * Results:
 *	The string representing the style information is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StylesToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Element information record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    Blt_Chain stylePalette = *(Blt_Chain *)(widgRec + offset);
    Blt_ChainLink link;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    link = Blt_Chain_FirstLink(stylePalette);
    if (link != NULL) {
	/* Skip the first style (it's the default) */
	for (link = Blt_Chain_NextLink(link); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    PenStyle *stylePtr;
	    Tcl_Obj *subListObjPtr;

	    stylePtr = Blt_Chain_GetValue(link);
	    subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    Tcl_ListObjAppendElement(interp, subListObjPtr, 
		Tcl_NewStringObj(stylePtr->penPtr->name, -1));
	    Tcl_ListObjAppendElement(interp, subListObjPtr, 
				     Tcl_NewDoubleObj(stylePtr->weight.min));
	    Tcl_ListObjAppendElement(interp, subListObjPtr, 
				     Tcl_NewDoubleObj(stylePtr->weight.max));
	    Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);
	}
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_StyleMap --
 *
 *	Creates an array of style indices and fills it based on the weight
 *	of each data point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed and allocated for the index array.
 *
 *---------------------------------------------------------------------------
 */

PenStyle **
Blt_StyleMap(Element *elemPtr)
{
    int i;
    int nWeights;		/* Number of weights to be examined.
				 * If there are more data points than
				 * weights, they will default to the
				 * normal pen. */

    PenStyle **dataToStyle;	/* Directory of styles.  Each array
				 * element represents the style for
				 * the data point at that index */
    Blt_ChainLink link;
    PenStyle *stylePtr;
    double *w;			/* Weight vector */
    int nPoints;

    nPoints = NUMBEROFPOINTS(elemPtr);
    nWeights = MIN(elemPtr->w.nValues, nPoints);
    w = elemPtr->w.values;
    link = Blt_Chain_FirstLink(elemPtr->stylePalette);
    stylePtr = Blt_Chain_GetValue(link);

    /* 
     * Create a style mapping array (data point index to style), 
     * initialized to the default style.
     */
    dataToStyle = malloc(nPoints * sizeof(PenStyle *));
    for (i = 0; i < nPoints; i++) {
	dataToStyle[i] = stylePtr;
    }

    for (i = 0; i < nWeights; i++) {
	for (link = Blt_Chain_LastLink(elemPtr->stylePalette); link != NULL; 
	     link = Blt_Chain_PrevLink(link)) {
	    stylePtr = Blt_Chain_GetValue(link);

	    if (stylePtr->weight.range > 0.0) {
		double norm;

		norm = (w[i] - stylePtr->weight.min) / stylePtr->weight.range;
		if (((norm - 1.0) <= DBL_EPSILON) && 
		    (((1.0 - norm) - 1.0) <= DBL_EPSILON)) {
		    dataToStyle[i] = stylePtr;
		    break;		/* Done: found range that matches. */
		}
	    }
	}
    }
    return dataToStyle;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetIndex --
 *
 *	Given a string representing the index of a pair of x,y
 *	coordinates, return the numeric index.
 *
 * Results:
 *     	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIndex(Tcl_Interp *interp, Element *elemPtr, Tcl_Obj *objPtr, int *indexPtr)
{
    char *string;

    string = Tcl_GetString(objPtr);
    if ((*string == 'e') && (strcmp("end", string) == 0)) {
	*indexPtr = NUMBEROFPOINTS(elemPtr) - 1;
    } else if (Blt_ExprIntFromObj(interp, objPtr, indexPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetElement --
 *
 *	Find the element represented the given name, returning a pointer to
 *	its data structure via elemPtrPtr.
 *
 * Results:
 *     	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetElement(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
	       Element **elemPtrPtr)
{
    Blt_HashEntry *hPtr;
    char *name;

    name = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->elements.table, name);
    if (hPtr == NULL) {
	if (interp != NULL) {
 	    Tcl_AppendResult(interp, "can't find element \"", name,
		"\" in \"", Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *elemPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyElement --
 *
 *	Add a new element to the graph.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyElement(Element *elemPtr)
{
    Graph *graphPtr = elemPtr->obj.graphPtr;

    Blt_DeleteBindings(graphPtr->bindTable, elemPtr);
    Blt_Legend_RemoveElement(graphPtr, elemPtr);

    Blt_FreeOptions(elemPtr->configSpecs, (char *)elemPtr,graphPtr->display, 0);
    /*
     * Call the element's own destructor to release the memory and
     * resources allocated for it.
     */
    (*elemPtr->procsPtr->destroyProc) (graphPtr, elemPtr);

    /* Remove it also from the element display list */
    if (elemPtr->link != NULL) {
	Blt_Chain_DeleteLink(graphPtr->elements.displayList, elemPtr->link);
	if (!IGNORE_ELEMENT(elemPtr)) {
	    graphPtr->flags |= RESET_WORLD;
	    Blt_EventuallyRedrawGraph(graphPtr);
	}
    }
    /* Remove the element for the graph's hash table of elements */
    if (elemPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&graphPtr->elements.table, elemPtr->hashPtr);
    }
    if (elemPtr->obj.name != NULL) {
      free((void*)(elemPtr->obj.name));
    }
    if (elemPtr->label != NULL) {
      free((void*)(elemPtr->label));
    }
    free(elemPtr);
}

static void FreeElement(char* data)
{
    Element *elemPtr = (Element *)data;
    DestroyElement(elemPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateElement --
 *
 *	Add a new element to the graph.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateElement(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv, ClassId classId)
{
    Element *elemPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '-') {
	Tcl_AppendResult(graphPtr->interp, "name of element \"", string, 
			 "\" can't start with a '-'", (char *)NULL);
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&graphPtr->elements.table, string, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "element \"", string, 
			 "\" already exists in \"", Tcl_GetString(objv[0]), 
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (classId == CID_ELEM_BAR) {
	elemPtr = Blt_BarElement(graphPtr, string, classId);
    } else { 
	/* Stripcharts are line graphs with some options enabled. */	
	elemPtr = Blt_LineElement(graphPtr, string, classId);
    }
    assert(elemPtr->configSpecs != NULL);
    elemPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, elemPtr);

    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, 
	elemPtr->obj.name, "Element", elemPtr->configSpecs, objc - 4, objv + 4,
	(char *)elemPtr, 0) != TCL_OK) {
	DestroyElement(elemPtr);
	return TCL_ERROR;
    }
    (*elemPtr->procsPtr->configProc) (graphPtr, elemPtr);
    elemPtr->link = Blt_Chain_Append(graphPtr->elements.displayList, elemPtr);
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    elemPtr->flags |= MAP_ITEM;
    graphPtr->flags |= RESET_AXES;
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyElements --
 *
 *	Removes all the graph's elements. This routine is called when
 *	the graph is destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated for the graph's elements is freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyElements(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Element *elemPtr;

    for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.table, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	elemPtr = Blt_GetHashValue(hPtr);
	elemPtr->hashPtr = NULL;
	DestroyElement(elemPtr);
    }
    Blt_DeleteHashTable(&graphPtr->elements.table);
    Blt_DeleteHashTable(&graphPtr->elements.tagTable);
    Blt_Chain_Destroy(graphPtr->elements.displayList);
}

void
Blt_ConfigureElements(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	(*elemPtr->procsPtr->configProc) (graphPtr, elemPtr);
    }
}

void
Blt_MapElements(Graph *graphPtr)
{
    Blt_ChainLink link;

    if (graphPtr->mode != BARS_INFRONT) {
	Blt_ResetBarGroups(graphPtr);
    }
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (IGNORE_ELEMENT(elemPtr)) {
	    continue;
	}
	if ((graphPtr->flags & MAP_ALL) || (elemPtr->flags & MAP_ITEM)) {
	    (*elemPtr->procsPtr->mapProc) (graphPtr, elemPtr);
	    elemPtr->flags &= ~MAP_ITEM;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawElements --
 *
 *	Calls the individual element drawing routines for each
 *	element.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Elements are drawn into the drawable (pixmap) which will
 *	eventually be displayed in the graph window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawElements(Graph *graphPtr, Drawable drawable)
{
    Blt_ChainLink link;

    /* Draw with respect to the stacking order. */
    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & (HIDE|DELETE_PENDING)) == 0) {
	    (*elemPtr->procsPtr->drawNormalProc)(graphPtr, drawable, elemPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawActiveElements --
 *
 *	Calls the individual element drawing routines to display
 *	the active colors for each element.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Elements are drawn into the drawable (pixmap) which will
 *	eventually be displayed in the graph window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawActiveElements(Graph *graphPtr, Drawable drawable)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & (HIDE|ACTIVE|DELETE_PENDING)) == ACTIVE) {
	    (*elemPtr->procsPtr->drawActiveProc)(graphPtr, drawable, elemPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ElementsToPostScript --
 *
 *	Generates PostScript output for each graph element in the
 *	element display list.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ElementsToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->flags & (HIDE|DELETE_PENDING)) {
	    continue;
	}
	/* Comment the PostScript to indicate the start of the element */
	Blt_Ps_Format(ps, "\n%% Element \"%s\"\n\n", elemPtr->obj.name);
	(*elemPtr->procsPtr->printNormalProc) (graphPtr, ps, elemPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ActiveElementsToPostScript --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ActiveElementsToPostScript( Graph *graphPtr, Blt_Ps ps)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & (DELETE_PENDING|HIDE|ACTIVE)) == ACTIVE) {
	    Blt_Ps_Format(ps, "\n%% Active Element \"%s\"\n\n", 
		elemPtr->obj.name);
	    (*elemPtr->procsPtr->printActiveProc)(graphPtr, ps, elemPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *	Marks data points of elements (given by their index) as active.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Interpreter to report errors to */
    int objc,				/* Number of element names */
    Tcl_Obj *const *objv)		/* List of element names */
{
    Element *elemPtr;
    int i;
    int *indices;
    int nIndices;

    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* List all the currently active elements */
	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.table, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    elemPtr = Blt_GetHashValue(hPtr);
	    if (elemPtr->flags & ACTIVE) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewStringObj(elemPtr->obj.name, -1));
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    elemPtr->flags |= ACTIVE | ACTIVE_PENDING;

    indices = NULL;
    nIndices = -1;
    if (objc > 4) {
	int *activePtr;

	nIndices = objc - 4;
	activePtr = indices = malloc(sizeof(int) * nIndices);
	for (i = 4; i < objc; i++) {
	    if (GetIndex(interp, elemPtr, objv[i], activePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    activePtr++;
	}
    }
    if (elemPtr->activeIndices != NULL) {
	free(elemPtr->activeIndices);
    }
    elemPtr->nActiveIndices = nIndices;
    elemPtr->activeIndices = indices;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

ClientData
Blt_MakeElementTag(Graph *graphPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&graphPtr->elements.tagTable, tagName, &isNew);
    return Blt_GetHashKey(&graphPtr->elements.tagTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	.g element bind elemName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	char *tagName;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.tagTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    tagName = Blt_GetHashKey(&graphPtr->elements.tagTable, hPtr);
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
				     Tcl_NewStringObj(tagName, -1));
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
	Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), 
	objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	Add a new element to the graph (using the default type of the
 *	graph).
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv,
    ClassId classId)
{
    return CreateElement(graphPtr, interp, objc, objv, classId);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Element *elemPtr;

    if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, elemPtr->configSpecs,
				  (char *)elemPtr, objv[4], 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClosestOp --
 *
 *	Find the element closest to the specified screen coordinates.
 *	Options:
 *	-halo		Consider points only with this maximum distance
 *			from the picked coordinate.
 *	-interpolate	Find closest point along element traces, not just
 *			data points.
 *	-along
 *
 * Results:
 *	A standard TCL result. If an element could be found within
 *	the halo distance, the interpreter result is "1", otherwise
 *	"0".  If a closest element exists, the designated TCL array
 *	variable will be set with the following information:
 *
 *	1) the element name,
 *	2) the index of the closest point,
 *	3) the distance (in screen coordinates) from the picked X-Y
 *	   coordinate and the closest point,
 *	4) the X coordinate (graph coordinate) of the closest point,
 *	5) and the Y-coordinate.
 *
 *---------------------------------------------------------------------------
 */

static Blt_ConfigSpec closestSpecs[] = {
    {BLT_CONFIG_PIXELS_NNEG, "-halo", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(ClosestSearch, halo), 0},
    {BLT_CONFIG_BOOLEAN, "-interpolate", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(ClosestSearch, mode), 0 }, 
    {BLT_CONFIG_CUSTOM, "-along", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(ClosestSearch, along), 0, &alongOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static int
ClosestOp(
    Graph *graphPtr,		/* Graph widget */
    Tcl_Interp *interp,		/* Interpreter to report results to */
    int objc,			/* Number of element names */
    Tcl_Obj *const *objv)	/* List of element names */
{
    Element *elemPtr;
    ClosestSearch search;
    int i, x, y;
    char *string;

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
	Tcl_AppendResult(interp, ": bad window x-coordinate", (char *)NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	Tcl_AppendResult(interp, ": bad window y-coordinate", (char *)NULL);
	return TCL_ERROR;
    }
    for (i = 5; i < objc; i += 2) {	/* Count switches-value pairs */
	string = Tcl_GetString(objv[i]);
	if ((string[0] != '-') || 
	    ((string[1] == '-') && (string[2] == '\0'))) {
	    break;
	}
    }
    if (i > objc) {
	i = objc;
    }

    search.mode = SEARCH_POINTS;
    search.halo = graphPtr->halo;
    search.index = -1;
    search.along = SEARCH_BOTH;
    search.x = x;
    search.y = y;

    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, closestSpecs, i - 5,
	objv + 5, (char *)&search, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;	/* Error occurred processing an option. */
    }
    if (i < objc) {
	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    i++;			/* Skip "--" */
	}
    }
    search.dist = (double)(search.halo + 1);

    if (i < objc) {
	for ( /* empty */ ; i < objc; i++) {
	    if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
		return TCL_ERROR; /* Can't find named element */
	    }
	    if (IGNORE_ELEMENT(elemPtr)) {
		continue;
	    }
	    if (elemPtr->flags & (HIDE|MAP_ITEM)) {
		continue;
	    }
	    (*elemPtr->procsPtr->closestProc) (graphPtr, elemPtr, &search);
	}
    } else {
	Blt_ChainLink link;

	/* 
	 * Find the closest point from the set of displayed elements,
	 * searching the display list from back to front.  That way if
	 * the points from two different elements overlay each other
	 * exactly, the last one picked will be the topmost.  
	 */
	for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	     link != NULL; link = Blt_Chain_PrevLink(link)) {
	    elemPtr = Blt_Chain_GetValue(link);
	    if (elemPtr->flags & (HIDE|MAP_ITEM|DELETE_PENDING)) {
		continue;
	    }
	    (*elemPtr->procsPtr->closestProc) (graphPtr, elemPtr, &search);
	}
    }
    if (search.dist < (double)search.halo) {
	Tcl_Obj *listObjPtr;
	/*
	 *  Return a list of name value pairs.
	 */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewStringObj("name", -1));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(search.elemPtr->obj.name, -1)); 
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj("index", -1));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewIntObj(search.index));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj("x", -1));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(search.point.x));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj("y", -1));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(search.point.y));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj("dist", -1));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(search.dist));
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Sets the element specifications by the given the command line
 *	arguments and calls the element specification configuration
 *	routine. If zero or one command line options are given, only
 *	information about the option(s) is returned in interp->result.
 *	If the element configuration has changed and the element is
 *	currently displayed, the axis limits are updated and
 *	recomputed.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new display list.
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
    int nNames, nOpts;
    Tcl_Obj *const *options;
    int i;

    /* Figure out where the option value pairs begin */
    objc -= 3;
    objv += 3;
    for (i = 0; i < objc; i++) {
	Element *elemPtr;
	char *string;

	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
	if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Can't find named element */
	}
    }
    nNames = i;			/* Number of element names specified */
    nOpts = objc - i;		/* Number of options specified */
    options = objv + nNames;	/* Start of options in objv  */

    for (i = 0; i < nNames; i++) {
	Element *elemPtr;
	int flags;

	if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	flags = BLT_CONFIG_OBJV_ONLY;
	if (nOpts == 0) {
	    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		elemPtr->configSpecs, (char *)elemPtr, (Tcl_Obj *)NULL, flags);
	} else if (nOpts == 1) {
	    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		elemPtr->configSpecs, (char *)elemPtr, options[0], flags);
	}
	if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, 
		elemPtr->configSpecs, nOpts, options, (char *)elemPtr, flags) 
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	if ((*elemPtr->procsPtr->configProc) (graphPtr, elemPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Failed to configure element */
	}
	if (Blt_ConfigModified(elemPtr->configSpecs, "-hide", (char *)NULL)) {
	    graphPtr->flags |= RESET_AXES;
	    elemPtr->flags |= MAP_ITEM;
	}
	/* If data points or axes have changed, reset the axes (may
	 * affect autoscaling) and recalculate the screen points of
	 * the element. */

	if (Blt_ConfigModified(elemPtr->configSpecs, "-*data", "-map*", "-x",
		"-y", (char *)NULL)) {
	    graphPtr->flags |= RESET_WORLD;
	    elemPtr->flags |= MAP_ITEM;
	}
	/* The new label may change the size of the legend */
	if (Blt_ConfigModified(elemPtr->configSpecs, "-label", (char *)NULL)) {
	    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
	}
    }
    /* Update the pixmap if any configuration option changed */
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeactivateOp --
 *
 *	Clears the active bit for the named elements.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeactivateOp(
    Graph *graphPtr,		/* Graph widget */
    Tcl_Interp *interp,		/* Not used. */
    int objc,			/* Number of element names */
    Tcl_Obj *const *objv)	/* List of element names */
{
    int i;

    for (i = 3; i < objc; i++) {
	Element *elemPtr;

	if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Can't find named element */
	}
	elemPtr->flags &= ~(ACTIVE | ACTIVE_PENDING);
	if (elemPtr->activeIndices != NULL) {
	    free(elemPtr->activeIndices);
	    elemPtr->activeIndices = NULL;
	}
	elemPtr->nActiveIndices = 0;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Delete the named elements from the graph.
 *
 * Results:
 *	TCL_ERROR is returned if any of the named elements can not be
 *	found.  Otherwise TCL_OK is returned;
 *
 * Side Effects:
 *	If the element is currently displayed, the plotting area of
 *	the graph is redrawn. Memory and resources allocated by the
 *	elements are released.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(
    Graph *graphPtr,		/* Graph widget */
    Tcl_Interp *interp,		/* Not used. */
    int objc,			/* Number of element names */
    Tcl_Obj *const *objv)	/* List of element names */
{
    int i;

    for (i = 3; i < objc; i++) {
	Element *elemPtr;

	if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Can't find named element */
	}
	elemPtr->flags |= DELETE_PENDING;
	Tcl_EventuallyFree(elemPtr, FreeElement);
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *	Indicates if the named element exists in the graph.
 *
 * Results:
 *	The return value is a standard TCL result.  The interpreter
 *	result will contain "1" or "0".
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ExistsOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&graphPtr->elements.table, Tcl_GetString(objv[3]));
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 * 	Returns the name of the picked element (using the element
 *	bind operation).  Right now, the only name accepted is
 *	"current".
 *
 * Results:
 *	A standard TCL result.  The interpreter result will contain
 *	the name of the element.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    char *string;

    string = Tcl_GetString(objv[3]);
    if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
	Element *elemPtr;

	elemPtr = Blt_GetCurrentItem(graphPtr->bindTable);
	/* Report only on elements. */
	if ((elemPtr != NULL) && ((elemPtr->flags & DELETE_PENDING) == 0) &&
	    (elemPtr->obj.classId >= CID_ELEM_BAR) &&
	    (elemPtr->obj.classId <= CID_ELEM_STRIP)) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name,-1);
	}
    }
    return TCL_OK;
}

static Tcl_Obj *
DisplayListObj(Graph *graphPtr)
{
    Tcl_Obj *listObjPtr;
    Blt_ChainLink link;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;
	Tcl_Obj *objPtr;

	elemPtr = Blt_Chain_GetValue(link);
	objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	Tcl_ListObjAppendElement(graphPtr->interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LowerOp --
 *
 *	Lowers the named elements to the bottom of the display list.
 *
 * Results:
 *	A standard TCL result. The interpreter result will contain the new
 *	display list of element names.
 *
 *	.g element lower elem ?elem...?
 *
 *---------------------------------------------------------------------------
 */
static int
LowerOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Chain chain;
    Blt_ChainLink link, next;
    int i;

    /* Move the links of lowered elements out of the display list into
     * a temporary list. */
    chain = Blt_Chain_Create();
    for (i = 3; i < objc; i++) {
	Element *elemPtr;

	if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Can't find named element */
	}
	Blt_Chain_UnlinkLink(graphPtr->elements.displayList, elemPtr->link); 
	Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
    }
    /* Append the links to end of the display list. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
	next = Blt_Chain_NextLink(link);
	Blt_Chain_UnlinkLink(chain, link); 
	Blt_Chain_LinkAfter(graphPtr->elements.displayList, link, NULL); 
    }	
    Blt_Chain_Destroy(chain);
    Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns the names of the elements is the graph matching
 *	one of more patterns provided.  If no pattern arguments
 *	are given, then all element names will be returned.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.table, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Element *elemPtr;
	    Tcl_Obj *objPtr;

	    elemPtr = Blt_GetHashValue(hPtr);
	    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.table, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Element *elemPtr;
	    int i;

	    elemPtr = Blt_GetHashValue(hPtr);
	    for (i = 3; i < objc; i++) {
		if (Tcl_StringMatch(elemPtr->obj.name,Tcl_GetString(objv[i]))) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
		    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		    break;
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
 * RaiseOp --
 *
 *	Reset the element within the display list.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain the new display list of element names.
 *
 *	.g element raise ?elem...?
 *
 *---------------------------------------------------------------------------
 */
static int
RaiseOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Chain chain;
    Blt_ChainLink link, prev;
    int i;

    /* Move the links of lowered elements out of the display list into
     * a temporary list. */
    chain = Blt_Chain_Create();
    for (i = 3; i < objc; i++) {
	Element *elemPtr;

	if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Can't find named element */
	}
	Blt_Chain_UnlinkLink(graphPtr->elements.displayList, elemPtr->link); 
	Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
    }
    /* Prepend the links to beginning of the display list in reverse order. */
    for (link = Blt_Chain_LastLink(chain); link != NULL; link = prev) {
	prev = Blt_Chain_PrevLink(link);
	Blt_Chain_UnlinkLink(chain, link); 
	Blt_Chain_LinkBefore(graphPtr->elements.displayList, link, NULL); 
    }	
    Blt_Chain_Destroy(chain);
    Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShowOp --
 *
 *	Queries or resets the element display list.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain the new display list of element names.
 *
 *---------------------------------------------------------------------------
 */
static int
ShowOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    if (objc == 4) {
	Blt_Chain chain;
	Blt_ChainLink link;
	Tcl_Obj **elem;
	int i, n;

	if (Tcl_ListObjGetElements(interp, objv[3], &n, &elem) != TCL_OK) {
	    return TCL_ERROR;
	}
	/* Collect the named elements into a list. */
	chain = Blt_Chain_Create();
	for (i = 0; i < n; i++) {
	    Element *elemPtr;	/* Element information record */

	    if (Blt_GetElement(interp, graphPtr, elem[i], &elemPtr) != TCL_OK) {
		Blt_Chain_Destroy(chain);
		return TCL_ERROR;
	    }
	    Blt_Chain_Append(chain, elemPtr);
	}
	/* Clear the links from the currently displayed elements.  */
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    elemPtr->link = NULL;
	}
	Blt_Chain_Destroy(graphPtr->elements.displayList);
	graphPtr->elements.displayList = chain;
	/* Set links on all the displayed elements.  */
	for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    elemPtr->link = link;
	}
	graphPtr->flags |= RESET_WORLD;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *	Returns the name of the type of the element given by some
 *	element name.
 *
 * Results:
 *	A standard TCL result. Returns the type of the element in
 *	interp->result. If the identifier given doesn't represent an
 *	element, then an error message is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(
    Graph *graphPtr,		/* Graph widget */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)	/* Element name */
{
    Element *elemPtr;
    const char *string;

    if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    switch (elemPtr->obj.classId) {
    case CID_ELEM_BAR:		string = "bar";		break;
    case CID_ELEM_CONTOUR:	string = "contour";	break;
    case CID_ELEM_LINE:		string = "line";	break;
    case CID_ELEM_STRIP:	string = "strip";	break;
    default:			string = "???";		break;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    return TCL_OK;
}

/*
 * Global routines:
 */
static Blt_OpSpec elemOps[] = {
    {"activate",   1, ActivateOp,   3, 0, "?elemName? ?index...?",},
    {"bind",       1, BindOp,       3, 6, "elemName sequence command",},
    {"cget",       2, CgetOp,       5, 5, "elemName option",},
    {"closest",    2, ClosestOp,    5, 0,
	"x y ?option value?... ?elemName?...",},
    {"configure",  2, ConfigureOp,  4, 0,
	"elemName ?elemName?... ?option value?...",},
    {"create",     2, CreateOp,     4, 0, "elemName ?option value?...",},
    {"deactivate", 3, DeactivateOp, 3, 0, "?elemName?...",},
    {"delete",     3, DeleteOp,     3, 0, "?elemName?...",},
    {"exists",     1, ExistsOp,     4, 4, "elemName",},
    {"get",        1, GetOp,        4, 4, "name",},
    {"lower",      1, LowerOp,      3, 0, "?elemName?...",},
    {"names",      1, NamesOp,      3, 0, "?pattern?...",},
    {"raise",      1, RaiseOp,      3, 0, "?elemName?...",},
    {"show",       1, ShowOp,       3, 4, "?elemList?",},
    {"type",       1, TypeOp,       4, 4, "elemName",},
};
static int numElemOps = sizeof(elemOps) / sizeof(Blt_OpSpec);


/*
 *---------------------------------------------------------------------------
 *
 * Blt_ElementOp --
 *
 *	This procedure is invoked to process the TCL command that
 *	corresponds to a widget managed by this module.  See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ElementOp(
    Graph *graphPtr,		/* Graph widget record */
    Tcl_Interp *interp,
    int objc,			/* # arguments */
    Tcl_Obj *const *objv,	/* Argument list */
    ClassId classId)
{
    void *ptr;
    int result;

    ptr = Blt_GetOpFromObj(interp, numElemOps, elemOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (ptr == NULL) {
	return TCL_ERROR;
    }
    if (ptr == CreateOp) {
	result = CreateOp(graphPtr, interp, objc, objv, classId);
    } else {
	GraphElementProc *proc;
	
	proc = ptr;
	result = (*proc) (graphPtr, interp, objc, objv);
    }
    return result;
}
