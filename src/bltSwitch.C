/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltSwitch.c --
 *
 * This module implements command/argument switch parsing procedures for the
 * BLT toolkit.
 *
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

#include <assert.h>
#include <stdarg.h>

#include "bltInt.h"
#include "bltSwitch.h"

static void
DoHelp(Tcl_Interp *interp, Blt_SwitchSpec *specs)
{
    Tcl_DString ds;
    Blt_SwitchSpec *sp;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, "following switches are available:", -1);
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
	Tcl_DStringAppend(&ds, "\n    ", 4);
	Tcl_DStringAppend(&ds, sp->switchName, -1);
	Tcl_DStringAppend(&ds, " ", 1);
	Tcl_DStringAppend(&ds, sp->help, -1);
    }
    Tcl_AppendResult(interp, Tcl_DStringValue(&ds), (char *)NULL);
    Tcl_DStringFree(&ds);
}

/*
 *---------------------------------------------------------------------------
 *
 * FindSwitchSpec --
 *
 *	Search through a table of configuration specs, looking for one that
 *	matches a given argvName.
 *
 * Results:
 *	The return value is a pointer to the matching entry, or NULL if
 *	nothing matched.  In that case an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_SwitchSpec *
FindSwitchSpec(
    Tcl_Interp *interp,		/* Used for reporting errors. */
    Blt_SwitchSpec *specs,	/* Pointer to table of configuration
				 * specifications for a widget. */
    const char *name,		/* Name identifying a particular switch. */
    int length,			/* Length of name. */
    int needFlags,		/* Flags that must be present in matching
				 * entry. */
    int hateFlags)		/* Flags that must NOT be present in matching
				 * entry. */
{
    Blt_SwitchSpec *sp;
    char c;			/* First character of current argument. */
    Blt_SwitchSpec *matchPtr;	/* Matching spec, or NULL. */

    c = name[1];
    matchPtr = NULL;
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
	if (sp->switchName == NULL) {
	    continue;
	}
	if (((sp->flags & needFlags) != needFlags) || (sp->flags & hateFlags)) {
	    continue;
	}
	if ((sp->switchName[1] != c) || 
	    (strncmp(sp->switchName, name, length) != 0)) {
	    continue;
	}
	if (sp->switchName[length] == '\0') {
	    return sp;		/* Stop on a perfect match. */
	}
	if (matchPtr != NULL) {
	    Tcl_AppendResult(interp, "ambiguous switch \"", name, "\"\n", 
		(char *) NULL);
	    DoHelp(interp, specs);
	    return NULL;
	}
	matchPtr = sp;
    }
    if (strcmp(name, "-help") == 0) {
	DoHelp(interp, specs);
	return NULL;
    }
    if (matchPtr == NULL) {
	Tcl_AppendResult(interp, "unknown switch \"", name, "\"\n", 
			 (char *)NULL);
	DoHelp(interp, specs);
	return NULL;
    }
    return matchPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DoSwitch --
 *
 *	This procedure applies a single configuration switch to a widget
 *	record.
 *
 * Results:
 *	A standard TCL return value.
 *
 * Side effects:
 *	WidgRec is modified as indicated by specPtr and value.  The old value
 *	is recycled, if that is appropriate for the value type.
 *
 *---------------------------------------------------------------------------
 */
