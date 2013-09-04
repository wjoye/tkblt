
/* 
 * bltConfig.c --
 *
 * This file contains a Tcl_Obj based replacement for the widget
 * configuration functions in Tk.
 *
 *	Copyright (c) 1990-1994 The Regents of the University of California.
 *	Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 *	See the file "license.terms" for information on usage and redistribution
 *	of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *	Copyright 2003-2004 George A Howlett.
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

/*
 * This is a Tcl_Obj based replacement for the widget configuration
 * functions in Tk.  
 *
 * What not use the new Tk_Option interface?
 *
 *	There were design changes in the new Tk_Option interface that
 *	make it unwieldy.  
 *
 *	o You have to dynamically allocate, store, and deallocate
 *	  your option table.  
 *      o The Tk_FreeConfigOptions routine requires a tkwin argument.
 *	  Unfortunately, most widgets save the display pointer and 
 *	  de-reference their tkwin when the window is destroyed.  
 *	o There's no TK_CONFIG_CUSTOM functionality.  This means that
 *	  save special options must be saved as strings by 
 *	  Tk_ConfigureWidget and processed later, thus losing the 
 *	  benefits of Tcl_Objs.  It also make error handling 
 *	  problematic, since you don't pick up certain errors like 
 *	  
 *	    .widget configure -myoption bad -myoption good
 *        
 *	  You will never see the first "bad" value.
 *	o Especially compared to the former Tk_ConfigureWidget calls,
 *	  the new interface is overly complex.  If there was a big
 *	  performance win, it might be worth the effort.  But let's 
 *	  face it, this biggest wins are in processing custom options
 *	  values with thousands of elements.  Most common resources 
 *	  (font, color, etc) have string tokens anyways.
 *
 *	On the other hand, the replacement functions in this file fell
 *	into place quite easily both from the aspect of API writer and
 *	user.  The biggest benefit is that you don't need to change lots
 *	of working code just to get the benefits of Tcl_Objs.
 * 
 */

#include "bltInt.h"

