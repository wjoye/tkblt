/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltVector.c --
 *
 * This module implements vector data objects.
 *
 *	Copyright 1995-2004 George A Howlett.
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

/*
 * TODO:
 *	o Add H. Kirsch's vector binary read operation
 *		x binread file0
 *		x binread -file file0
 *
 *	o Add ASCII/binary file reader
 *		x read fileName
 *
 *	o Allow Tcl-based client notifications.
 *		vector x
 *		x notify call Display
 *		x notify delete Display
 *		x notify reorder #1 #2
 */

#include <assert.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "bltInt.h"
#include "bltVecInt.h"
#include "bltOp.h"
#include "bltNsUtil.h"
#include "bltSwitch.h"

#define DEF_ARRAY_SIZE		64
#define TRACE_ALL  (TCL_TRACE_WRITES | TCL_TRACE_READS | TCL_TRACE_UNSETS)


#define VECTOR_CHAR(c)	((isalnum((unsigned char)(c))) || \
	(c == '_') || (c == ':') || (c == '@') || (c == '.'))

/*
 * VectorClient --
 *
 *	A vector can be shared by several clients.  Each client allocates this
 *	structure that acts as its key for using the vector.  Clients can also
 *	designate a callback routine that is executed whenever the vector is
 *	updated or destroyed.
 *
 */
typedef struct {
    unsigned int magic;		/* Magic value designating whether this really
				 * is a vector token or not */

    Vector *serverPtr;		/* Pointer to the master record of the vector.
				 * If NULL, indicates that the vector has been
				 * destroyed but as of yet, this client hasn't
				 * recognized it. */

    Blt_VectorChangedProc *proc;/* Routine to call when the contents of the
				 * vector change or the vector is deleted. */

    ClientData clientData;	/* Data passed whenever the vector change
				 * procedure is called. */

    Blt_ChainLink link;		/* Used to quickly remove this entry from its
				 * server's client chain. */
} VectorClient;

static Tcl_CmdDeleteProc VectorInstDeleteProc;
static Tcl_ObjCmdProc VectorCmd;
static Tcl_InterpDeleteProc VectorInterpDeleteProc;

typedef struct {
    char *varName;		/* Requested variable name. */
    char *cmdName;		/* Requested command name. */
    int flush;			/* Flush */
    int watchUnset;		/* Watch when variable is unset. */
} CreateSwitches;

static Blt_SwitchSpec createSwitches[] = 
{
    {BLT_SWITCH_STRING, "-variable", "varName",
	Tk_Offset(CreateSwitches, varName), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-command", "command",
	Tk_Offset(CreateSwitches, cmdName), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_BOOLEAN, "-watchunset", "bool",
	Tk_Offset(CreateSwitches, watchUnset), 0},
    {BLT_SWITCH_BOOLEAN, "-flush", "bool",
	Tk_Offset(CreateSwitches, flush), 0},
    {BLT_SWITCH_END}
};

typedef int (VectorCmdProc)(Vector *vecObjPtr, Tcl_Interp* interp, 
	int objc, Tcl_Obj* const objv[]);