static int
DoSwitch(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Blt_SwitchSpec *sp,		/* Specifier to apply. */
    Tcl_Obj *objPtr,		/* Value to use to fill in widgRec. */
    void *record)		/* Record whose fields are to be modified.
				 * Values must be properly initialized. */
{
    do {
	char *ptr;

	ptr = (char *)record + sp->offset;
	switch (sp->type) {
	case BLT_SWITCH_BOOLEAN:
	    {
		int bool;

		if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (sp->mask > 0) {
		    if (bool) {
			*((int *)ptr) |= sp->mask;
		    } else {
			*((int *)ptr) &= ~sp->mask;
		    }
		} else {
		    *((int *)ptr) = bool;
		}
	    }
	    break;

	case BLT_SWITCH_DOUBLE:
	    if (Tcl_GetDoubleFromObj(interp, objPtr, (double *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_SWITCH_OBJ:
	    Tcl_IncrRefCount(objPtr);
	    *(Tcl_Obj **)ptr = objPtr;
	    break;

	case BLT_SWITCH_FLOAT:
	    {
		double value;

		if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(float *)ptr = (float)value;
	    }
	    break;

	case BLT_SWITCH_INT:
	    if (Tcl_GetIntFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_SWITCH_INT_NNEG:
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_NNEG, 
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = (int)value;
	    }
	    break;

	case BLT_SWITCH_INT_POS:
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_POS, 
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = (int)value;
	    }
	    break;

	case BLT_SWITCH_LIST:
	    {
		int argc;

		if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &argc, 
				  (const char ***)ptr) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    break;

	case BLT_SWITCH_LONG:
	    if (Tcl_GetLongFromObj(interp, objPtr, (long *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_SWITCH_LONG_NNEG:
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_NNEG, 
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(long *)ptr = value;
	    }
	    break;

	case BLT_SWITCH_LONG_POS:
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_POS, &value)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
		*(long *)ptr = value;
	    }
	    break;

	case BLT_SWITCH_STRING: 
	    {
		char *value;
		
		value = Tcl_GetString(objPtr);
		value =  (*value == '\0') ?  NULL : Blt_Strdup(value);
		if (*(char **)ptr != NULL) {
		    free(*(char **)ptr);
		}
		*(char **)ptr = value;
	    }
	    break;

	case BLT_SWITCH_CUSTOM:
	    assert(sp->customPtr != NULL);
	    if ((*sp->customPtr->parseProc)(sp->customPtr->clientData, interp,
		sp->switchName, objPtr, (char *)record, sp->offset, sp->flags) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	default: 
	    Tcl_AppendResult(interp, "bad switch table: unknown type \"",
		 Blt_Itoa(sp->type), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	sp++;
    } while ((sp->switchName == NULL) && (sp->type != BLT_SWITCH_END));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ParseSwitches --
 *
 *	Process command-line switches to fill in fields of a record with
 *	resources and other parameters.
 *
 * Results:
 *	Returns the number of arguments comsumed by parsing the command line.
 *	If an error occurred, -1 will be returned and an error messages can be
 *	found as the interpreter result.
 *
 * Side effects:
 *	The fields of widgRec get filled in with information from argc/argv.
 *	Old information in widgRec's fields gets recycled.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ParseSwitches(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Blt_SwitchSpec *specs,	/* Describes legal switches. */
    int objc,			/* Number of elements in argv. */
    Tcl_Obj *const *objv,	/* Command-line switches. */
    void *record,		/* Record whose fields are to be modified.
				 * Values must be properly initialized. */
    int flags)			/* Used to specify additional flags that must
				 * be present in switch specs for them to be
				 * considered.  */
{
    Blt_SwitchSpec *sp;
    int count;
    int needFlags;		/* Specs must contain this set of flags or
				 * else they are not considered. */
    int hateFlags;		/* If a spec contains any bits here, it's not
				 * considered. */

    needFlags = flags & ~(BLT_SWITCH_USER_BIT - 1);
    hateFlags = 0;

    /*
     * Pass 1:  Clear the change flags on all the specs so that we 
     *          can check it later.
     */
    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
	sp->flags &= ~BLT_SWITCH_SPECIFIED;
    }
    /*
     * Pass 2:  Process the arguments that match entries in the specs.
     *		It's an error if the argument doesn't match anything.
     */
    for (count = 0; count < objc; count++) {
	char *arg;
	int length;

	arg = Tcl_GetStringFromObj(objv[count], &length);
	if (flags & BLT_SWITCH_OBJV_PARTIAL) {
	    /* 
	     * If the argument doesn't start with a '-' (not a switch) or is
	     * '--', stop processing and return the number of arguments
	     * comsumed.
	     */
	    if (arg[0] != '-') {
		return count;
	    }
	    if ((arg[1] == '-') && (arg[2] == '\0')) {
		return count + 1; /* include the "--" in the count. */
	    }
	}
	sp = FindSwitchSpec(interp, specs, arg, length, needFlags, hateFlags);
	if (sp == NULL) {
	    return -1;
	}
	if (sp->type == BLT_SWITCH_BITMASK) {
	    char *ptr;

	    ptr = (char *)record + sp->offset;
	    *((int *)ptr) |= sp->mask;
	} else if (sp->type == BLT_SWITCH_BITMASK_INVERT) {
	    char *ptr;
	    
	    ptr = (char *)record + sp->offset;
	    *((int *)ptr) &= ~sp->mask;
	} else if (sp->type == BLT_SWITCH_VALUE) {
	    char *ptr;
	    
	    ptr = (char *)record + sp->offset;
	    *((int *)ptr) = sp->mask;
	} else {
	    count++;
	    if (count == objc) {
		Tcl_AppendResult(interp, "value for \"", arg, "\" missing", 
				 (char *) NULL);
		return -1;
	    }
	    if (DoSwitch(interp, sp, objv[count], record) != TCL_OK) {
		char msg[200];

		sprintf_s(msg, 200, "\n    (processing \"%.40s\" switch)", 
			sp->switchName);
		Tcl_AddErrorInfo(interp, msg);
		return -1;
	    }
	}
	sp->flags |= BLT_SWITCH_SPECIFIED;
    }
    return count;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeSwitches --
 *
 *	Free up all resources associated with switches.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
void
Blt_FreeSwitches(
    Blt_SwitchSpec *specs,	/* Describes legal switches. */
    void *record,		/* Record whose fields contain current values
				 * for switches. */
    int needFlags)		/* Used to specify additional flags that must
				 * be present in config specs for them to be
				 * considered. */
{
    Blt_SwitchSpec *sp;

    for (sp = specs; sp->type != BLT_SWITCH_END; sp++) {
	if ((sp->flags & needFlags) == needFlags) {
	    char *ptr;

	    ptr = (char *)record + sp->offset;
	    switch (sp->type) {
	    case BLT_SWITCH_STRING:
	    case BLT_SWITCH_LIST:
		if (*((char **) ptr) != NULL) {
		    free(*((char **) ptr));
		    *((char **) ptr) = NULL;
		}
		break;

	    case BLT_SWITCH_OBJ:
		if (*((Tcl_Obj **) ptr) != NULL) {
		    Tcl_DecrRefCount(*((Tcl_Obj **)ptr));
		    *((Tcl_Obj **) ptr) = NULL;
		}
		break;

	    case BLT_SWITCH_CUSTOM:
		assert(sp->customPtr != NULL);
		if ((*(char **)ptr != NULL) && 
		    (sp->customPtr->freeProc != NULL)) {
		    (*sp->customPtr->freeProc)((char *)record, sp->offset, 
			sp->flags);
		}
		break;

	    default:
		break;
	    }
	}
    }
}

int 
Blt_ExprDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr)
{
    /* First try to extract the value as a double precision number. */
    if (Tcl_GetDoubleFromObj((Tcl_Interp *)NULL, objPtr, valuePtr) == TCL_OK) {
	return TCL_OK;
    }
    /* Then try to parse it as an expression. */
    if (Tcl_ExprDouble(interp, Tcl_GetString(objPtr), valuePtr) == TCL_OK) {
	return TCL_OK;
    }
    return TCL_ERROR;
}

int 
Blt_ExprIntFromObj(
    Tcl_Interp *interp, 
    Tcl_Obj *objPtr, 
    int *valuePtr)
{
    long lvalue;

    /* First try to extract the value as a simple integer. */
    if (Tcl_GetIntFromObj((Tcl_Interp *)NULL, objPtr, valuePtr) == TCL_OK) {
	return TCL_OK;
    }
    /* Otherwise try to parse it as an expression. */
    if (Tcl_ExprLong(interp, Tcl_GetString(objPtr), &lvalue) == TCL_OK) {
	*valuePtr = lvalue;
	return TCL_OK;
    }
    return TCL_ERROR;
}