#include <stdarg.h>
#include "bltFont.h"
#include "bltPicture.h"
#include "bltBgStyle.h"

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPositionFromObj --
 *
 *	Convert a string representing a numeric position.
 *	A position can be in one of the following forms.
 *
 * 	  number	- number of the item in the hierarchy, indexed
 *			  from zero.
 *	  "end"		- last position in the hierarchy.
 *
 * Results:
 *	A standard TCL result.  If "string" is a valid index, then
 *	*indexPtr is filled with the corresponding numeric index.
 *	If "end" was selected then *indexPtr is set to -1.
 *	Otherwise an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPositionFromObj(
    Tcl_Interp *interp,		/* Interpreter to report results back
				 * to. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representation of the index.
				 * Can be an integer or "end" to refer
				 * to the last index. */
    long *indexPtr)		/* Holds the converted index. */
{
    const char *string;

    string = Tcl_GetString(objPtr);
    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
	*indexPtr = -1;		/* Indicates last position in hierarchy. */
    } else {
	long position;

	if (Tcl_GetLongFromObj(interp, objPtr, &position) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (position < 0) {
	    Tcl_AppendResult(interp, "bad position \"", string, "\"",
		(char *)NULL);
	    return TCL_ERROR;
	}
	*indexPtr = position;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPixelsFromObj --
 *
 *	Like Tk_GetPixelsFromObj, but checks for negative, zero.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPixelsFromObj(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tcl_Obj *objPtr,
    int check,			/* Can be PIXELS_POS, PIXELS_NNEG,
				 * or PIXELS_ANY, */
    int *valuePtr)
{
    int length;

    if (Tk_GetPixelsFromObj(interp, tkwin, objPtr, &length) != TCL_OK) {
	return TCL_ERROR;
    }
    if (length >= SHRT_MAX) {
	Tcl_AppendResult(interp, "bad distance \"", Tcl_GetString(objPtr), 
		 "\": too big to represent", (char *)NULL);
	return TCL_ERROR;
    }
    switch (check) {
    case PIXELS_NNEG:
	if (length < 0) {
	    Tcl_AppendResult(interp, "bad distance \"", Tcl_GetString(objPtr), 
		     "\": can't be negative", (char *)NULL);
	    return TCL_ERROR;
	}
	break;

    case PIXELS_POS:
	if (length <= 0) {
	    Tcl_AppendResult(interp, "bad distance \"", Tcl_GetString(objPtr), 
		     "\": must be positive", (char *)NULL);
	    return TCL_ERROR;
	}
	break;

    case PIXELS_ANY:
	break;
    }
    *valuePtr = length;
    return TCL_OK;
}

int
Blt_GetPadFromObj(
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Window */
    Tcl_Obj *objPtr,		/* Pixel value string */
    Blt_Pad *padPtr)
{
    int side1, side2;
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc < 1) || (objc > 2)) {
	Tcl_AppendResult(interp, "wrong # elements in padding list",
	    (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_GetPixelsFromObj(interp, tkwin, objv[0], PIXELS_NNEG, 
	     &side1) != TCL_OK) {
	return TCL_ERROR;
    }
    side2 = side1;
    if ((objc > 1) && 
	(Blt_GetPixelsFromObj(interp, tkwin, objv[1], PIXELS_NNEG, 
	      &side2) != TCL_OK)) {
	return TCL_ERROR;
    }
    /* Don't update the pad structure until we know both values are okay. */
    padPtr->side1 = side1;
    padPtr->side2 = side2;
    return TCL_OK;
}

int
Blt_GetStateFromObj(
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tcl_Obj *objPtr,		/* Pixel value string */
    int *statePtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
	*statePtr = STATE_NORMAL;
    } else if ((c == 'd') && (strncmp(string, "disabled", length) == 0)) {
	*statePtr = STATE_DISABLED;
    } else if ((c == 'a') && (strncmp(string, "active", length) == 0)) {
	*statePtr = STATE_ACTIVE;
    } else {
	Tcl_AppendResult(interp, "bad state \"", string,
	    "\": should be normal, active, or disabled", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

const char *
Blt_NameOfState(int state)
{
    switch (state) {
    case STATE_ACTIVE:
	return "active";
    case STATE_DISABLED:
	return "disabled";
    case STATE_NORMAL:
	return "normal";
    default:
	return "???";
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_NameOfFill --
 *
 *	Converts the integer representing the fill style into a string.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_NameOfFill(int fill)
{
    switch (fill) {
    case FILL_X:
	return "x";
    case FILL_Y:
	return "y";
    case FILL_NONE:
	return "none";
    case FILL_BOTH:
	return "both";
    default:
	return "unknown value";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetFillFromObj --
 *
 *	Converts the fill style string into its numeric representation.
 *
 *	Valid style strings are:
 *
 *	  "none"   Use neither plane.
 * 	  "x"	   X-coordinate plane.
 *	  "y"	   Y-coordinate plane.
 *	  "both"   Use both coordinate planes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_GetFillFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *fillPtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	*fillPtr = FILL_NONE;
    } else if ((c == 'x') && (strncmp(string, "x", length) == 0)) {
	*fillPtr = FILL_X;
    } else if ((c == 'y') && (strncmp(string, "y", length) == 0)) {
	*fillPtr = FILL_Y;
    } else if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
	*fillPtr = FILL_BOTH;
    } else {
	Tcl_AppendResult(interp, "bad argument \"", string,
	    "\": should be \"none\", \"x\", \"y\", or \"both\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NameOfResize --
 *
 *	Converts the resize value into its string representation.
 *
 * Results:
 *	Returns a pointer to the static name string.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_NameOfResize(int resize)
{
    switch (resize & RESIZE_BOTH) {
    case RESIZE_NONE:
	return "none";
    case RESIZE_EXPAND:
	return "expand";
    case RESIZE_SHRINK:
	return "shrink";
    case RESIZE_BOTH:
	return "both";
    default:
	return "unknown resize value";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetResizeFromObj --
 *
 *	Converts the resize string into its numeric representation.
 *
 *	Valid style strings are:
 *
 *	  "none"   
 * 	  "expand" 
 *	  "shrink" 
 *	  "both"   
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_GetResizeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *resizePtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	*resizePtr = RESIZE_NONE;
    } else if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
	*resizePtr = RESIZE_BOTH;
    } else if ((c == 'e') && (strncmp(string, "expand", length) == 0)) {
	*resizePtr = RESIZE_EXPAND;
    } else if ((c == 's') && (strncmp(string, "shrink", length) == 0)) {
	*resizePtr = RESIZE_SHRINK;
    } else {
	Tcl_AppendResult(interp, "bad resize argument \"", string,
	    "\": should be \"none\", \"expand\", \"shrink\", or \"both\"",
	    (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetDashesFromObj --
 *
 *	Converts a TCL list of dash values into a dash list ready for
 *	use with XSetDashes.
 *
 * 	A valid list dash values can have zero through 11 elements
 *	(PostScript limit).  Values must be between 1 and 255. Although
 *	a list of 0 (like the empty string) means no dashes.
 *
 * Results:
 *	A standard TCL result. If the list represented a valid dash
 *	list TCL_OK is returned and *dashesPtr* will contain the
 *	valid dash list. Otherwise, TCL_ERROR is returned and
 *	interp->result will contain an error message.
 *
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetDashesFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Blt_Dashes *dashesPtr)
{
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    if (string == NULL) {
	dashesPtr->values[0] = 0;
	return TCL_OK;
    }
    c = string[0];
    if (c == '\0') {
	dashesPtr->values[0] = 0;
    } else if ((c == 'd') && (strcmp(string, "dot") == 0)) {	
	/* 1 */
	dashesPtr->values[0] = 1;
	dashesPtr->values[1] = 0;
    } else if ((c == 'd') && (strcmp(string, "dash") == 0)) {	
	/* 5 2 */
	dashesPtr->values[0] = 5;
	dashesPtr->values[1] = 2;
	dashesPtr->values[2] = 0;
    } else if ((c == 'd') && (strcmp(string, "dashdot") == 0)) { 
	/* 2 4 2 */
 	dashesPtr->values[0] = 2;
	dashesPtr->values[1] = 4;
	dashesPtr->values[2] = 2;
	dashesPtr->values[3] = 0;
    } else if ((c == 'd') && (strcmp(string, "dashdotdot") == 0)) { 
	/* 2 4 2 2 */
	dashesPtr->values[0] = 2;
	dashesPtr->values[1] = 4;
	dashesPtr->values[2] = 2;
	dashesPtr->values[3] = 2;
	dashesPtr->values[4] = 0;
    } else {
	int objc;
	Tcl_Obj **objv;
	int i;

	if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc > 11) {	/* This is the postscript limit */
	    Tcl_AppendResult(interp, "too many values in dash list \"", 
			     string, "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < objc; i++) {
	    int value;

	    if (Tcl_GetIntFromObj(interp, objv[i], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    /*
	     * Backward compatibility:
	     * Allow list of 0 to turn off dashes
	     */
	    if ((value == 0) && (objc == 1)) {
		break;
	    }
	    if ((value < 1) || (value > 255)) {
		Tcl_AppendResult(interp, "dash value \"", 
			 Tcl_GetString(objv[i]), "\" is out of range", 
			 (char *)NULL);
		return TCL_ERROR;
	    }
	    dashesPtr->values[i] = (unsigned char)value;
	}
	/* Make sure the array ends with a NUL byte  */
	dashesPtr->values[i] = 0;
    }
    return TCL_OK;
}

const char *
Blt_NameOfSide(int side)
{
    switch (side) {
    case SIDE_LEFT:
	return "left";
    case SIDE_RIGHT:
	return "right";
    case SIDE_BOTTOM:
	return "bottom";
    case SIDE_TOP:
	return "top";
    }
    return "unknown side value";
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetSideFromObj --
 *
 *	Converts the fill style string into its numeric representation.
 *
 *	Valid style strings are "left", "right", "top", or  "bottom".
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED */
int
Blt_GetSideFromObj(
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tcl_Obj *objPtr,		/* Value string */
    int *sidePtr)		/* (out) Token representing side:
				 * either SIDE_LEFT, SIDE_RIGHT,
				 * SIDE_TOP, or SIDE_BOTTOM. */
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
	*sidePtr = SIDE_LEFT;
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
	*sidePtr = SIDE_RIGHT;
    } else if ((c == 't') && (strncmp(string, "top", length) == 0)) {
	*sidePtr = SIDE_TOP;
    } else if ((c == 'b') && (strncmp(string, "bottom", length) == 0)) {
	*sidePtr = SIDE_BOTTOM;
    } else {
	Tcl_AppendResult(interp, "bad side \"", string,
	    "\": should be left, right, top, or bottom", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResetLimits --
 *
 *	Resets the limits to their default values.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResetLimits(Blt_Limits *limitsPtr) /* Limits to be imposed on the value */
{
    limitsPtr->flags = 0;
    limitsPtr->min = LIMITS_MIN;
    limitsPtr->max = LIMITS_MAX;
    limitsPtr->nom = LIMITS_NOM;
}

int 
Blt_GetLimitsFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr, 
		     Blt_Limits *limitsPtr)
{
    int values[3];
    int nValues;
    int limitsFlags;

    /* Initialize limits to default values */
    values[2] = LIMITS_NOM;
    values[1] = LIMITS_MAX;
    values[0] = LIMITS_MIN;
    limitsFlags = 0;
    nValues = 0;
    if (objPtr != NULL) {
	Tcl_Obj **objv;
	int objc;
	int i;

	if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc > 3) {
	    Tcl_AppendResult(interp, "wrong # limits \"", Tcl_GetString(objPtr),
			     "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	for (i = 0; i < objc; i++) {
	    const char *string;
	    int size;

	    string = Tcl_GetString(objv[i]);
	    if (string[0] == '\0') {
		continue;		/* Empty string: use default value */
	    }
	    limitsFlags |= (1 << i);
	    if (Tk_GetPixelsFromObj(interp, tkwin, objv[i], &size) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((size < LIMITS_MIN) || (size > LIMITS_MAX)) {
		Tcl_AppendResult(interp, "bad limit \"", string, "\"",
				 (char *)NULL);
		return TCL_ERROR;
	    }
	    values[i] = size;
	}
	nValues = objc;
    }
    /*
     * Check the limits specified.  We can't check the requested size of
     * widgets.
     */
    switch (nValues) {
    case 1:
	limitsFlags |= (LIMITS_MIN_SET | LIMITS_MAX_SET);
	values[1] = values[0];		/* Set minimum and maximum to value */
	break;

    case 2:
	if (values[1] < values[0]) {
	    Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
		"\": min > max", (char *)NULL);
	    return TCL_ERROR;		/* Minimum is greater than maximum */
	}
	break;

    case 3:
	if (values[1] < values[0]) {
	    Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
			     "\": min > max", (char *)NULL);
	    return TCL_ERROR;		/* Minimum is greater than maximum */
	}
	if ((values[2] < values[0]) || (values[2] > values[1])) {
	    Tcl_AppendResult(interp, "nominal value \"", Tcl_GetString(objPtr),
		"\" out of range", (char *)NULL);
	    return TCL_ERROR;		/* Nominal is outside of range defined
					 * by minimum and maximum */
	}
	break;
    }
    limitsPtr->min = values[0];
    limitsPtr->max = values[1];
    limitsPtr->nom = values[2];
    limitsPtr->flags = limitsFlags;
    return TCL_OK;
}

/* Configuration option helper routines */

/*
 *---------------------------------------------------------------------------
 *
 * DoConfig --
 *
 *	This procedure applies a single configuration option
 *	to a widget record.
 *
 * Results:
 *	A standard TCL return value.
 *
 * Side effects:
 *	WidgRec is modified as indicated by specPtr and value.
 *	The old value is recycled, if that is appropriate for
 *	the value type.
 *
 *---------------------------------------------------------------------------
 */
static int
DoConfig(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window containing widget (needed to
				 * set up X resources). */
    Blt_ConfigSpec *sp,		/* Specifier to apply. */
    Tcl_Obj *objPtr,		/* Value to use to fill in widgRec. */
    char *widgRec)		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
{
    char *ptr;
    int objIsEmpty;

    objIsEmpty = FALSE;
    if (objPtr == NULL) {
	objIsEmpty = TRUE;
    } else if (sp->specFlags & BLT_CONFIG_NULL_OK) {
	int length;

	if (objPtr->bytes != NULL) {
	    length = objPtr->length;
	} else {
	    Tcl_GetStringFromObj(objPtr, &length);
	}
	objIsEmpty = (length == 0);
    }
    do {
	ptr = widgRec + sp->offset;
	switch (sp->type) {
	case BLT_CONFIG_ANCHOR: 
	    {
		Tk_Anchor anchor;
		
		if (Tk_GetAnchorFromObj(interp, objPtr, &anchor) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(Tk_Anchor *)ptr = anchor;
	    }
	    break;

	case BLT_CONFIG_BITMAP: 
	    {
		Pixmap bitmap;
		
		if (objIsEmpty) {
		    bitmap = None;
		} else {
		    bitmap = Tk_AllocBitmapFromObj(interp, tkwin, objPtr);
		    if (bitmap == None) {
			return TCL_ERROR;
		    }
		}
		if (*(Pixmap *)ptr != None) {
		    Tk_FreeBitmap(Tk_Display(tkwin), *(Pixmap *)ptr);
		}
		*(Pixmap *)ptr = bitmap;
	    }
	    break;

	case BLT_CONFIG_BOOLEAN: 
	    {
		int bool;
		
		if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = bool;
	    }
	    break;

	case BLT_CONFIG_BORDER: 
	    {
		Tk_3DBorder border;

		if (objIsEmpty) {
		    border = NULL;
		} else {
		    border = Tk_Alloc3DBorderFromObj(interp, tkwin, objPtr);
		    if (border == NULL) {
			return TCL_ERROR;
		    }
		}
		if (*(Tk_3DBorder *)ptr != NULL) {
		    Tk_Free3DBorder(*(Tk_3DBorder *)ptr);
		}
		*(Tk_3DBorder *)ptr = border;
	    }
	    break;

	case BLT_CONFIG_CAP_STYLE: 
	    {
		int cap;
		Tk_Uid uid;
		
		uid = Tk_GetUid(Tcl_GetString(objPtr));
		if (Tk_GetCapStyle(interp, uid, &cap) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = cap;
	    }
	    break;

	case BLT_CONFIG_COLOR: 
	    {
		XColor *color;
		
		if (objIsEmpty) {
		    color = NULL;
		} else {
		    color = Tk_GetColor(interp, tkwin, 
			Tk_GetUid(Tcl_GetString(objPtr)));
		    if (color == NULL) {
			return TCL_ERROR;
		    }
		}
		if (*(XColor **)ptr != NULL) {
		    Tk_FreeColor(*(XColor **)ptr);
		}
		*(XColor **)ptr = color;
	    }
	    break;

	case BLT_CONFIG_CURSOR:
	case BLT_CONFIG_ACTIVE_CURSOR: 
	    {
		Tk_Cursor cursor;
		
		if (objIsEmpty) {
		    cursor = None;
		} else {
		    cursor = Tk_AllocCursorFromObj(interp, tkwin, objPtr);
		    if (cursor == None) {
			return TCL_ERROR;
		    }
		}
		if (*(Tk_Cursor *)ptr != None) {
		    Tk_FreeCursor(Tk_Display(tkwin), *(Tk_Cursor *)ptr);
		}
		*(Tk_Cursor *)ptr = cursor;
		if (sp->type == BLT_CONFIG_ACTIVE_CURSOR) {
		    Tk_DefineCursor(tkwin, cursor);
		}
	    }
	    break;

	case BLT_CONFIG_CUSTOM: 
	    if ((*sp->customPtr->parseProc)(sp->customPtr->clientData, interp, 
		tkwin, objPtr, widgRec, sp->offset, sp->specFlags) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_CONFIG_DOUBLE: 
	    {
		double value;
		
		if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(double *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_FONT: 
	    {
		Blt_Font font;
		
		if (objIsEmpty) {
		    font = NULL;
		} else {
		    font = Blt_AllocFontFromObj(interp, tkwin, objPtr);
		    if (font == NULL) {
			return TCL_ERROR;
		    }
		}
		if (*(Blt_Font *)ptr != NULL) {
		    Blt_FreeFont(*(Blt_Font *)ptr);
		}
		*(Blt_Font *)ptr = font;
	    }
	    break;

	case BLT_CONFIG_TK_FONT: 
	    {
		Tk_Font font;
		
		if (objIsEmpty) {
		    font = NULL;
		} else {
		    font = Tk_AllocFontFromObj(interp, tkwin, objPtr);
		    if (font == NULL) {
			return TCL_ERROR;
		    }
		}
		if (*(Tk_Font *)ptr != NULL) {
		    Tk_FreeFont(*(Tk_Font *)ptr);
		}
		*(Tk_Font *)ptr = font;
	    }
	    break;

	case BLT_CONFIG_INT: 
	    {
		int value;
		
		if (Tcl_GetIntFromObj(interp, objPtr, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_JOIN_STYLE: 
	    {
		int join;
		Tk_Uid uid;

		uid = Tk_GetUid(Tcl_GetString(objPtr));
		if (Tk_GetJoinStyle(interp, uid, &join) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = join;
	    }
	    break;

	case BLT_CONFIG_JUSTIFY: 
	    {
		Tk_Justify justify;
		
		if (Tk_GetJustifyFromObj(interp, objPtr, &justify) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(Tk_Justify *)ptr = justify;
	    }
	    break;

	case BLT_CONFIG_MM: 
	    {
		double value;

		if (Tk_GetMMFromObj(interp, tkwin, objPtr, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(double *)ptr = value;
	    }
	    break;


	case BLT_CONFIG_RELIEF: 
	    {
		int relief;
		
		if (Tk_GetReliefFromObj(interp, objPtr, &relief) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = relief;
	    }
	    break;

	case BLT_CONFIG_STRING: 
	    {
		char *value;
		
		value = (objIsEmpty) ? NULL : 
		    Blt_Strdup(Tcl_GetString(objPtr));
		if (*(char **)ptr != NULL) {
		    free(*(char **)ptr);
		    *((char **) ptr) = NULL;
		}
		*(char **)ptr = value;
	    }
	    break;

	case BLT_CONFIG_UID: 
	    if (*(Blt_Uid *)ptr != NULL) {
		Blt_FreeUid(*(Blt_Uid *)ptr);
	    }
	    if (objIsEmpty) {
		*(Blt_Uid *)ptr = NULL;
	    } else {
		*(Blt_Uid *)ptr = Blt_GetUid(Tcl_GetString(objPtr));
	    }
	    break;

	case BLT_CONFIG_WINDOW: 
	    {
		Tk_Window tkwin2;

		if (objIsEmpty) {
		    tkwin2 = None;
		} else {
		    const char *path;

		    path = Tcl_GetString(objPtr);
		    tkwin2 = Tk_NameToWindow(interp, path, tkwin);
		    if (tkwin2 == NULL) {
			return TCL_ERROR;
		    }
		}
		*(Tk_Window *)ptr = tkwin2;
	    }
	    break;

	case BLT_CONFIG_BITMASK: 
	    {
		int bool;
		unsigned long mask, flags;

		if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
		    return TCL_ERROR;
		}
		mask = (unsigned long)sp->customPtr;
		flags = *(int *)ptr;
		flags &= ~mask;
		if (bool) {
		    flags |= mask;
		}
		*(int *)ptr = flags;
	    }
	    break;

	case BLT_CONFIG_BITMASK_INVERT: 
	    {
		int bool;
		unsigned long mask, flags;

		if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
		    return TCL_ERROR;
		}
		mask = (unsigned long)sp->customPtr;
		flags = *(int *)ptr;
		flags &= ~mask;
		if (!bool) {
		    flags |= mask;
		}
		*(int *)ptr = flags;
	    }
	    break;

	case BLT_CONFIG_DASHES:
	    if (Blt_GetDashesFromObj(interp, objPtr, (Blt_Dashes *)ptr) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;


	case BLT_CONFIG_FILL:
	    if (Blt_GetFillFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_CONFIG_RESIZE:
	    if (Blt_GetResizeFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_CONFIG_FLOAT: 
	    {
		double value;
		
		if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(float *)ptr = (float)value;
	    }
	    break;

	case BLT_CONFIG_INT_NNEG: 
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_NNEG, 
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = (int)value;
	    }
	    break;


	case BLT_CONFIG_INT_POS: 
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_POS, &value) 
		    != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = (int)value;
	    }
	    break;


	case BLT_CONFIG_LIST: 
	    {
		const char **argv;
		int argc;
		
		if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &argc, &argv) 
		    != TCL_OK) {
		    return TCL_ERROR;
		}
		if (*(char ***)ptr != NULL) {
		  Tcl_Free((void*)(*(char ***)ptr));
		    *((char ***) ptr) = NULL;
		}
		*(const char ***)ptr = argv;
	    }
	    break;

	case BLT_CONFIG_LONG: 
	    {
		long value;
		
	        if (Tcl_GetLongFromObj(interp, objPtr, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(long *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_LONG_NNEG: 
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_NNEG, 
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(long *)ptr = value;
	    }
	    break;


	case BLT_CONFIG_LONG_POS: 
	    {
		long value;
		
		if (Blt_GetCountFromObj(interp, objPtr, COUNT_POS, &value) 
		    != TCL_OK) {
		    return TCL_ERROR;
		}
		*(long *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_OBJ: 
	    {
		Tcl_IncrRefCount(objPtr);
		if (*(Tcl_Obj **)ptr != NULL) {
		    Tcl_DecrRefCount(*(Tcl_Obj **)ptr);
		}
		*(Tcl_Obj **)ptr = objPtr;
	    }
	    break;

	case BLT_CONFIG_PAD:
	    if (Blt_GetPadFromObj(interp, tkwin, objPtr, (Blt_Pad *)ptr) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_CONFIG_PIXELS_NNEG: 
	    {
		int value;
		
		if (Blt_GetPixelsFromObj(interp, tkwin, objPtr, 
			PIXELS_NNEG, &value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_PIXELS: 
	    {
		int value;
		
		if (Blt_GetPixelsFromObj(interp, tkwin, objPtr, PIXELS_ANY, 
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_PIXELS_POS: 
	    {
		int value;
		
		if (Blt_GetPixelsFromObj(interp, tkwin, objPtr, PIXELS_POS,
			&value) != TCL_OK) {
		    return TCL_ERROR;
		}
		*(int *)ptr = value;
	    }
	    break;

	case BLT_CONFIG_STATE: 
	    if (Blt_GetStateFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_CONFIG_SIDE:
	    if (Blt_GetSideFromObj(interp, objPtr, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_CONFIG_BACKGROUND: 
	    {
		Blt_Background style;
		
		if (objIsEmpty) {
		    style = NULL;
		} else {
		    style = Blt_GetBackgroundFromObj(interp, tkwin, objPtr);
		    if (style == NULL) {
			return TCL_ERROR;
		    }
		}
		if (*(Blt_Background *)ptr != NULL) {
		    Blt_FreeBackground(*(Blt_Background *)ptr);
		}
		*(Blt_Background *)ptr = style;
	    }
	    break;

	case BLT_CONFIG_PIX32: 
	    if (Blt_GetPixelFromObj(interp, objPtr, (Blt_Pixel *)ptr)!=TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	default: 
	    Tcl_AppendResult(interp, "bad config table: unknown type ", 
			     Blt_Itoa(sp->type), (char *)NULL);
	    return TCL_ERROR;
	}
	sp++;
    } while ((sp->switchName == NULL) && (sp->type != BLT_CONFIG_END));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatConfigValue --
 *
 *	This procedure formats the current value of a configuration
 *	option.
 *
 * Results:
 *	The return value is the formatted value of the option given
 *	by specPtr and widgRec.  If the value is static, so that it
 *	need not be freed, *freeProcPtr will be set to NULL;  otherwise
 *	*freeProcPtr will be set to the address of a procedure to
 *	free the result, and the caller must invoke this procedure
 *	when it is finished with the result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
FormatConfigValue(
    Tcl_Interp *interp,		/* Interpreter for use in real conversions. */
    Tk_Window tkwin,		/* Window corresponding to widget. */
    Blt_ConfigSpec *sp,		/* Pointer to information describing option.
				 * Must not point to a synonym option. */
    char *widgRec)		/* Pointer to record holding current
				 * values of info for widget. */
{
    char *ptr;
    const char *string;

    ptr = widgRec + sp->offset;
    string = "";
    switch (sp->type) {
    case BLT_CONFIG_ANCHOR:
	string = Tk_NameOfAnchor(*(Tk_Anchor *)ptr);
	break;

    case BLT_CONFIG_BITMAP: 
	if (*(Pixmap *)ptr != None) {
	    string = Tk_NameOfBitmap(Tk_Display(tkwin), *(Pixmap *)ptr);
	}
	break;

    case BLT_CONFIG_BOOLEAN: 
	return Tcl_NewBooleanObj(*(int *)ptr);

    case BLT_CONFIG_BORDER: 
	if (*(Tk_3DBorder *)ptr != NULL) {
	    string = Tk_NameOf3DBorder(*(Tk_3DBorder *)ptr);
	}
	break;

    case BLT_CONFIG_CAP_STYLE:
	string = Tk_NameOfCapStyle(*(int *)ptr);
	break;

    case BLT_CONFIG_COLOR: 
	if (*(XColor **)ptr != NULL) {
	    string = Tk_NameOfColor(*(XColor **)ptr);
	}
	break;

    case BLT_CONFIG_CURSOR:
    case BLT_CONFIG_ACTIVE_CURSOR:
	if (*(Tk_Cursor *)ptr != None) {
	    string = Tk_NameOfCursor(Tk_Display(tkwin), *(Tk_Cursor *)ptr);
	}
	break;

    case BLT_CONFIG_CUSTOM:
	return (*sp->customPtr->printProc)
		(sp->customPtr->clientData, interp, tkwin, widgRec, 
		sp->offset, sp->specFlags);

    case BLT_CONFIG_DOUBLE: 
	return Tcl_NewDoubleObj(*(double *)ptr);

    case BLT_CONFIG_FONT: 
	if (*(Blt_Font *)ptr != NULL) {
	    string = Blt_NameOfFont(*(Blt_Font *)ptr);
	}
	break;

    case BLT_CONFIG_TK_FONT: 
	if (*(Tk_Font *)ptr != NULL) {
	    string = Tk_NameOfFont(*(Tk_Font *)ptr);
	}
	break;

    case BLT_CONFIG_INT: 
	return Tcl_NewIntObj(*(int *)ptr);

    case BLT_CONFIG_JOIN_STYLE:
	string = Tk_NameOfJoinStyle(*(int *)ptr);
	break;

    case BLT_CONFIG_JUSTIFY:
	string = Tk_NameOfJustify(*(Tk_Justify *)ptr);
	break;

    case BLT_CONFIG_MM:
	return Tcl_NewDoubleObj(*(double *)ptr);

    case BLT_CONFIG_PIXELS: 
    case BLT_CONFIG_PIXELS_POS: 
    case BLT_CONFIG_PIXELS_NNEG: 
	return Tcl_NewIntObj(*(int *)ptr);

    case BLT_CONFIG_RELIEF: 
	string = Tk_NameOfRelief(*(int *)ptr);
	break;

    case BLT_CONFIG_STRING: 
    case BLT_CONFIG_UID:
	if (*(char **)ptr != NULL) {
	    string = *(char **)ptr;
	}
	break;

    case BLT_CONFIG_BITMASK:
	{
	    unsigned long flag;

	    flag = (*(unsigned long *)ptr) & (unsigned long)sp->customPtr;
	    return Tcl_NewBooleanObj((flag != 0));
	}

    case BLT_CONFIG_BITMASK_INVERT:
	{
	    unsigned long flag;

	    flag = (*(unsigned long *)ptr) & (unsigned long)sp->customPtr;
	    return Tcl_NewBooleanObj((flag == 0));
	}

    case BLT_CONFIG_DASHES: 
	{
	    unsigned char *p;
	    Tcl_Obj *listObjPtr;
	    Blt_Dashes *dashesPtr = (Blt_Dashes *)ptr;
	    
	    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    for(p = dashesPtr->values; *p != 0; p++) {
		Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(*p));
	    }
	    return listObjPtr;
	}

    case BLT_CONFIG_INT_NNEG:
    case BLT_CONFIG_INT_POS:
	return Tcl_NewIntObj(*(int *)ptr);

    case BLT_CONFIG_FILL: 
	string = Blt_NameOfFill(*(int *)ptr);
	break;

    case BLT_CONFIG_RESIZE: 
	string = Blt_NameOfResize(*(int *)ptr);
	break;

    case BLT_CONFIG_FLOAT: 
	{
	    double x = *(float *)ptr;
	    return Tcl_NewDoubleObj(x);
	}

    case BLT_CONFIG_LIST: 
	{
	    Tcl_Obj *objPtr, *listObjPtr;
	    char *const *p;
	    
	    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    for (p = *(char ***)ptr; *p != NULL; p++) {
		objPtr = Tcl_NewStringObj(*p, -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	    return listObjPtr;
	}

    case BLT_CONFIG_LONG: 
	return Tcl_NewLongObj(*(long *)ptr);

    case BLT_CONFIG_LONG_NNEG:
    case BLT_CONFIG_LONG_POS:
	return Tcl_NewLongObj(*(long *)ptr);

    case BLT_CONFIG_OBJ:
	if (*(Tcl_Obj **)ptr != NULL) {
	    return *(Tcl_Obj **)ptr;
	}
	break;

    case BLT_CONFIG_PAD: 
	{
	    Blt_Pad *padPtr = (Blt_Pad *)ptr;
	    Tcl_Obj *objPtr, *listObjPtr;
	    
	    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    objPtr = Tcl_NewIntObj(padPtr->side1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewIntObj(padPtr->side2);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    return listObjPtr;
	}

    case BLT_CONFIG_STATE: 
	string = Blt_NameOfState(*(int *)ptr);
	break;

    case BLT_CONFIG_SIDE: 
	string = Blt_NameOfSide(*(int *)ptr);
	break;

    case BLT_CONFIG_BACKGROUND: 
	if (*(Blt_Background *)ptr != NULL) {
	    string = Blt_NameOfBackground(*(Blt_Background *)ptr);
	}
	break;

    case BLT_CONFIG_PIX32: 
	string = Blt_NameOfPixel((Blt_Pixel *)ptr);
	break;

    default: 
	string = "?? unknown type ??";
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatConfigInfo --
 *
 *	Create a valid TCL list holding the configuration information
 *	for a single configuration option.
 *
 * Results:
 *	A TCL list, dynamically allocated.  The caller is expected to
 *	arrange for this list to be freed eventually.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
FormatConfigInfo(
    Tcl_Interp *interp,		/* Interpreter to use for things
				 * like floating-point precision. */
    Tk_Window tkwin,		/* Window corresponding to widget. */
    Blt_ConfigSpec *sp,		/* Pointer to information describing
				 * option. */
    char *widgRec)		/* Pointer to record holding current
				 * values of info for widget. */
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (sp->switchName != NULL) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(sp->switchName, -1));
    }  else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (sp->dbName != NULL) {
	Tcl_ListObjAppendElement(interp, listObjPtr,  
		Tcl_NewStringObj(sp->dbName, -1));
    } else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (sp->type == BLT_CONFIG_SYNONYM) {
	return listObjPtr;
    } 
    if (sp->dbClass != NULL) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(sp->dbClass, -1));
    } else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (sp->defValue != NULL) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(sp->defValue, -1));
    } else {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    Tcl_ListObjAppendElement(interp, listObjPtr, 
	FormatConfigValue(interp, tkwin, sp, widgRec));
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindConfigSpec --
 *
 *	Search through a table of configuration specs, looking for
 *	one that matches a given switchName.
 *
 * Results:
 *	The return value is a pointer to the matching entry, or NULL
 *	if nothing matched.  In that case an error message is left
 *	in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_ConfigSpec *
FindConfigSpec(
    Tcl_Interp *interp,		/* Used for reporting errors. */
    Blt_ConfigSpec *specs,	/* Pointer to table of configuration
				 * specifications for a widget. */
    Tcl_Obj *objPtr,		/* Name (suitable for use in a "config"
				 * command) identifying particular option. */
    int needFlags,		/* Flags that must be present in matching
				 * entry. */
    int hateFlags)		/* Flags that must NOT be present in
				 * matching entry. */
{
    Blt_ConfigSpec *matchPtr;	/* Matching spec, or NULL. */
    Blt_ConfigSpec *sp;
    const char *string;
    char c;			/* First character of current argument. */
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[1];
    matchPtr = NULL;
    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
	if (sp->switchName == NULL) {
	    continue;
	}
	if ((sp->switchName[1] != c) || 
	    (strncmp(sp->switchName, string, length) != 0)) {
	    continue;
	}
	if (((sp->specFlags & needFlags) != needFlags) || 
	    (sp->specFlags & hateFlags)) {
	    continue;
	}
	if (sp->switchName[length] == 0) {
	    matchPtr = sp;
	    goto gotMatch;
	}
	if (matchPtr != NULL) {
	    if (interp != NULL) {
	        Tcl_AppendResult(interp, "ambiguous option \"", string, "\"", 
			(char *)NULL);
            }
	    return (Blt_ConfigSpec *)NULL;
	}
	matchPtr = sp;
    }

    if (matchPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown option \"", string, "\"", 
		(char *)NULL);
	}
	return (Blt_ConfigSpec *)NULL;
    }

    /*
     * Found a matching entry.  If it's a synonym, then find the
     * entry that it's a synonym for.
     */

 gotMatch:
    sp = matchPtr;
    if (sp->type == BLT_CONFIG_SYNONYM) {
	for (sp = specs; /*empty*/; sp++) {
	    if (sp->type == BLT_CONFIG_END) {
		if (interp != NULL) {
   		    Tcl_AppendResult(interp, 
			"couldn't find synonym for option \"", string, "\"", 
			(char *)NULL);
		}
		return (Blt_ConfigSpec *) NULL;
	    }
	    if ((sp->dbName == matchPtr->dbName) && 
		(sp->type != BLT_CONFIG_SYNONYM) && 
		((sp->specFlags & needFlags) == needFlags) && 
		!(sp->specFlags & hateFlags)) {
		break;
	    }
	}
    }
    return sp;
}

/* Public routines */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureWidgetFromObj --
 *
 *	Process command-line options and database options to
 *	fill in fields of a widget record with resources and
 *	other parameters.
 *
 * Results:
 *	A standard TCL return value.  In case of an error,
 *	the interp's result will hold an error message.
 *
 * Side effects:
 *	The fields of widgRec get filled in with information
 *	from argc/argv and the option database.  Old information
 *	in widgRec's fields gets recycled.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ConfigureWidgetFromObj(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window containing widget (needed to
				 * set up X resources). */
    Blt_ConfigSpec *specs,	/* Describes legal options. */
    int objc,			/* Number of elements in argv. */
    Tcl_Obj *const *objv,	/* Command-line options. */
    char *widgRec,		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
    int flags)			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered.  Also,
				 * may have BLT_CONFIG_OBJV_ONLY set. */
{
    Blt_ConfigSpec *sp;
    int needFlags;		/* Specs must contain this set of flags
				 * or else they are not considered. */
    int hateFlags;		/* If a spec contains any bits here, it's
				 * not considered. */
    int result;

    if (tkwin == NULL) {
	/*
	 * Either we're not really in Tk, or the main window was destroyed and
	 * we're on our way out of the application
	 */
	Tcl_AppendResult(interp, "NULL main window", (char *)NULL);
	return TCL_ERROR;
    }

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
	hateFlags = BLT_CONFIG_COLOR_ONLY;
    } else {
	hateFlags = BLT_CONFIG_MONO_ONLY;
    }

    /*
     * Pass one:  scan through all the option specs, replacing strings
     * with Tk_Uid structs (if this hasn't been done already) and
     * clearing the BLT_CONFIG_OPTION_SPECIFIED flags.
     */

    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
	if (!(sp->specFlags & INIT) && (sp->switchName != NULL)) {
	    if (sp->dbName != NULL) {
		sp->dbName = Tk_GetUid(sp->dbName);
	    }
	    if (sp->dbClass != NULL) {
		sp->dbClass = Tk_GetUid(sp->dbClass);
	    }
	    if (sp->defValue != NULL) {
		sp->defValue = Tk_GetUid(sp->defValue);
	    }
	}
	sp->specFlags = (sp->specFlags & ~BLT_CONFIG_OPTION_SPECIFIED) | INIT;
    }

    /*
     * Pass two:  scan through all of the arguments, processing those
     * that match entries in the specs.
     */
    while (objc > 0) {
	sp = FindConfigSpec(interp, specs, objv[0], needFlags, hateFlags);
	if (sp == NULL) {
	    return TCL_ERROR;
	}

	/* Process the entry.  */
	if (objc < 2) {
	    Tcl_AppendResult(interp, "value for \"", Tcl_GetString(objv[0]),
		    "\" missing", (char *)NULL);
	    return TCL_ERROR;
	}
	if (DoConfig(interp, tkwin, sp, objv[1], widgRec) != TCL_OK) {
	    char msg[100];

	    sprintf_s(msg, 100, "\n    (processing \"%.40s\" option)",
		    sp->switchName);
	    Tcl_AddErrorInfo(interp, msg);
	    return TCL_ERROR;
	}
	sp->specFlags |= BLT_CONFIG_OPTION_SPECIFIED;
	objc -= 2, objv += 2;
    }

    /*
     * Pass three:  scan through all of the specs again;  if no
     * command-line argument matched a spec, then check for info
     * in the option database.  If there was nothing in the
     * database, then use the default.
     */

    if ((flags & BLT_CONFIG_OBJV_ONLY) == 0) {
	Tcl_Obj *objPtr;

	for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
	    if ((sp->specFlags & BLT_CONFIG_OPTION_SPECIFIED) || 
		(sp->switchName == NULL) || (sp->type == BLT_CONFIG_SYNONYM)) {
		continue;
	    }
	    if (((sp->specFlags & needFlags) != needFlags) || 
		(sp->specFlags & hateFlags)) {
		continue;
	    }
	    objPtr = NULL;
	    if (sp->dbName != NULL) {
		Tk_Uid value;

		/* If a resource name was specified, check if there's
		 * also a value was associated with it.  This
		 * overrides the default value. */
		value = Tk_GetOption(tkwin, sp->dbName, sp->dbClass);
		if (value != NULL) {
		    objPtr = Tcl_NewStringObj(value, -1);
		}
	    }

	    if (objPtr != NULL) {
		Tcl_IncrRefCount(objPtr);
		result = DoConfig(interp, tkwin, sp, objPtr, widgRec);
		Tcl_DecrRefCount(objPtr);
		if (result != TCL_OK) {
		    char msg[200];
    
		    sprintf_s(msg, 200, 
			"\n    (%s \"%.50s\" in widget \"%.50s\")",
			"database entry for", sp->dbName, Tk_PathName(tkwin));
		    Tcl_AddErrorInfo(interp, msg);
		    return TCL_ERROR;
		}
	    } else if ((sp->defValue != NULL) && 
		((sp->specFlags & BLT_CONFIG_DONT_SET_DEFAULT) == 0)) {

		/* No resource value is found, use the default value. */
		objPtr = Tcl_NewStringObj(sp->defValue, -1);
		Tcl_IncrRefCount(objPtr);
		result = DoConfig(interp, tkwin, sp, objPtr, widgRec);
		Tcl_DecrRefCount(objPtr);
		if (result != TCL_OK) {
		    char msg[200];
		    
		    sprintf_s(msg, 200, 
			"\n    (%s \"%.50s\" in widget \"%.50s\")",
			"default value for", sp->dbName, Tk_PathName(tkwin));
		    Tcl_AddErrorInfo(interp, msg);
		    return TCL_ERROR;
		}
	    }
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureInfoFromObj --
 *
 *	Return information about the configuration options
 *	for a window, and their current values.
 *
 * Results:
 *	Always returns TCL_OK.  The interp's result will be modified
 *	hold a description of either a single configuration option
 *	available for "widgRec" via "specs", or all the configuration
 *	options available.  In the "all" case, the result will
 *	available for "widgRec" via "specs".  The result will
 *	be a list, each of whose entries describes one option.
 *	Each entry will itself be a list containing the option's
 *	name for use on command lines, database name, database
 *	class, default value, and current value (empty string
 *	if none).  For options that are synonyms, the list will
 *	contain only two values:  name and synonym name.  If the
 *	"name" argument is non-NULL, then the only information
 *	returned is that for the named argument (i.e. the corresponding
 *	entry in the overall list is returned).
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_ConfigureInfoFromObj(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window corresponding to widgRec. */
    Blt_ConfigSpec *specs,	/* Describes legal options. */
    char *widgRec,		/* Record whose fields contain current
				 * values for options. */
    Tcl_Obj *objPtr,		/* If non-NULL, indicates a single option
				 * whose info is to be returned.  Otherwise
				 * info is returned for all options. */
    int flags)			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    Blt_ConfigSpec *sp;
    Tcl_Obj *listObjPtr, *valueObjPtr;
    const char *string;
    int needFlags, hateFlags;

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
	hateFlags = BLT_CONFIG_COLOR_ONLY;
    } else {
	hateFlags = BLT_CONFIG_MONO_ONLY;
    }

    /*
     * If information is only wanted for a single configuration
     * spec, then handle that one spec specially.
     */

    Tcl_SetResult(interp, (char *)NULL, TCL_STATIC);
    if (objPtr != NULL) {
	sp = FindConfigSpec(interp, specs, objPtr, needFlags, hateFlags);
	if (sp == NULL) {
	    return TCL_ERROR;
	}
	valueObjPtr =  FormatConfigInfo(interp, tkwin, sp, widgRec);
	Tcl_SetObjResult(interp, valueObjPtr);
	return TCL_OK;
    }

    /*
     * Loop through all the specs, creating a big list with all
     * their information.
     */
    string = NULL;		/* Suppress compiler warning. */
    if (objPtr != NULL) {
	string = Tcl_GetString(objPtr);
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
	if ((objPtr != NULL) && (sp->switchName != string)) {
	    continue;
	}
	if (((sp->specFlags & needFlags) != needFlags) || 
	    (sp->specFlags & hateFlags)) {
	    continue;
	}
	if (sp->switchName == NULL) {
	    continue;
	}
	valueObjPtr = FormatConfigInfo(interp, tkwin, sp, widgRec);
	Tcl_ListObjAppendElement(interp, listObjPtr, valueObjPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureValueFromObj --
 *
 *	This procedure returns the current value of a configuration
 *	option for a widget.
 *
 * Results:
 *	The return value is a standard TCL completion code (TCL_OK or
 *	TCL_ERROR).  The interp's result will be set to hold either the value
 *	of the option given by objPtr (if TCL_OK is returned) or
 *	an error message (if TCL_ERROR is returned).
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ConfigureValueFromObj(
    Tcl_Interp *interp,		/* Interpreter for error reporting. */
    Tk_Window tkwin,		/* Window corresponding to widgRec. */
    Blt_ConfigSpec *specs,	/* Describes legal options. */
    char *widgRec,		/* Record whose fields contain current
				 * values for options. */
    Tcl_Obj *objPtr,		/* Gives the command-line name for the
				 * option whose value is to be returned. */
    int flags)			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    Blt_ConfigSpec *sp;
    int needFlags, hateFlags;

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    if (Tk_Depth(tkwin) <= 1) {
	hateFlags = BLT_CONFIG_COLOR_ONLY;
    } else {
	hateFlags = BLT_CONFIG_MONO_ONLY;
    }
    sp = FindConfigSpec(interp, specs, objPtr, needFlags, hateFlags);
    if (sp == NULL) {
	return TCL_ERROR;
    }
    objPtr = FormatConfigValue(interp, tkwin, sp, widgRec);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeOptions --
 *
 *	Free up all resources associated with configuration options.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any resource in widgRec that is controlled by a configuration
 *	option (e.g. a Tk_3DBorder or XColor) is freed in the appropriate
 *	fashion.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeOptions(
    Blt_ConfigSpec *specs,	/* Describes legal options. */
    char *widgRec,		/* Record whose fields contain current
				 * values for options. */
    Display *display,		/* X display; needed for freeing some
				 * resources. */
    int needFlags)		/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    Blt_ConfigSpec *sp;

    for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
	char *ptr;

	if ((sp->specFlags & needFlags) != needFlags) {
	    continue;
	}
	ptr = widgRec + sp->offset;
	switch (sp->type) {
	case BLT_CONFIG_STRING:
	    if (*((char **) ptr) != NULL) {
		free(*((char **) ptr));
		*((char **) ptr) = NULL;
	    }
	    break;

	case BLT_CONFIG_COLOR:
	    if (*((XColor **) ptr) != NULL) {
		Tk_FreeColor(*((XColor **) ptr));
		*((XColor **) ptr) = NULL;
	    }
	    break;

	case BLT_CONFIG_FONT:
	    if (*((Blt_Font *) ptr) != None) {
	        Blt_FreeFont(*((Blt_Font *) ptr));
  	        *((Blt_Font *) ptr) = NULL;
            }
	    break;

	case BLT_CONFIG_TK_FONT:
	    if (*((Tk_Font *) ptr) != None) {
	        Tk_FreeFont(*((Tk_Font *) ptr));
  	        *((Tk_Font *) ptr) = NULL;
            }
	    break;

	case BLT_CONFIG_BITMAP:
	    if (*((Pixmap *) ptr) != None) {
		Tk_FreeBitmap(display, *((Pixmap *) ptr));
		*((Pixmap *) ptr) = None;
	    }
	    break;

	case BLT_CONFIG_BORDER:
	    if (*((Tk_3DBorder *) ptr) != NULL) {
		Tk_Free3DBorder(*((Tk_3DBorder *) ptr));
		*((Tk_3DBorder *) ptr) = NULL;
	    }
	    break;

	case BLT_CONFIG_CURSOR:
	case BLT_CONFIG_ACTIVE_CURSOR:
	    if (*((Tk_Cursor *) ptr) != None) {
		Tk_FreeCursor(display, *((Tk_Cursor *) ptr));
		*((Tk_Cursor *) ptr) = None;
	    }
	    break;

	case BLT_CONFIG_OBJ:
	    if (*(Tcl_Obj **)ptr != NULL) {
		Tcl_DecrRefCount(*(Tcl_Obj **)ptr);
		*(Tcl_Obj **)ptr = NULL;
	    }
	    break;

	case BLT_CONFIG_LIST:
	    if (*((char ***) ptr) != NULL) {
	      Tcl_Free((void*)(*((char ***) ptr)));
		*((char ***) ptr) = NULL;
	    }
	    break;

	case BLT_CONFIG_UID:
	    if (*(Blt_Uid *)ptr != NULL) {
		Blt_FreeUid(*(Blt_Uid *)ptr);
		*(Blt_Uid *)ptr = NULL;
	    }
	    break;

	case BLT_CONFIG_BACKGROUND:
	    if (*((Blt_Background *)ptr) != NULL) {
		Blt_FreeBackground(*((Blt_Background *)ptr));
		*((Blt_Background *)ptr) = NULL;
	    }
	    break;

	case BLT_CONFIG_CUSTOM:
	    if ((sp->customPtr->freeProc != NULL) && (*(char **)ptr != NULL)) {
		(*sp->customPtr->freeProc)(sp->customPtr->clientData,
			display, widgRec, sp->offset);
	    }
	    break;

	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigModified --
 *
 *      Given the configuration specifications and one or more option
 *	patterns (terminated by a NULL), indicate if any of the matching
 *	configuration options has been reset.
 *
 * Results:
 *      Returns 1 if one of the options has changed, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int 
Blt_ConfigModified TCL_VARARGS_DEF(Blt_ConfigSpec *, arg1)
{
    va_list argList;
    Blt_ConfigSpec *specs;
    Blt_ConfigSpec *sp;
    const char *option;

    specs = TCL_VARARGS_START(Blt_ConfigSpec *, arg1, argList);
    while ((option = va_arg(argList, const char *)) != NULL) {
	for (sp = specs; sp->type != BLT_CONFIG_END; sp++) {
	    if ((Tcl_StringMatch(sp->switchName, option)) &&
		(sp->specFlags & BLT_CONFIG_OPTION_SPECIFIED)) {
		va_end(argList);
		return 1;
	    }
	}
    }
    va_end(argList);
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureComponentFromObj --
 *
 *	Configures a component of a widget.  This is useful for
 *	widgets that have multiple components which aren't uniquely
 *	identified by a Tk_Window. It allows us, for example, set
 *	resources for axes of the graph widget. The graph really has
 *	only one window, but its convenient to specify components in a
 *	hierarchy of options.
 *
 *		*graph.x.logScale yes
 *		*graph.Axis.logScale yes
 *		*graph.temperature.scaleSymbols yes
 *		*graph.Element.scaleSymbols yes
 *
 *	This is really a hack to work around the limitations of the Tk
 *	resource database.  It creates a temporary window, needed to
 *	call Tk_ConfigureWidget, using the name of the component.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *	A temporary window is created merely to pass to Tk_ConfigureWidget.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ConfigureComponentFromObj(
    Tcl_Interp *interp,
    Tk_Window parent,		/* Window to associate with component */
    const char *name,		/* Name of component */
    const char *className,
    Blt_ConfigSpec *sp,
    int objc,
    Tcl_Obj *const *objv,
    char *widgRec,
    int flags)
{
    Tk_Window tkwin;
    int result;
    char *tmpName;
    int isTemporary = FALSE;

    tmpName = Blt_Strdup(name);

    /* Window name can't start with an upper case letter */
    tmpName[0] = tolower(name[0]);

    /*
     * Create component if a child window by the component's name
     * doesn't already exist.
     */
    tkwin = Blt_FindChild(parent, tmpName);
    if (tkwin == NULL) {
	tkwin = Tk_CreateWindow(interp, parent, tmpName, (char *)NULL);
	isTemporary = TRUE;
    }
    if (tkwin == NULL) {
	Tcl_AppendResult(interp, "can't find window in \"", 
			 Tk_PathName(parent), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    assert(Tk_Depth(tkwin) == Tk_Depth(parent));
    free(tmpName);

    Tk_SetClass(tkwin, className);
    result = Blt_ConfigureWidgetFromObj(interp, tkwin, sp, objc, objv, widgRec,
	flags);
    if (isTemporary) {
	Tk_DestroyWindow(tkwin);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ObjIsOption --
 *
 *	Indicates whether objPtr is a valid configuration option 
 *	such as -background.
 *
 * Results:
 *	Returns 1 is a matching option is found and 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ObjIsOption(
    Blt_ConfigSpec *specs,	/* Describes legal options. */
    Tcl_Obj *objPtr,		/* Command-line option name. */
    int flags)			/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered.  Also,
				 * may have BLT_CONFIG_OBJV_ONLY set. */
{
    Blt_ConfigSpec *sp;
    int needFlags;		/* Specs must contain this set of flags
				 * or else they are not considered. */

    needFlags = flags & ~(BLT_CONFIG_USER_BIT - 1);
    sp = FindConfigSpec((Tcl_Interp *)NULL, specs, objPtr, needFlags, 0);
    return (sp != NULL);
}