static Vector *
FindVectorInNamespace(
    VectorInterpData *dataPtr,	/* Interpreter-specific data. */
    Blt_ObjectName *objNamePtr)
{
    Tcl_DString dString;
    const char *name;
    Tcl_HashEntry *hPtr;

    name = Blt_MakeQualifiedName(objNamePtr, &dString);
    hPtr = Tcl_FindHashEntry(&dataPtr->vectorTable, name);
    Tcl_DStringFree(&dString);
    if (hPtr != NULL) {
	return Tcl_GetHashValue(hPtr);
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetVectorObject --
 *
 *	Searches for the vector associated with the name given.  Allow for a
 *	range specification.
 *
 * Results:
 *	Returns a pointer to the vector if found, otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static Vector *
GetVectorObject(
    VectorInterpData *dataPtr,	/* Interpreter-specific data. */
    const char *name,
    int flags)
{
    Blt_ObjectName objName;
    Vector *vPtr;
    Tcl_Interp* interp;

    interp = dataPtr->interp;
    if (!Blt_ParseObjectName(interp, name, &objName, 
		BLT_NO_ERROR_MSG | BLT_NO_DEFAULT_NS)) {
	return NULL;		/* Can't find namespace. */
    } 
    vPtr = NULL;
    if (objName.nsPtr != NULL) {
	vPtr = FindVectorInNamespace(dataPtr, &objName);
    } else {
	if (flags & NS_SEARCH_CURRENT) {
	    objName.nsPtr = Tcl_GetCurrentNamespace(interp);
	    vPtr = FindVectorInNamespace(dataPtr, &objName);
	}
	if ((vPtr == NULL) && (flags & NS_SEARCH_GLOBAL)) {
	    objName.nsPtr = Tcl_GetGlobalNamespace(interp);
	    vPtr = FindVectorInNamespace(dataPtr, &objName);
	}
    }
    return vPtr;
}

void
Blt_Vec_UpdateRange(Vector *vPtr)
{
    double min, max;
    double *vp, *vend;

    vp = vPtr->valueArr + vPtr->first;
    vend = vPtr->valueArr + vPtr->last;
    min = max = *vp++;
    for (/* empty */; vp <= vend; vp++) {
	if (min > *vp) {
	    min = *vp; 
	} else if (max < *vp) { 
	    max = *vp; 
	} 
    } 
    vPtr->min = min;
    vPtr->max = max;
    vPtr->notifyFlags &= ~UPDATE_RANGE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_GetIndex --
 *
 *	Converts the string representing an index in the vector, to its
 *	numeric value.  A valid index may be an numeric string of the string
 *	"end" (indicating the last element in the string).
 *
 * Results:
 *	A standard TCL result.  If the string is a valid index, TCL_OK is
 *	returned.  Otherwise TCL_ERROR is returned and interp->result will
 *	contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_GetIndex(
    Tcl_Interp* interp,
    Vector *vPtr,
    const char *string,
    int *indexPtr,
    int flags,
    Blt_VectorIndexProc **procPtrPtr)
{
    char c;
    int value;

    c = string[0];

    /* Treat the index "end" like a numeric index.  */

    if ((c == 'e') && (strcmp(string, "end") == 0)) {
	if (vPtr->length < 1) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad index \"end\": vector is empty", 
				 (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	*indexPtr = vPtr->length - 1;
	return TCL_OK;
    } else if ((c == '+') && (strcmp(string, "++end") == 0)) {
	*indexPtr = vPtr->length;
	return TCL_OK;
    }
    if (procPtrPtr != NULL) {
	Tcl_HashEntry *hPtr;

	hPtr = Tcl_FindHashEntry(&vPtr->dataPtr->indexProcTable, string);
	if (hPtr != NULL) {
	    *indexPtr = SPECIAL_INDEX;
	    *procPtrPtr = Tcl_GetHashValue(hPtr);
	    return TCL_OK;
	}
    }
    if (Tcl_GetInt(interp, (char *)string, &value) != TCL_OK) {
	long int lvalue;
	/*   
	 * Unlike Tcl_GetInt, Tcl_ExprLong needs a valid interpreter, but the
	 * interp passed in may be NULL.  So we have to use vPtr->interp and
	 * then reset the result.
	 */
	if (Tcl_ExprLong(vPtr->interp, (char *)string, &lvalue) != TCL_OK) {
	    Tcl_ResetResult(vPtr->interp);
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad index \"", string, "\"", 
				 (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	value = (int)lvalue;
    }
    /*
     * Correct the index by the current value of the offset. This makes all
     * the numeric indices non-negative, which is how we distinguish the
     * special non-numeric indices.
     */
    value -= vPtr->offset;

    if ((value < 0) || ((flags & INDEX_CHECK) && (value >= vPtr->length))) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "index \"", string, "\" is out of range", 
			 (char *)NULL);
	}
	return TCL_ERROR;
    }
    *indexPtr = (int)value;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_GetIndexRange --
 *
 *	Converts the string representing an index in the vector, to its
 *	numeric value.  A valid index may be an numeric string of the string
 *	"end" (indicating the last element in the string).
 *
 * Results:
 *	A standard TCL result.  If the string is a valid index, TCL_OK is
 *	returned.  Otherwise TCL_ERROR is returned and interp->result will
 *	contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_GetIndexRange(
    Tcl_Interp* interp,
    Vector *vPtr,
    const char *string,
    int flags,
    Blt_VectorIndexProc **procPtrPtr)
{
    int ielem;
    char *colon;

    colon = NULL;
    if (flags & INDEX_COLON) {
	colon = strchr(string, ':');
    }
    if (colon != NULL) {
	if (string == colon) {
	    vPtr->first = 0;	/* Default to the first index */
	} else {
	    int result;

	    *colon = '\0';
	    result = Blt_Vec_GetIndex(interp, vPtr, string, &ielem, flags,
		(Blt_VectorIndexProc **) NULL);
	    *colon = ':';
	    if (result != TCL_OK) {
		return TCL_ERROR;
	    }
	    vPtr->first = ielem;
	}
	if (*(colon + 1) == '\0') {
	    /* Default to the last index */
	    vPtr->last = (vPtr->length > 0) ? vPtr->length - 1 : 0;
	} else {
	    if (Blt_Vec_GetIndex(interp, vPtr, colon + 1, &ielem, flags,
		    (Blt_VectorIndexProc **) NULL) != TCL_OK) {
		return TCL_ERROR;
	    }
	    vPtr->last = ielem;
	}
	if (vPtr->first > vPtr->last) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad range \"", string,
			 "\" (first > last)", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
    } else {
	if (Blt_Vec_GetIndex(interp, vPtr, string, &ielem, flags, 
		       procPtrPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	vPtr->last = vPtr->first = ielem;
    }
    return TCL_OK;
}

Vector *
Blt_Vec_ParseElement(
    Tcl_Interp* interp,
    VectorInterpData *dataPtr,	/* Interpreter-specific data. */
    const char *start,
    const char **endPtr,
    int flags)
{
    char *p;
    char saved;
    Vector *vPtr;

    p = (char *)start;
    /* Find the end of the vector name */
    while (VECTOR_CHAR(*p)) {
	p++;
    }
    saved = *p;
    *p = '\0';

    vPtr = GetVectorObject(dataPtr, start, flags);
    if (vPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find vector \"", start, "\"", 
			     (char *)NULL);
	}
	*p = saved;
	return NULL;
    }
    *p = saved;
    vPtr->first = 0;
    vPtr->last = vPtr->length - 1;
    if (*p == '(') {
	int count, result;

	start = p + 1;
	p++;

	/* Find the matching right parenthesis */
	count = 1;
	while (*p != '\0') {
	    if (*p == ')') {
		count--;
		if (count == 0) {
		    break;
		}
	    } else if (*p == '(') {
		count++;
	    }
	    p++;
	}
	if (count > 0) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "unbalanced parentheses \"", start, 
			"\"", (char *)NULL);
	    }
	    return NULL;
	}
	*p = '\0';
	result = Blt_Vec_GetIndexRange(interp, vPtr, start, 
		(INDEX_COLON | INDEX_CHECK), (Blt_VectorIndexProc **) NULL);
	*p = ')';
	if (result != TCL_OK) {
	    return NULL;
	}
	p++;
    }
    if (endPtr != NULL) {
      *endPtr = p;
    }
    return vPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_NotifyClients --
 *
 *	Notifies each client of the vector that the vector has changed
 *	(updated or destroyed) by calling the provided function back.  The
 *	function pointer may be NULL, in that case the client is not notified.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The results depend upon what actions the client callbacks
 *	take.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_NotifyClients(ClientData clientData)
{
    Vector *vPtr = clientData;
    Blt_ChainLink link, next;
    Blt_VectorNotify notify;

    notify = (vPtr->notifyFlags & NOTIFY_DESTROYED)
	? BLT_VECTOR_NOTIFY_DESTROY : BLT_VECTOR_NOTIFY_UPDATE;
    vPtr->notifyFlags &= ~(NOTIFY_UPDATED | NOTIFY_DESTROYED | NOTIFY_PENDING);
    for (link = Blt_Chain_FirstLink(vPtr->chain); link != NULL; link = next) {
	VectorClient *clientPtr;

	next = Blt_Chain_NextLink(link);
	clientPtr = Blt_Chain_GetValue(link);
	if ((clientPtr->proc != NULL) && (clientPtr->serverPtr != NULL)) {
	    (*clientPtr->proc) (vPtr->interp, clientPtr->clientData, notify);
	}
    }
    /*
     * Some clients may not handle the "destroy" callback properly (they
     * should call Blt_FreeVectorId to release the client identifier), so mark
     * any remaining clients to indicate that vector's server has gone away.
     */
    if (notify == BLT_VECTOR_NOTIFY_DESTROY) {
	for (link = Blt_Chain_FirstLink(vPtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    VectorClient *clientPtr;

	    clientPtr = Blt_Chain_GetValue(link);
	    clientPtr->serverPtr = NULL;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_UpdateClients --
 *
 *	Notifies each client of the vector that the vector has changed
 *	(updated or destroyed) by calling the provided function back.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The individual client callbacks are eventually invoked.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_UpdateClients(Vector *vPtr)
{
    vPtr->dirty++;
    vPtr->max = vPtr->min = NAN;
    if (vPtr->notifyFlags & NOTIFY_NEVER) {
	return;
    }
    vPtr->notifyFlags |= NOTIFY_UPDATED;
    if (vPtr->notifyFlags & NOTIFY_ALWAYS) {
	Blt_Vec_NotifyClients(vPtr);
	return;
    }
    if (!(vPtr->notifyFlags & NOTIFY_PENDING)) {
	vPtr->notifyFlags |= NOTIFY_PENDING;
	Tcl_DoWhenIdle(Blt_Vec_NotifyClients, vPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_FlushCache --
 *
 *	Unsets all the elements of the TCL array variable associated with the
 *	vector, freeing memory associated with the variable.  This includes
 *	both the hash table and the hash keys.  The down side is that this
 *	effectively flushes the caching of vector elements in the array.  This
 *	means that the subsequent reads of the array will require a decimal to
 *	string conversion.
 *
 *	This is needed when the vector changes its values, making the array
 *	variable out-of-sync.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All elements of array variable (except one) are unset, freeing
 *	the memory associated with the variable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_FlushCache(Vector *vPtr)
{
    Tcl_Interp* interp = vPtr->interp;

    if (vPtr->arrayName == NULL) {
	return;			/* Doesn't use the variable API */
    }
    /* Turn off the trace temporarily so that we can unset all the
     * elements in the array.  */

    Tcl_UntraceVar2(interp, vPtr->arrayName, (char *)NULL,
	TRACE_ALL | vPtr->varFlags, Blt_Vec_VarTrace, vPtr);

    /* Clear all the element entries from the entire array */
    Tcl_UnsetVar2(interp, vPtr->arrayName, (char *)NULL, vPtr->varFlags);

    /* Restore the "end" index by default and the trace on the entire array */
    Tcl_SetVar2(interp, vPtr->arrayName, "end", "", vPtr->varFlags);
    Tcl_TraceVar2(interp, vPtr->arrayName, (char *)NULL,
	TRACE_ALL | vPtr->varFlags, Blt_Vec_VarTrace, vPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_LookupName --
 *
 *	Searches for the vector associated with the name given.  Allow for a
 *	range specification.
 *
 * Results:
 *	Returns a pointer to the vector if found, otherwise NULL.  If the name
 *	is not associated with a vector and the TCL_LEAVE_ERR_MSG flag is set,
 *	and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_LookupName(
    VectorInterpData *dataPtr,	/* Interpreter-specific data. */
    const char *vecName,
    Vector **vPtrPtr)
{
    Vector *vPtr;
    const char *endPtr;

    vPtr = Blt_Vec_ParseElement(dataPtr->interp, dataPtr, vecName, &endPtr, 
	NS_SEARCH_BOTH);
    if (vPtr == NULL) {
	return TCL_ERROR;
    }
    if (*endPtr != '\0') {
	Tcl_AppendResult(dataPtr->interp, 
			 "extra characters after vector name", (char *)NULL);
	return TCL_ERROR;
    }
    *vPtrPtr = vPtr;
    return TCL_OK;
}

double
Blt_Vec_Min(Vector *vecObjPtr)
{
    double *vp, *vend;
    double min;

    vp = vecObjPtr->valueArr + vecObjPtr->first;
    vend = vecObjPtr->valueArr + vecObjPtr->last;
    min = *vp++;
    for (/* empty */; vp <= vend; vp++) {
	if (min > *vp) {
	    min = *vp; 
	} 
    } 
    vecObjPtr->min = min;
    return vecObjPtr->min;
}

double
Blt_Vec_Max(Vector *vecObjPtr)
{
    double max;
    double *vp, *vend;

    max = NAN;
    vp = vecObjPtr->valueArr + vecObjPtr->first;
    vend = vecObjPtr->valueArr + vecObjPtr->last;
    max = *vp++;
    for (/* empty */; vp <= vend; vp++) {
	if (max < *vp) {
	    max = *vp; 
	} 
    } 
    vecObjPtr->max = max;
    return vecObjPtr->max;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteCommand --
 *
 *	Deletes the TCL command associated with the vector, without triggering
 *	a callback to "VectorInstDeleteProc".
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteCommand(Vector *vPtr) /* Vector associated with the TCL command. */
{
    Tcl_Interp* interp = vPtr->interp;
    char *qualName;		/* Name of TCL command. */
    Tcl_CmdInfo cmdInfo;
    Tcl_DString dString;
    Blt_ObjectName objName;

    Tcl_DStringInit(&dString);
    objName.name = Tcl_GetCommandName(interp, vPtr->cmdToken);
    objName.nsPtr = Blt_GetCommandNamespace(vPtr->cmdToken);
    qualName = Blt_MakeQualifiedName(&objName, &dString);
    if (Tcl_GetCommandInfo(interp, qualName, &cmdInfo)) {
	/* Disable the callback before deleting the TCL command.*/	
	cmdInfo.deleteProc = NULL;	
	Tcl_SetCommandInfo(interp, qualName, &cmdInfo);
	Tcl_DeleteCommandFromToken(interp, vPtr->cmdToken);
    }
    Tcl_DStringFree(&dString);
    vPtr->cmdToken = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnmapVariable --
 *
 *	Destroys the trace on the current TCL variable designated to access
 *	the vector.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
UnmapVariable(Vector *vPtr)
{
    Tcl_Interp* interp = vPtr->interp;

    /* Unset the entire array */
    Tcl_UntraceVar2(interp, vPtr->arrayName, (char *)NULL,
	(TRACE_ALL | vPtr->varFlags), Blt_Vec_VarTrace, vPtr);
    Tcl_UnsetVar2(interp, vPtr->arrayName, (char *)NULL, vPtr->varFlags);

    if (vPtr->arrayName != NULL) {
      free((void*)(vPtr->arrayName));
      vPtr->arrayName = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_MapVariable --
 *
 *	Sets up traces on a TCL variable to access the vector.
 *
 *	If another variable is already mapped, it's first untraced and
 *	removed.  Don't do anything else for variables named "" (even though
 *	Tcl allows this pathology). Saves the name of the new array variable.
 *
 * Results:
 *	A standard TCL result. If an error occurs setting the variable
 *	TCL_ERROR is returned and an error message is left in the interpreter.
 *
 * Side effects:
 *	Traces are set for the new variable. The new variable name is saved in
 *	a malloc'ed string in vPtr->arrayName.  If this variable is non-NULL,
 *	it indicates that a TCL variable has been mapped to this vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_MapVariable(
    Tcl_Interp* interp, 
    Vector *vPtr, 
    const char *path)
{
    Blt_ObjectName objName;
    char *newPath;
    const char *result;
    Tcl_DString dString;

    if (vPtr->arrayName != NULL) {
	UnmapVariable(vPtr);
    }
    if ((path == NULL) || (path[0] == '\0')) {
	return TCL_OK;		/* If the variable pathname is the empty
				 * string, simply return after removing any
				 * existing variable. */
    }
    /* Get the variable name (without the namespace qualifier). */
    if (!Blt_ParseObjectName(interp, path, &objName, BLT_NO_DEFAULT_NS)) {
	return TCL_ERROR;
    }
    if (objName.nsPtr == NULL) {
	/* 
	 * If there was no namespace qualifier, try harder to see if the
	 * variable is non-local.
	 */
	objName.nsPtr = Blt_GetVariableNamespace(interp, objName.name);
    } 
    Tcl_DStringInit(&dString);
    vPtr->varFlags = 0;
    if (objName.nsPtr != NULL) {	/* Global or namespace variable. */
	newPath = Blt_MakeQualifiedName(&objName, &dString);
	vPtr->varFlags |= (TCL_GLOBAL_ONLY);
    } else {			/* Local variable. */
	newPath = (char *)objName.name;
    }

    /*
     * To play it safe, delete the variable first.  This has the benefical
     * side-effect of unmapping the variable from another vector that may be
     * currently associated with it.
     */
    Tcl_UnsetVar2(interp, newPath, (char *)NULL, 0);

    /* 
     * Set the index "end" in the array.  This will create the variable
     * immediately so that we can check its namespace context.
     */
    result = Tcl_SetVar2(interp, newPath, "end", "", TCL_LEAVE_ERR_MSG);
    if (result == NULL) {
	Tcl_DStringFree(&dString);
	return TCL_ERROR;
    }
    /* Create a full-array trace on reads, writes, and unsets. */
    Tcl_TraceVar2(interp, newPath, (char *)NULL, TRACE_ALL, Blt_Vec_VarTrace,
	vPtr);
    vPtr->arrayName = Blt_Strdup(newPath);
    Tcl_DStringFree(&dString);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_SetSize --
 *
 *	Resizes the vector to the designated new size.
 *
 *	If the new size is the same as the old, simply return.  Otherwise
 *	we're copying the data from one memory location to another.
 *
 *	If the storage changed memory locations, free up the old location if
 *	it was dynamically allocated.
 *
 * Results:
 *	A standard TCL result.  If the reallocation is successful,
 *	TCL_OK is returned, otherwise TCL_ERROR.
 *
 * Side effects:
 *	Memory for the array is reallocated.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_SetSize(
    Tcl_Interp* interp, 
    Vector *vPtr, 
    int newSize)		/* Size of array in elements */
{
    if (newSize <= 0) {
	newSize = DEF_ARRAY_SIZE;
    }
    if (newSize == vPtr->size) {
	/* Same size, use the current array. */
	return TCL_OK;
    } 
    if (vPtr->freeProc == TCL_DYNAMIC) {
	double *newArr;

	/* Old memory was dynamically allocated, so use realloc. */
	newArr = realloc(vPtr->valueArr, newSize * sizeof(double));
	if (newArr == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "can't reallocate ", 
				 Blt_Itoa(newSize), " elements for vector \"", 
				 vPtr->name, "\"", (char *)NULL); 
	    }
	    return TCL_ERROR;
	}
	vPtr->size = newSize;
	vPtr->valueArr = newArr;
	return TCL_OK;
    }

    {
	double *newArr;

	/* Old memory was created specially (static or special allocator).
	 * Replace with dynamically allocated memory (malloc-ed). */

	newArr = calloc(newSize, sizeof(double));
	if (newArr == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "can't allocate ", 
				 Blt_Itoa(newSize), " elements for vector \"", 
				 vPtr->name, "\"", (char *)NULL); 
	    }
	    return TCL_ERROR;
	}
	{
	    int used, wanted;
	    
	    /* Copy the contents of the old memory into the new. */
	    used = vPtr->length;
	    wanted = newSize;
	    
	    if (used > wanted) {
		used = wanted;
	    }
	    /* Copy any previous data */
	    if (used > 0) {
		memcpy(newArr, vPtr->valueArr, used * sizeof(double));
	    }
	}
	
	assert(vPtr->valueArr != NULL);
	
	/* 
	 * We're not using the old storage anymore, so free it if it's not
	 * TCL_STATIC.  It's static because the user previously reset the
	 * vector with a statically allocated array (setting freeProc to
	 * TCL_STATIC).
	 */
	if (vPtr->freeProc != TCL_STATIC) {
	    if (vPtr->freeProc == TCL_DYNAMIC) {
		free(vPtr->valueArr);
	    } else {
		(*vPtr->freeProc) ((char *)vPtr->valueArr);
	    }
	}
	vPtr->freeProc = TCL_DYNAMIC; /* Set the type of the new storage */
	vPtr->valueArr = newArr;
	vPtr->size = newSize;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_SetSize --
 *
 *	Set the length (the number of elements currently in use) of the
 *	vector.  If the new length is greater than the size (total number of
 *	slots), then the vector is grown.
 *
 * Results:
 *	A standard TCL result.  If the reallocation is successful, TCL_OK is
 *	returned, otherwise TCL_ERROR.
 *
 * Side effects:
 *	Memory for the array is possibly reallocated.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_SetLength(
    Tcl_Interp* interp, 
    Vector *vPtr, 
    int newLength)		/* Size of array in elements */
{
    if (vPtr->size < newLength) {
	if (Blt_Vec_SetSize(interp, vPtr, newLength) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    vPtr->length = newLength;
    vPtr->first = 0;
    vPtr->last = newLength - 1;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_ChangeLength --
 *
 *	Resizes the vector to the new size.
 *
 *	The new size of the vector is computed by doubling the size of the
 *	vector until it fits the number of slots needed (designated by
 *	*length*).
 *
 *	If the new size is the same as the old, simply adjust the length of
 *	the vector.  Otherwise we're copying the data from one memory location
 *	to another. The trailing elements of the vector need to be reset to
 *	zero.
 *
 *	If the storage changed memory locations, free up the old location if
 *	it was dynamically allocated.
 *
 * Results:
 *	A standard TCL result.  If the reallocation is successful,
 *	TCL_OK is returned, otherwise TCL_ERROR.
 *
 * Side effects:
 *	Memory for the array is reallocated.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_Vec_ChangeLength(
    Tcl_Interp* interp, 
    Vector *vPtr, 
    int newLength)
{
    if (newLength < 0) {
	newLength = 0;
    } 
    if (newLength > vPtr->size) {
	int newSize;		/* Size of array in elements */
    
	/* Compute the new size of the array.  It's a multiple of
	 * DEF_ARRAY_SIZE. */
	newSize = DEF_ARRAY_SIZE;
	while (newSize < newLength) {
	    newSize += newSize;
	}
	if (newSize != vPtr->size) {
	    if (Blt_Vec_SetSize(interp, vPtr, newSize) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    vPtr->length = newLength;
    vPtr->first = 0;
    vPtr->last = newLength - 1;
    return TCL_OK;
    
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Reset --
 *
 *	Resets the vector data.  This is called by a client to indicate that
 *	the vector data has changed.  The vector does not need to point to
 *	different memory.  Any clients of the vector will be notified of the
 *	change.
 *
 * Results:
 *	A standard TCL result.  If the new array size is invalid, TCL_ERROR is
 *	returned.  Otherwise TCL_OK is returned and the new vector data is
 *	recorded.
 *
 * Side Effects:
 *	Any client designated callbacks will be posted.  Memory may be changed
 *	for the vector array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_Reset(
    Vector *vPtr,
    double *valueArr,		/* Array containing the elements of the
				 * vector. If NULL, indicates to reset the 
				 * vector size to the default. */
    int length,			/* The number of elements that the vector 
				 * currently holds. */
    int size,			/* The maximum number of elements that the
				 * array can hold. */
    Tcl_FreeProc *freeProc)	/* Address of memory deallocation routine
				 * for the array of values.  Can also be
				 * TCL_STATIC, TCL_DYNAMIC, or TCL_VOLATILE. */
{
    if (vPtr->valueArr != valueArr) {	/* New array of values resides
					 * in different memory than
					 * the current vector.  */
	if ((valueArr == NULL) || (size == 0)) {
	    /* Empty array. Set up default values */
	    valueArr = malloc(sizeof(double) * DEF_ARRAY_SIZE);
	    size = DEF_ARRAY_SIZE;
	    if (valueArr == NULL) {
		Tcl_AppendResult(vPtr->interp, "can't allocate ", 
			Blt_Itoa(size), " elements for vector \"", 
			vPtr->name, "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    freeProc = TCL_DYNAMIC;
	    length = 0;
	} else if (freeProc == TCL_VOLATILE) {
	    double *newArr;
	    /* Data is volatile. Make a copy of the value array.  */
	    newArr = malloc(size * sizeof(double));
	    if (newArr == NULL) {
		Tcl_AppendResult(vPtr->interp, "can't allocate ", 
			Blt_Itoa(size), " elements for vector \"", 
			vPtr->name, "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    memcpy((char *)newArr, (char *)valueArr, 
		   sizeof(double) * length);
	    valueArr = newArr;
	    freeProc = TCL_DYNAMIC;
	} 

	if (vPtr->freeProc != TCL_STATIC) {
	    /* Old data was dynamically allocated. Free it before attaching
	     * new data.  */
	    if (vPtr->freeProc == TCL_DYNAMIC) {
		free(vPtr->valueArr);
	    } else {
		(*freeProc) ((char *)vPtr->valueArr);
	    }
	}
	vPtr->freeProc = freeProc;
	vPtr->valueArr = valueArr;
	vPtr->size = size;
    }

    vPtr->length = length;
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

Vector *
Blt_Vec_New(VectorInterpData *dataPtr) /* Interpreter-specific data. */
{
    Vector *vPtr;

    vPtr = calloc(1, sizeof(Vector));
    vPtr->valueArr = malloc(sizeof(double) * DEF_ARRAY_SIZE);
    if (vPtr->valueArr == NULL) {
	free(vPtr);
	return NULL;
    }
    vPtr->size = DEF_ARRAY_SIZE;
    vPtr->freeProc = TCL_DYNAMIC;
    vPtr->length = 0;
    vPtr->interp = dataPtr->interp;
    vPtr->hashPtr = NULL;
    vPtr->chain = Blt_Chain_Create();
    vPtr->flush = FALSE;
    vPtr->min = vPtr->max = NAN;
    vPtr->notifyFlags = NOTIFY_WHENIDLE;
    vPtr->dataPtr = dataPtr;
    return vPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Free --
 *
 *	Removes the memory and frees resources associated with the vector.
 *
 *	o Removes the trace and the TCL array variable and unsets
 *	  the variable.
 *	o Notifies clients of the vector that the vector is being
 *	  destroyed.
 *	o Removes any clients that are left after notification.
 *	o Frees the memory (if necessary) allocated for the array.
 *	o Removes the entry from the hash table of vectors.
 *	o Frees the memory allocated for the name.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_Free(Vector *vPtr)
{
    Blt_ChainLink link;

    if (vPtr->cmdToken != 0) {
	DeleteCommand(vPtr);
    }
    if (vPtr->arrayName != NULL) {
	UnmapVariable(vPtr);
    }
    vPtr->length = 0;

    /* Immediately notify clients that vector is going away */
    if (vPtr->notifyFlags & NOTIFY_PENDING) {
	vPtr->notifyFlags &= ~NOTIFY_PENDING;
	Tcl_CancelIdleCall(Blt_Vec_NotifyClients, vPtr);
    }
    vPtr->notifyFlags |= NOTIFY_DESTROYED;
    Blt_Vec_NotifyClients(vPtr);

    for (link = Blt_Chain_FirstLink(vPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	VectorClient *clientPtr;

	clientPtr = Blt_Chain_GetValue(link);
	free(clientPtr);
    }
    Blt_Chain_Destroy(vPtr->chain);
    if ((vPtr->valueArr != NULL) && (vPtr->freeProc != TCL_STATIC)) {
	if (vPtr->freeProc == TCL_DYNAMIC) {
	    free(vPtr->valueArr);
	} else {
	    (*vPtr->freeProc) ((char *)vPtr->valueArr);
	}
    }
    if (vPtr->hashPtr != NULL) {
	Tcl_DeleteHashEntry(vPtr->hashPtr);
    }
#ifdef NAMESPACE_DELETE_NOTIFY
    if (vPtr->nsPtr != NULL) {
	Blt_DestroyNsDeleteNotify(vPtr->interp, vPtr->nsPtr, vPtr);
    }
#endif /* NAMESPACE_DELETE_NOTIFY */
    free(vPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorInstDeleteProc --
 *
 *	Deletes the command associated with the vector.  This is called only
 *	when the command associated with the vector is destroyed.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
VectorInstDeleteProc(ClientData clientData)
{
    Vector *vPtr = clientData;

    vPtr->cmdToken = 0;
    Blt_Vec_Free(vPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Create --
 *
 *	Creates a vector structure and the following items:
 *
 *	o TCL command
 *	o TCL array variable and establishes traces on the variable
 *	o Adds a  new entry in the vector hash table
 *
 * Results:
 *	A pointer to the new vector structure.  If an error occurred NULL is
 *	returned and an error message is left in interp->result.
 *
 * Side effects:
 *	A new TCL command and array variable is added to the interpreter.
 *
 * ---------------------------------------------------------------------- 
 */
Vector *
Blt_Vec_Create(
    VectorInterpData *dataPtr,	/* Interpreter-specific data. */
    const char *vecName,	/* Namespace-qualified name of the vector */
    const char *cmdName,	/* Name of the TCL command mapped to
				 * the vector */
    const char *varName,	/* Name of the TCL array mapped to the
				 * vector */
    int *isNewPtr)
{
    Tcl_DString dString;
    Vector *vPtr;
    int isNew;
    Blt_ObjectName objName;
    char *qualName;
    Tcl_HashEntry *hPtr;
    Tcl_Interp* interp = dataPtr->interp;

    isNew = 0;
    vPtr = NULL;

    if (!Blt_ParseObjectName(interp, vecName, &objName, 0)) {
	return NULL;
    }
    Tcl_DStringInit(&dString);
    if ((objName.name[0] == '#') && (strcmp(objName.name, "#auto") == 0)) {

	do {	/* Generate a unique vector name. */
	    char string[200];

	    sprintf_s(string, 200, "vector%d", dataPtr->nextId++);
	    objName.name = string;
	    qualName = Blt_MakeQualifiedName(&objName, &dString);
	    hPtr = Tcl_FindHashEntry(&dataPtr->vectorTable, qualName);
	} while (hPtr != NULL);
    } else {
	const char *p;

	for (p = objName.name; *p != '\0'; p++) {
	    if (!VECTOR_CHAR(*p)) {
		Tcl_AppendResult(interp, "bad vector name \"", objName.name,
		    "\": must contain digits, letters, underscore, or period",
		    (char *)NULL);
		goto error;
	    }
	}
	qualName = Blt_MakeQualifiedName(&objName, &dString);
	vPtr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, dataPtr, qualName, 
		NULL, NS_SEARCH_CURRENT);
    }
    if (vPtr == NULL) {
	hPtr = Tcl_CreateHashEntry(&dataPtr->vectorTable, qualName, &isNew);
	vPtr = Blt_Vec_New(dataPtr);
	vPtr->hashPtr = hPtr;
	vPtr->nsPtr = objName.nsPtr;

	vPtr->name = Tcl_GetHashKey(&dataPtr->vectorTable, hPtr);
#ifdef NAMESPACE_DELETE_NOTIFY
	Blt_CreateNsDeleteNotify(interp, objName.nsPtr, vPtr, 
		VectorInstDeleteProc);
#endif /* NAMESPACE_DELETE_NOTIFY */
	Tcl_SetHashValue(hPtr, vPtr);
    }
    if (cmdName != NULL) {
	Tcl_CmdInfo cmdInfo;

	if ((cmdName == vecName) ||
	    ((cmdName[0] == '#') && (strcmp(cmdName, "#auto")==0))) {
	    cmdName = qualName;
	} 
	if (Tcl_GetCommandInfo(interp, (char *)cmdName, &cmdInfo)) {
	    if (vPtr != cmdInfo.objClientData) {
		Tcl_AppendResult(interp, "command \"", cmdName,
			 "\" already exists", (char *)NULL);
		goto error;
	    }
	    /* We get here only if the old name is the same as the new. */
	    goto checkVariable;
	}
    }
    if (vPtr->cmdToken != 0) {
	DeleteCommand(vPtr);	/* Command already exists, delete old first */
    }
    if (cmdName != NULL) {
	Tcl_DString dString2;
	
	Tcl_DStringInit(&dString2);
	if (cmdName != qualName) {
	    if (!Blt_ParseObjectName(interp, cmdName, &objName, 0)) {
		goto error;
	    }
	    cmdName = Blt_MakeQualifiedName(&objName, &dString2);
	}
	vPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)cmdName, 
		Blt_Vec_InstCmd, vPtr, VectorInstDeleteProc);
	Tcl_DStringFree(&dString2);
    }
  checkVariable:
    if (varName != NULL) {
	if ((varName[0] == '#') && (strcmp(varName, "#auto") == 0)) {
	    varName = qualName;
	}
	if (Blt_Vec_MapVariable(interp, vPtr, varName) != TCL_OK) {
	    goto error;
	}
    }

    Tcl_DStringFree(&dString);
    *isNewPtr = isNew;
    return vPtr;

  error:
    Tcl_DStringFree(&dString);
    if (vPtr != NULL) {
	Blt_Vec_Free(vPtr);
    }
    return NULL;
}


int
Blt_Vec_Duplicate(Vector *destPtr, Vector *srcPtr)
{
    size_t nBytes;
    size_t length;

    if (destPtr == srcPtr) {
	/* Copying the same vector. */
    }
    length = srcPtr->last - srcPtr->first + 1;
    if (Blt_Vec_ChangeLength(destPtr->interp, destPtr, length) != TCL_OK) {
	return TCL_ERROR;
    }
    nBytes = length * sizeof(double);
    memcpy(destPtr->valueArr, srcPtr->valueArr + srcPtr->first, nBytes);
    destPtr->offset = srcPtr->offset;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * VectorNamesOp --
 *
 *	Reports the names of all the current vectors in the interpreter.
 *
 * Results:
 *	A standard TCL result.  interp->result will contain a list of
 *	all the names of the vector instances.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
VectorNamesOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp* interp,
    int objc,
    Tcl_Obj* const objv[])
{
    VectorInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (objc == 2) {
	Tcl_HashEntry *hPtr;
	Tcl_HashSearch cursor;

	for (hPtr = Tcl_FirstHashEntry(&dataPtr->vectorTable, &cursor);
	     hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
	    char *name;

	    name = Tcl_GetHashKey(&dataPtr->vectorTable, hPtr);
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(name, -1));
	}
    } else {
	Tcl_HashEntry *hPtr;
	Tcl_HashSearch cursor;

	for (hPtr = Tcl_FirstHashEntry(&dataPtr->vectorTable, &cursor);
	     hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
	    char *name;
	    int i;

	    name = Tcl_GetHashKey(&dataPtr->vectorTable, hPtr);
	    for (i = 2; i < objc; i++) {
		char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(name, pattern)) {
		    Tcl_ListObjAppendElement(interp, listObjPtr, 
				Tcl_NewStringObj(name, -1));
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
 * VectorCreateOp --
 *
 *	Creates a TCL command, and array variable representing an instance of
 *	a vector.
 *
 *	vector a
 *	vector b(20)
 *	vector c(-5:14)
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
VectorCreate2(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp* interp,
    int argStart,
    int objc,
    Tcl_Obj* const objv[])
{
    VectorInterpData *dataPtr = clientData;
    Vector *vPtr;
    int count, i;
    CreateSwitches switches;

    /*
     * Handle switches to the vector command and collect the vector name
     * arguments into an array.
     */
    count = 0;
    vPtr = NULL;
    for (i = argStart; i < objc; i++) {
	char *string;

	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
    }
    count = i - argStart;
    if (count == 0) {
	Tcl_AppendResult(interp, "no vector names supplied", (char *)NULL);
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, createSwitches, objc - i, objv + i, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (count > 1) {
	if (switches.cmdName != NULL) {
	    Tcl_AppendResult(interp, 
		"can't specify more than one vector with \"-command\" switch",
		(char *)NULL);
	    goto error;
	}
	if (switches.varName != NULL) {
	    Tcl_AppendResult(interp,
		"can't specify more than one vector with \"-variable\" switch",
		(char *)NULL);
	    goto error;
	}
    }
    for (i = 0; i < count; i++) {
	char *leftParen, *rightParen;
	char *string;
	int isNew;
	int size, first, last;

	size = first = last = 0;
	string = Tcl_GetString(objv[i + argStart]);
	leftParen = strchr(string, '(');
	rightParen = strchr(string, ')');
	if (((leftParen != NULL) && (rightParen == NULL)) ||
	    ((leftParen == NULL) && (rightParen != NULL)) ||
	    (leftParen > rightParen)) {
	    Tcl_AppendResult(interp, "bad vector specification \"", string,
		"\"", (char *)NULL);
	    goto error;
	}
	if (leftParen != NULL) {
	    int result;
	    char *colon;

	    *rightParen = '\0';
	    colon = strchr(leftParen + 1, ':');
	    if (colon != NULL) {

		/* Specification is in the form vecName(first:last) */
		*colon = '\0';
		result = Tcl_GetInt(interp, leftParen + 1, &first);
		if ((*(colon + 1) != '\0') && (result == TCL_OK)) {
		    result = Tcl_GetInt(interp, colon + 1, &last);
		    if (first > last) {
			Tcl_AppendResult(interp, "bad vector range \"",
			    string, "\"", (char *)NULL);
			result = TCL_ERROR;
		    }
		    size = (last - first) + 1;
		}
		*colon = ':';
	    } else {
		/* Specification is in the form vecName(size) */
		result = Tcl_GetInt(interp, leftParen + 1, &size);
	    }
	    *rightParen = ')';
	    if (result != TCL_OK) {
		goto error;
	    }
	    if (size < 0) {
		Tcl_AppendResult(interp, "bad vector size \"", string, "\"",
		    (char *)NULL);
		goto error;
	    }
	}
	if (leftParen != NULL) {
	    *leftParen = '\0';
	}
	/*
	 * By default, we create a TCL command by the name of the vector.
	 */
	vPtr = Blt_Vec_Create(dataPtr, string,
	    (switches.cmdName == NULL) ? string : switches.cmdName,
	    (switches.varName == NULL) ? string : switches.varName, &isNew);
	if (leftParen != NULL) {
	    *leftParen = '(';
	}
	if (vPtr == NULL) {
	    goto error;
	}
	vPtr->freeOnUnset = switches.watchUnset;
	vPtr->flush = switches.flush;
	vPtr->offset = first;
	if (size > 0) {
	    if (Blt_Vec_ChangeLength(interp, vPtr, size) != TCL_OK) {
		goto error;
	    }
	}
	if (!isNew) {
	    if (vPtr->flush) {
		Blt_Vec_FlushCache(vPtr);
	    }
	    Blt_Vec_UpdateClients(vPtr);
	}
    }
    Blt_FreeSwitches(createSwitches, (char *)&switches, 0);
    if (vPtr != NULL) {
	/* Return the name of the last vector created  */
	Tcl_SetStringObj(Tcl_GetObjResult(interp), vPtr->name, -1);
    }
    return TCL_OK;
  error:
    Blt_FreeSwitches(createSwitches, (char *)&switches, 0);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorCreateOp --
 *
 *	Creates a TCL command, and array variable representing an instance of
 *	a vector.
 *
 *	vector a
 *	vector b(20)
 *	vector c(-5:14)
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
VectorCreateOp(
    ClientData clientData,
    Tcl_Interp* interp,
    int objc,
    Tcl_Obj* const objv[])
{
    return VectorCreate2(clientData, interp, 2, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorDestroyOp --
 *
 *	Destroys the vector and its related TCL command and array variable (if
 *	they exist).
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	Deletes the vector.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
VectorDestroyOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp* interp,		/* Not used. */
    int objc,
    Tcl_Obj* const objv[])
{
    VectorInterpData *dataPtr = clientData;
    Vector *vPtr;
    int i;

    for (i = 2; i < objc; i++) {
	if (Blt_Vec_LookupName(dataPtr, Tcl_GetString(objv[i]), &vPtr) 
		!= TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_Vec_Free(vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorExprOp --
 *
 *	Computes the result of the expression which may be either a scalar
 *	(single value) or vector (list of values).
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
VectorExprOp(
    ClientData clientData,	/* Not Used. */
    Tcl_Interp* interp,
    int objc,			/* Not used. */
    Tcl_Obj* const objv[])
{
    return Blt_ExprVector(interp, Tcl_GetString(objv[2]), (Blt_Vector *)NULL);
}

static Blt_OpSpec vectorCmdOps[] =
{
    {"create", 1, VectorCreateOp, 3, 0,
	"vecName ?vecName...? ?switches...?",},
    {"destroy", 1, VectorDestroyOp, 3, 0,
	"vecName ?vecName...?",},
    {"expr", 1, VectorExprOp, 3, 3, "expression",},
    {"names", 1, VectorNamesOp, 2, 3, "?pattern?...",},
};

static int nCmdOps = sizeof(vectorCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
VectorCmd(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp* interp,
    int objc,
    Tcl_Obj* const objv[])
{
    VectorCmdProc *proc;
    /*
     * Try to replicate the old vector command's behavior:
     */
    if (objc > 1) {
	char *string;
	char c;
	int i;
	Blt_OpSpec *specPtr;

	string = Tcl_GetString(objv[1]);
	c = string[0];
	for (specPtr = vectorCmdOps, i = 0; i < nCmdOps; i++, specPtr++) {
	    if ((c == specPtr->name[0]) &&
		(strcmp(string, specPtr->name) == 0)) {
		goto doOp;
	    }
	}
	/*
	 * The first argument is not an operation, so assume that its
	 * actually the name of a vector to be created
	 */
	return VectorCreate2(clientData, interp, 1, objc, objv);
    }
  doOp:
    /* Do the usual vector operation lookup now. */
    proc = Blt_GetOpFromObj(interp, nCmdOps, vectorCmdOps, BLT_OP_ARG1, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the "vector" command
 *	is deleted.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys the math and index hash tables.  In addition removes
 *	the hash table managing all vector names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
VectorInterpDeleteProc(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp* interp)
{
    VectorInterpData *dataPtr = clientData;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch cursor;
    
    for (hPtr = Tcl_FirstHashEntry(&dataPtr->vectorTable, &cursor);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
	Vector *vPtr;

	vPtr = Tcl_GetHashValue(hPtr);
	vPtr->hashPtr = NULL;
	Blt_Vec_Free(vPtr);
    }
    Tcl_DeleteHashTable(&dataPtr->vectorTable);

    /* If any user-defined math functions were installed, remove them.  */
    Blt_Vec_UninstallMathFunctions(&dataPtr->mathProcTable);
    Tcl_DeleteHashTable(&dataPtr->mathProcTable);

    Tcl_DeleteHashTable(&dataPtr->indexProcTable);
    Tcl_DeleteAssocData(interp, VECTOR_THREAD_KEY);
    free(dataPtr);
}

VectorInterpData *
Blt_Vec_GetInterpData(Tcl_Interp* interp)
{
    VectorInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (VectorInterpData *)
	Tcl_GetAssocData(interp, VECTOR_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = malloc(sizeof(VectorInterpData));
	dataPtr->interp = interp;
	dataPtr->nextId = 0;
	Tcl_SetAssocData(interp, VECTOR_THREAD_KEY, VectorInterpDeleteProc,
		 dataPtr);
	Tcl_InitHashTable(&dataPtr->vectorTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&dataPtr->mathProcTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&dataPtr->indexProcTable, TCL_STRING_KEYS);
	Blt_Vec_InstallMathFunctions(&dataPtr->mathProcTable);
	Blt_Vec_InstallSpecialIndices(&dataPtr->indexProcTable);
	srand48(time((time_t *) NULL));
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorCmdInitProc --
 *
 *	This procedure is invoked to initialize the "vector" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_VectorCmdInitProc(Tcl_Interp* interp)
{
    static Blt_InitCmdSpec cmdSpec = {"vector", VectorCmd, };
    
    cmdSpec.clientData = Blt_Vec_GetInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

/* C Application interface to vectors */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateVector --
 *
 *	Creates a new vector by the name and size.
 *
 * Results:
 *	A standard TCL result.  If the new array size is invalid or a vector
 *	already exists by that name, TCL_ERROR is returned.  Otherwise TCL_OK
 *	is returned and the new vector is created.
 *
 * Side Effects:
 *	Memory will be allocated for the new vector.  A new TCL command and
 *	Tcl array variable will be created.
 *
 *---------------------------------------------------------------------------
 */

/*LINTLIBRARY*/
int
Blt_CreateVector2(
    Tcl_Interp* interp,
    const char *vecName, const char *cmdName, const char *varName,
    int initialSize,
    Blt_Vector **vecPtrPtr)
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */
    Vector *vPtr;
    int isNew;
    char *nameCopy;

    if (initialSize < 0) {
	Tcl_AppendResult(interp, "bad vector size \"", Blt_Itoa(initialSize),
	    "\"", (char *)NULL);
	return TCL_ERROR;
    }
    dataPtr = Blt_Vec_GetInterpData(interp);

    nameCopy = Blt_Strdup(vecName);
    vPtr = Blt_Vec_Create(dataPtr, nameCopy, cmdName, varName, &isNew);
    free(nameCopy);

    if (vPtr == NULL) {
	return TCL_ERROR;
    }
    if (initialSize > 0) {
	if (Blt_Vec_ChangeLength(interp, vPtr, initialSize) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (vecPtrPtr != NULL) {
	*vecPtrPtr = (Blt_Vector *) vPtr;
    }
    return TCL_OK;
}

int
Blt_CreateVector(
    Tcl_Interp* interp,
    const char *name,
    int size,
    Blt_Vector **vecPtrPtr)
{
    return Blt_CreateVector2(interp, name, name, name, size, vecPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteVector --
 *
 *	Deletes the vector of the given name.  All clients with designated
 *	callback routines will be notified.
 *
 * Results:
 *	A standard TCL result.  If no vector exists by that name, TCL_ERROR is
 *	returned.  Otherwise TCL_OK is returned and vector is deleted.
 *
 * Side Effects:
 *	Memory will be released for the new vector.  Both the TCL command and
 *	array variable will be deleted.  All clients which set call back
 *	procedures will be notified.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_DeleteVector(Blt_Vector *vecPtr)
{
    Vector *vPtr = (Vector *)vecPtr;

    Blt_Vec_Free(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteVectorByName --
 *
 *	Deletes the vector of the given name.  All clients with designated
 *	callback routines will be notified.
 *
 * Results:
 *	A standard TCL result.  If no vector exists by that name, TCL_ERROR is
 *	returned.  Otherwise TCL_OK is returned and vector is deleted.
 *
 * Side Effects:
 *	Memory will be released for the new vector.  Both the TCL command and
 *	array variable will be deleted.  All clients which set call back
 *	procedures will be notified.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_DeleteVectorByName(Tcl_Interp* interp, const char *name)
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */
    Vector *vPtr;
    char *nameCopy;
    int result;

    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_Vec_ParseElement routine will segfault when it tries to write into
     * the string.  Therefore make a writable copy and free it when we're
     * done.
     */
    nameCopy = Blt_Strdup(name);
    dataPtr = Blt_Vec_GetInterpData(interp);
    result = Blt_Vec_LookupName(dataPtr, nameCopy, &vPtr);
    free(nameCopy);

    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_Vec_Free(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorExists2 --
 *
 *	Returns whether the vector associated with the client token still
 *	exists.
 *
 * Results:
 *	Returns 1 is the vector still exists, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_VectorExists2(Tcl_Interp* interp, const char *vecName)
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */

    dataPtr = Blt_Vec_GetInterpData(interp);
    if (GetVectorObject(dataPtr, vecName, NS_SEARCH_BOTH) != NULL) {
	return TRUE;
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorExists --
 *
 *	Returns whether the vector associated with the client token
 *	still exists.
 *
 * Results:
 *	Returns 1 is the vector still exists, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_VectorExists(Tcl_Interp* interp, const char *vecName)
{
    char *nameCopy;
    int result;

    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_VectorParseName routine will segfault when it tries to write into
     * the string.  Therefore make a writable copy and free it when we're
     * done.
     */
    nameCopy = Blt_Strdup(vecName);
    result = Blt_VectorExists2(interp, nameCopy);
    free(nameCopy);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVector --
 *
 *	Returns a pointer to the vector associated with the given name.
 *
 * Results:
 *	A standard TCL result.  If there is no vector "name", TCL_ERROR is
 *	returned.  Otherwise TCL_OK is returned and vecPtrPtr will point to
 *	the vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetVector(Tcl_Interp* interp, const char *name, Blt_Vector **vecPtrPtr)
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */
    Vector *vPtr;
    char *nameCopy;
    int result;

    dataPtr = Blt_Vec_GetInterpData(interp);
    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_VectorParseName routine will segfault when it tries to write into
     * the string.  Therefore make a writable copy and free it when we're
     * done.
     */
    nameCopy = Blt_Strdup(name);
    result = Blt_Vec_LookupName(dataPtr, nameCopy, &vPtr);
    free(nameCopy);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_Vec_UpdateRange(vPtr);
    *vecPtrPtr = (Blt_Vector *) vPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVectorFromObj --
 *
 *	Returns a pointer to the vector associated with the given name.
 *
 * Results:
 *	A standard TCL result.  If there is no vector "name", TCL_ERROR
 *	is returned.  Otherwise TCL_OK is returned and vecPtrPtr will
 *	point to the vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetVectorFromObj(
    Tcl_Interp* interp,
    Tcl_Obj *objPtr,
    Blt_Vector **vecPtrPtr)
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */
    Vector *vPtr;

    dataPtr = Blt_Vec_GetInterpData(interp);
    if (Blt_Vec_LookupName(dataPtr, Tcl_GetString(objPtr), &vPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_Vec_UpdateRange(vPtr);
    *vecPtrPtr = (Blt_Vector *) vPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResetVector --
 *
 *	Resets the vector data.  This is called by a client to indicate that
 *	the vector data has changed.  The vector does not need to point to
 *	different memory.  Any clients of the vector will be notified of the
 *	change.
 *
 * Results:
 *	A standard TCL result.  If the new array size is invalid,
 *	TCL_ERROR is returned.  Otherwise TCL_OK is returned and the
 *	new vector data is recorded.
 *
 * Side Effects:
 *	Any client designated callbacks will be posted.  Memory may
 *	be changed for the vector array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ResetVector(
    Blt_Vector *vecPtr,
    double *valueArr,		/* Array containing the elements of the
				 * vector. If NULL, indicates to reset the 
				 * vector.*/
    int length,			/* The number of elements that the vector 
				 * currently holds. */
    int size,			/* The maximum number of elements that the
				 * array can hold. */
    Tcl_FreeProc *freeProc)	/* Address of memory deallocation routine
				 * for the array of values.  Can also be
				 * TCL_STATIC, TCL_DYNAMIC, or TCL_VOLATILE. */
{
    Vector *vPtr = (Vector *)vecPtr;

    if (size < 0) {
	Tcl_AppendResult(vPtr->interp, "bad array size", (char *)NULL);
	return TCL_ERROR;
    }
    return Blt_Vec_Reset(vPtr, valueArr, length, size, freeProc);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResizeVector --
 *
 *	Changes the size of the vector.  All clients with designated callback
 *	routines will be notified of the size change.
 *
 * Results:
 *	A standard TCL result.  If no vector exists by that name, TCL_ERROR is
 *	returned.  Otherwise TCL_OK is returned and vector is resized.
 *
 * Side Effects:
 *	Memory may be reallocated for the new vector size.  All clients which
 *	set call back procedures will be notified.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ResizeVector(Blt_Vector *vecPtr, int length)
{
    Vector *vPtr = (Vector *)vecPtr;

    if (Blt_Vec_ChangeLength((Tcl_Interp *)NULL, vPtr, length) != TCL_OK) {
	Tcl_AppendResult(vPtr->interp, "can't resize vector \"", vPtr->name,
	    "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_AllocVectorId --
 *
 *	Creates an identifier token for an existing vector.  The identifier is
 *	used by the client routines to get call backs when (and if) the vector
 *	changes.
 *
 * Results:
 *	A standard TCL result.  If "vecName" is not associated with a vector,
 *	TCL_ERROR is returned and interp->result is filled with an error
 *	message.
 *
 *---------------------------------------------------------------------------
 */
Blt_VectorId
Blt_AllocVectorId(Tcl_Interp* interp, const char *name)
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */
    Vector *vPtr;
    VectorClient *clientPtr;
    Blt_VectorId clientId;
    int result;
    char *nameCopy;

    dataPtr = Blt_Vec_GetInterpData(interp);
    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_VectorParseName routine will segfault when it tries to write into
     * the string.  Therefore make a writable copy and free it when we're
     * done.
     */
    nameCopy = Blt_Strdup(name);
    result = Blt_Vec_LookupName(dataPtr, nameCopy, &vPtr);
    free(nameCopy);

    if (result != TCL_OK) {
	return (Blt_VectorId) 0;
    }
    /* Allocate a new client structure */
    clientPtr = calloc(1, sizeof(VectorClient));
    clientPtr->magic = VECTOR_MAGIC;

    /* Add the new client to the server's list of clients */
    clientPtr->link = Blt_Chain_Append(vPtr->chain, clientPtr);
    clientPtr->serverPtr = vPtr;
    clientId = (Blt_VectorId) clientPtr;
    return clientId;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_SetVectorChangedProc --
 *
 *	Sets the routine to be called back when the vector is changed or
 *	deleted.  *clientData* will be provided as an argument. If *proc* is
 *	NULL, no callback will be made.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The designated routine will be called when the vector is changed
 *	or deleted.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_SetVectorChangedProc(
    Blt_VectorId clientId,	/* Client token identifying the vector */
    Blt_VectorChangedProc *proc,/* Address of routine to call when the contents
				 * of the vector change. If NULL, no routine
				 * will be called */
    ClientData clientData)	/* One word of information to pass along when
				 * the above routine is called */
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if (clientPtr->magic != VECTOR_MAGIC) {
	return;			/* Not a valid token */
    }
    clientPtr->clientData = clientData;
    clientPtr->proc = proc;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeVectorId --
 *
 *	Releases the token for an existing vector.  This indicates that the
 *	client is no longer interested the vector.  Any previously specified
 *	callback routine will no longer be invoked when (and if) the vector
 *	changes.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Any previously specified callback routine will no longer be
 *	invoked when (and if) the vector changes.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeVectorId(Blt_VectorId clientId)
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if (clientPtr->magic != VECTOR_MAGIC) {
	return;			/* Not a valid token */
    }
    if (clientPtr->serverPtr != NULL) {
	/* Remove the client from the server's list */
	Blt_Chain_DeleteLink(clientPtr->serverPtr->chain, clientPtr->link);
    }
    free(clientPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NameOfVectorId --
 *
 *	Returns the name of the vector (and array variable).
 *
 * Results:
 *	The name of the array variable is returned.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_NameOfVectorId(Blt_VectorId clientId) 
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if ((clientPtr->magic != VECTOR_MAGIC) || (clientPtr->serverPtr == NULL)) {
	return NULL;
    }
    return clientPtr->serverPtr->name;
}

const char *
Blt_NameOfVector(Blt_Vector *vecPtr) /* Vector to query. */
{
    Vector *vPtr = (Vector *)vecPtr;
    return vPtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorNotifyPending --
 *
 *	Returns the name of the vector (and array variable).
 *
 * Results:
 *	The name of the array variable is returned.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_VectorNotifyPending(Blt_VectorId clientId)
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if ((clientPtr == NULL) || (clientPtr->magic != VECTOR_MAGIC) || 
	(clientPtr->serverPtr == NULL)) {
	return 0;
    }
    return (clientPtr->serverPtr->notifyFlags & NOTIFY_PENDING);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVectorById --
 *
 *	Returns a pointer to the vector associated with the client
 *	token.
 *
 * Results:
 *	A standard TCL result.  If the client token is not associated
 *	with a vector any longer, TCL_ERROR is returned. Otherwise,
 *	TCL_OK is returned and vecPtrPtr will point to vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetVectorById(
    Tcl_Interp* interp,
    Blt_VectorId clientId,	/* Client token identifying the vector */
    Blt_Vector **vecPtrPtr)
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if (clientPtr->magic != VECTOR_MAGIC) {
	Tcl_AppendResult(interp, "bad vector token", (char *)NULL);
	return TCL_ERROR;
    }
    if (clientPtr->serverPtr == NULL) {
	Tcl_AppendResult(interp, "vector no longer exists", (char *)NULL);
	return TCL_ERROR;
    }
    Blt_Vec_UpdateRange(clientPtr->serverPtr);
    *vecPtrPtr = (Blt_Vector *) clientPtr->serverPtr;
    return TCL_OK;
}

/*LINTLIBRARY*/
void
Blt_InstallIndexProc(Tcl_Interp* interp, const char *string, 
		     Blt_VectorIndexProc *procPtr) 
{
    VectorInterpData *dataPtr;	/* Interpreter-specific data. */
    Tcl_HashEntry *hPtr;
    int isNew;

    dataPtr = Blt_Vec_GetInterpData(interp);
    hPtr = Tcl_CreateHashEntry(&dataPtr->indexProcTable, string, &isNew);
    if (procPtr == NULL) {
	Tcl_DeleteHashEntry(hPtr);
    } else {
	Tcl_SetHashValue(hPtr, procPtr);
    }
}

/* spinellia@acm.org START */


#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

/* routine by Brenner
 * data is the array of complex data points, perversely
 * starting at 1
 * nn is the number of complex points, i.e. half the length of data
 * isign is 1 for forward, -1 for inverse
 */
static void 
four1(double *data, unsigned long nn, int isign)
{
    unsigned long n,mmax,m,j,istep,i;
    double wtemp,wr,wpr,wpi,wi,theta;
    double tempr,tempi;
    
    n=nn << 1;
    j=1;
    for (i = 1;i<n;i+=2) {
	if (j > i) {
	    SWAP(data[j],data[i]);
	    SWAP(data[j+1],data[i+1]);
	}
	m=n >> 1;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }
    mmax=2;
    while (n > mmax) {
	istep=mmax << 1;
	theta=isign*(6.28318530717959/mmax);
	wtemp=sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0;
	wi=0.0;
	for (m=1;m<mmax;m+=2) {
	    for (i=m;i<=n;i+=istep) {
		j=i+mmax;
		tempr=wr*data[j]-wi*data[j+1];
		tempi=wr*data[j+1]+wi*data[j];
		data[j]=data[i]-tempr;
		data[j+1]=data[i+1]-tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr=(wtemp=wr)*wpr-wi*wpi+wr;
	    wi=wi*wpr+wtemp*wpi+wi;
	}
	mmax=istep;
    }
}
#undef SWAP

static int 
smallest_power_of_2_not_less_than(int x)
{
    int pow2 = 1;

    while (pow2 < x){
	pow2 <<= 1;
    }
    return pow2;
}


int
Blt_Vec_FFT(
    Tcl_Interp* interp,		/* Interpreter to report errors to */
    Vector *realPtr,	/* If non-NULL, indicates to compute and
				   store the real values in this vector.  */
    Vector *phasesPtr,	/* If non-NULL, indicates to compute
				 * and store the imaginary values in
				 * this vector. */
    Vector *freqPtr,	/* If non-NULL, indicates to compute
				 * and store the frequency values in
				 * this vector.  */
    double delta,		/*  */
    int flags,			/* Bit mask representing various
				 * flags: FFT_NO_constANT,
				 * FFT_SPECTRUM, and FFT_BARTLETT. */
    Vector *srcPtr) 
{
    int length;
    int pow2len;
    double *paddedData;
    int i;
    double Wss = 0.0;
    /* TENTATIVE */
    int middle = 1;
    int noconstant;

    noconstant = (flags & FFT_NO_CONSTANT) ? 1 : 0;

    /* Length of the original vector. */
    length = srcPtr->last - srcPtr->first + 1;
    /* new length */
    pow2len = smallest_power_of_2_not_less_than( length );

    /* We do not do in-place FFTs */
    if (realPtr == srcPtr) {
	Tcl_AppendResult(interp, "real vector \"", realPtr->name, 
		 "\" can't be the same as the source", (char *)NULL);
	return TCL_ERROR;
    }
    if (phasesPtr != NULL) {
	if (phasesPtr == srcPtr) {
	    Tcl_AppendResult(interp, "imaginary vector \"", phasesPtr->name, 
			"\" can't be the same as the source", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Blt_Vec_ChangeLength(interp, phasesPtr, 
		pow2len/2-noconstant+middle) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (freqPtr != NULL) {
	if (freqPtr == srcPtr) {
	    Tcl_AppendResult(interp, "frequency vector \"", freqPtr->name, 
		     "\" can't be the same as the source", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Blt_Vec_ChangeLength(interp, freqPtr, 
			   pow2len/2-noconstant+middle) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    /* Allocate memory zero-filled array. */
    paddedData = calloc(pow2len * 2, sizeof(double));
    if (paddedData == NULL) {
	Tcl_AppendResult(interp, "can't allocate memory for padded data",
		 (char *)NULL);
	return TCL_ERROR;
    }
    
    /*
     * Since we just do real transforms, only even locations will be
     * filled with data.
     */
    if (flags & FFT_BARTLETT) {	/* Bartlett window 1 - ( (x - N/2) / (N/2) ) */
	double Nhalf = pow2len*0.5;
	double Nhalf_1 = 1.0 / Nhalf;
	double w;

	for (i = 0; i < length; i++) {
	    w = 1.0 - fabs( (i-Nhalf) * Nhalf_1 );
	    Wss += w;
	    paddedData[2*i] = w * srcPtr->valueArr[i];
	}
	for(/*empty*/; i < pow2len; i++) {
	    w = 1.0 - fabs((i-Nhalf) * Nhalf_1);
	    Wss += w;
	}
    } else {			/* Squared window, i.e. no data windowing. */
	for (i = 0; i < length; i++) { 
	    paddedData[2*i] = srcPtr->valueArr[i]; 
	}
	Wss = pow2len;
    }
    
    /* Fourier */
    four1(paddedData-1, pow2len, 1);
    
    /*
      for(i=0;i<pow2len;i++){
      printf( "(%f %f) ", paddedData[2*i], paddedData[2*i+1] );
      }
    */
    
    /* the spectrum is the modulus of the transforms, scaled by 1/N^2 */
    /* or 1/(N * Wss) for windowed data */
    if (flags & FFT_SPECTRUM) {
	double re, im, reS, imS;
	double factor = 1.0 / (pow2len*Wss);
	double *v = realPtr->valueArr;
	
	for (i = 0 + noconstant; i < pow2len / 2; i++) {
	    re = paddedData[2*i];
	    im = paddedData[2*i+1];
	    reS = paddedData[2*pow2len-2*i-2];
	    imS = paddedData[2*pow2len-2*i-1];
	    v[i - noconstant] = factor * (
# if 0
			  hypot( paddedData[2*i], paddedData[2*i+1] )
			  + hypot(
				  paddedData[pow2len*2-2*i-2],
				  paddedData[pow2len*2-2*i-1]
			)
# else
			  sqrt( re*re + im* im ) + sqrt( reS*reS + imS*imS )
# endif
		   );
	}
    } else {
	for(i = 0 + noconstant; i < pow2len / 2 + middle; i++) {
	    realPtr->valueArr[i - noconstant] = paddedData[2*i];
	}
    }
    if( phasesPtr != NULL ){
        for (i = 0 + noconstant; i < pow2len / 2 + middle; i++) {
	    phasesPtr->valueArr[i-noconstant] = paddedData[2*i+1];
	}
    }
    
    /* Compute frequencies */
    if (freqPtr != NULL) {
        double N = pow2len;
	double denom = 1.0 / N / delta;
        for( i=0+noconstant; i<pow2len/2+middle; i++ ){
	    freqPtr->valueArr[i-noconstant] = ((double) i) * denom;
	}
    }
    
    /* Memory is necessarily dynamic, because nobody touched it ! */
    free(paddedData);
    
    realPtr->offset = 0;
    return TCL_OK;
}


int
Blt_Vec_InverseFFT(Tcl_Interp* interp, Vector *srcImagPtr, Vector *destRealPtr, 
    Vector *destImagPtr, Vector *srcPtr)
{
    int length;
    int pow2len;
    double *paddedData;
    int i;
    double oneOverN;

    if ((destRealPtr == srcPtr) || (destImagPtr == srcPtr )){
/* we do not do in-place FFTs */
	return TCL_ERROR;
    }
    length = srcPtr->last - srcPtr->first + 1;

/* minus one because of the magical middle element! */
    pow2len = smallest_power_of_2_not_less_than( (length-1)*2 );
    oneOverN = 1.0 / pow2len;

    if (Blt_Vec_ChangeLength(interp, destRealPtr, pow2len) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_Vec_ChangeLength(interp, destImagPtr, pow2len) != TCL_OK) {
	return TCL_ERROR;
    }

    if( length != (srcImagPtr->last - srcImagPtr->first + 1) ){
	Tcl_AppendResult(srcPtr->interp,
		"the length of the imagPart vector must ",
		"be the same as the real one", (char *)NULL);
	return TCL_ERROR;
    }

    paddedData = malloc( pow2len*2*sizeof(double) );
    if( paddedData == NULL ){
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "memory allocation failed", (char *)NULL);
	}
	return TCL_ERROR;
    }
    for(i=0;i<pow2len*2;i++) { paddedData[i] = 0.0; }
    for(i=0;i<length-1;i++){
	paddedData[2*i] = srcPtr->valueArr[i];
	paddedData[2*i+1] = srcImagPtr->valueArr[i];
	paddedData[pow2len*2 - 2*i - 2 ] = srcPtr->valueArr[i+1];
	paddedData[pow2len*2 - 2*i - 1 ] = - srcImagPtr->valueArr[i+1];
    }
/* mythical middle element */
    paddedData[(length-1)*2] = srcPtr->valueArr[length-1];
    paddedData[(length-1)*2+1] = srcImagPtr->valueArr[length-1];

/*
for(i=0;i<pow2len;i++){
	printf( "(%f %f) ", paddedData[2*i], paddedData[2*i+1] );
}
 */

/* fourier */
    four1( paddedData-1, pow2len, -1 );

/* put values in their places, normalising by 1/N */
    for(i=0;i<pow2len;i++){
	destRealPtr->valueArr[i] = paddedData[2*i] * oneOverN;
	destImagPtr->valueArr[i] = paddedData[2*i+1] * oneOverN;
    }

/* memory is necessarily dynamic, because nobody touched it ! */
    free( paddedData );

    return TCL_OK;
}


/* spinellia@acm.org STOP */



static double
FindSplit(Point2d *points, int i, int j, int *split)	
{    
    double maxDist2;
    
    maxDist2 = -1.0;
    if ((i + 1) < j) {
	int k;
	double a, b, c;	

	/* 
	 * 
	 *  dist2 P(k) =  |  1  P(i).x  P(i).y  |
	 *		  |  1  P(j).x  P(j).y  |
	 *                |  1  P(k).x  P(k).y  |
	 *       ------------------------------------------
 	 *       (P(i).x - P(j).x)^2 + (P(i).y - P(j).y)^2
	 */

	a = points[i].y - points[j].y;
	b = points[j].x - points[i].x;
	c = (points[i].x * points[j].y) - (points[i].y * points[j].x);
	for (k = (i + 1); k < j; k++) {
	    double dist2;

	    dist2 = (points[k].x * a) + (points[k].y * b) + c;
	    if (dist2 < 0.0) {
		dist2 = -dist2;	
	    }
	    if (dist2 > maxDist2) {
		maxDist2 = dist2;	/* Track the maximum. */
		*split = k;
	    }
	}
	/* Correction for segment length---should be redone if can == 0 */
	maxDist2 *= maxDist2 / (a * a + b * b);
    } 
    return maxDist2;
}


/* Douglas-Peucker line simplification algorithm */
int
Blt_SimplifyLine(Point2d *inputPts, int low, int high, double tolerance,
		 int *indices)
{
#define StackPush(a)	s++, stack[s] = (a)
#define StackPop(a)	(a) = stack[s], s--
#define StackEmpty()	(s < 0)
#define StackTop()	stack[s]
    int *stack;
    int split = -1; 
    double dist2, tolerance2;
    int s = -1;			/* Points to top stack item. */
    int count;

    stack = malloc(sizeof(int) * (high - low + 1));
    StackPush(high);
    count = 0;
    indices[count++] = 0;
    tolerance2 = tolerance * tolerance;
    while (!StackEmpty()) {
	dist2 = FindSplit(inputPts, low, StackTop(), &split);
	if (dist2 > tolerance2) {
	    StackPush(split);
	} else {
	    indices[count++] = StackTop();
	    StackPop(low);
	}
    } 
    free(stack);
    return count;
}
