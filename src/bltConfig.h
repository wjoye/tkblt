
/* 
 * bltConfig.h --
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

#ifndef BLT_CONFIG_H
#define BLT_CONFIG_H

#  include <stddef.h>

#ifndef Blt_Offset
#ifdef offsetof
#define Blt_Offset(type, field) ((int) offsetof(type, field))
#else
#define Blt_Offset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif
#endif /* Blt_Offset */


typedef int (Blt_OptionParseProc)(ClientData clientData, Tcl_Interp *interp, 
	Tk_Window tkwin, Tcl_Obj *objPtr, char *widgRec, int offset, int flags);
typedef Tcl_Obj *(Blt_OptionPrintProc)(ClientData clientData, 
	Tcl_Interp *interp, Tk_Window tkwin, char *widgRec, int offset, 
	int flags);
typedef void (Blt_OptionFreeProc)(ClientData clientData, Display *display, 
	char *widgRec, int offset);

typedef struct Blt_CustomOption {
    Blt_OptionParseProc *parseProc;	/* Procedure to call to parse
					 * an option and store it in
					 * converted form. */

    Blt_OptionPrintProc *printProc;	/* Procedure to return a
					 * Tcl_Obj representing an
					 * existing option value. */

    Blt_OptionFreeProc *freeProc;	/* Procedure used to free the
					 * value. */

    ClientData clientData;		/* Arbitrary one-word value
					 * used by option parser:
					 * passed to parseProc and
					 * printProc. */
} Blt_CustomOption;

/*
 * Structure used to specify information for Tk_ConfigureWidget.  Each
 * structure gives complete information for one option, including
 * how the option is specified on the command line, where it appears
 * in the option database, etc.
 */

typedef struct {
    int type;			/* Type of option, such as
				 * BLT_CONFIG_COLOR; see definitions
				 * below.  Last option in table must
				 * have type BLT_CONFIG_END. */

    const char *switchName;	/* Switch used to specify option in
				 * argv.  NULL means this spec is part
				 * of a group. */

    Tk_Uid dbName;		/* Name for option in option
				 * database. */

    Tk_Uid dbClass;		/* Class for option in database. */

    Tk_Uid defValue;		/* Default value for option if not
				 * specified in command line or
				 * database. */

    int offset;			/* Where in widget record to store
				 * value; use Blt_Offset macro to
				 * generate values for this. */

    int specFlags;		/* Any combination of the values
				 * defined below; other bits are used
				 * internally by tkConfig.c. */

    Blt_CustomOption *customPtr; /* If type is BLT_CONFIG_CUSTOM then
				 * this is a pointer to info about how
				 * to parse and print the option.
				 * Otherwise it is irrelevant. */
} Blt_ConfigSpec;

/*
 * Type values for Blt_ConfigSpec structures.  See the user
 * documentation for details.
 */
typedef enum {
    BLT_CONFIG_ACTIVE_CURSOR, 
    BLT_CONFIG_ANCHOR, 
    BLT_CONFIG_BITMAP,
    BLT_CONFIG_BOOLEAN, 
    BLT_CONFIG_BORDER, 
    BLT_CONFIG_CAP_STYLE, 
    BLT_CONFIG_COLOR, 
    BLT_CONFIG_CURSOR, 
    BLT_CONFIG_CUSTOM, 
    BLT_CONFIG_DOUBLE, 
    BLT_CONFIG_FONT, 
    BLT_CONFIG_INT, 
    BLT_CONFIG_JOIN_STYLE,
    BLT_CONFIG_JUSTIFY, 
    BLT_CONFIG_MM, 
    BLT_CONFIG_RELIEF, 
    BLT_CONFIG_STRING,
    BLT_CONFIG_SYNONYM, 
    BLT_CONFIG_WINDOW, 

    BLT_CONFIG_BITMASK,
    BLT_CONFIG_BITMASK_INVERT,
    BLT_CONFIG_DASHES,
    BLT_CONFIG_FILL,
    BLT_CONFIG_FLOAT, 
    BLT_CONFIG_INT_NNEG,	/* 0..N */
    BLT_CONFIG_INT_POS,		/* 1..N */
    BLT_CONFIG_LIST,
    BLT_CONFIG_LONG, 
    BLT_CONFIG_LONG_NNEG,	/* 0..N */
    BLT_CONFIG_LONG_POS,	/* 1..N */
    BLT_CONFIG_OBJ,
    BLT_CONFIG_PAD,
    BLT_CONFIG_PIXELS_NNEG,	/* 1.1c 2m 3.2i excluding negative
				   values. */
    BLT_CONFIG_PIXELS_POS,	/* 1.1c 2m 3.2i excluding negative
				 * values and zero. */
    BLT_CONFIG_PIXELS,		/* 1.1c 2m 3.2i. */
    BLT_CONFIG_SIDE,
    BLT_CONFIG_STATE, 
    BLT_CONFIG_BACKGROUND,
    BLT_CONFIG_PIX32, 

    BLT_CONFIG_TK_FONT, 
    BLT_CONFIG_END
} Blt_ConfigTypes;

/*
 * Possible values for flags argument to Tk_ConfigureWidget:
 */
#define BLT_CONFIG_OBJV_ONLY	1

/*
 * Possible flag values for Blt_ConfigSpec structures.  Any bits at or
 * above BLT_CONFIG_USER_BIT may be used by clients for selecting
 * certain entries.  Before changing any values here, coordinate with
 * tkOldConfig.c (internal-use-only flags are defined there).
 */
/*
 * Values for "flags" field of Blt_ConfigSpec structures.  Be sure to
 * coordinate these values with those defined in tk.h
 * (BLT_CONFIG_COLOR_ONLY, etc.).  There must not be overlap!
 *
 * INIT -		Non-zero means (char *) things have been
 *			converted to Tk_Uid's.
 */
#define INIT				(1<<0)
#define BLT_CONFIG_NULL_OK		(1<<1)
#define BLT_CONFIG_COLOR_ONLY		(1<<2)
#define BLT_CONFIG_MONO_ONLY		(1<<3)
#define BLT_CONFIG_DONT_SET_DEFAULT	(1<<4)
#define BLT_CONFIG_OPTION_SPECIFIED	(1<<5)
#define BLT_CONFIG_USER_BIT		(1<<8)


#define SIDE_LEFT		(1<<0)
#define SIDE_TOP		(1<<1)
#define SIDE_RIGHT		(1<<2)
#define SIDE_BOTTOM		(1<<3)

#define STATE_NORMAL		(0)
#define STATE_ACTIVE		(1<<0)
#define STATE_DISABLED		(1<<1)
#define STATE_EMPHASIS		(1<<2)

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Pad --
 *
 * 	Specifies vertical and horizontal padding.
 *
 *	Padding can be specified on a per side basis.  The fields
 *	side1 and side2 refer to the opposite sides, either
 *	horizontally or vertically.
 *
 *		side1	side2
 *              -----   -----
 *          x | left    right
 *	    y | top     bottom
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    unsigned short int side1, side2;
} Blt_Pad;

#define padLeft  	xPad.side1
#define padRight  	xPad.side2
#define padTop		yPad.side1
#define padBottom	yPad.side2
#define PADDING(x)	((x).side1 + (x).side2)

/*
 *---------------------------------------------------------------------------
 *
 * The following enumerated values are used as bit flags.
 *	FILL_NONE		Neither coordinate plane is specified 
 *	FILL_X			Horizontal plane.
 *	FILL_Y			Vertical plane.
 *	FILL_BOTH		Both vertical and horizontal planes.
 *
 *---------------------------------------------------------------------------
 */
#define FILL_NONE	0
#define FILL_X		1
#define FILL_Y		2
#define FILL_BOTH	3

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Dashes --
 *
 * 	List of dash values (maximum 11 based upon PostScript limit).
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    unsigned char values[12];
    int offset;
} Blt_Dashes;

#define LineIsDashed(d) ((d).values[0] != 0)

/*
 * Blt_Limits --
 *
 * 	Defines the bounding of a size (width or height) in the paneset.  It may
 * 	be related to the widget, pane or paneset size.
 */
typedef struct {
    int flags;				/* Flags indicate whether using default
					 * values for limits or not. See flags
					 * below. */
    int max, min;			/* Values for respective limits. */
    int nom;				/* Nominal starting value. */
} Blt_Limits;

#define LIMITS_MIN_SET	(1<<0)
#define LIMITS_MAX_SET	(1<<1)
#define LIMITS_NOM_SET	(1<<2)

#define LIMITS_MIN	0		/* Default minimum limit  */
#define LIMITS_MAX	SHRT_MAX	/* Default maximum limit */
#define LIMITS_NOM	-1000		/* Default nomimal value.  Indicates
					 * if a pane has received any space
					 * yet */

extern void Blt_SetDashes (Display *display, GC gc, Blt_Dashes *dashesPtr);
extern Blt_Dashes *Blt_GetDashes (GC gc);

extern void Blt_ResetLimits(Blt_Limits *limitsPtr);
extern int Blt_GetLimitsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr, Blt_Limits *limitsPtr);

extern int Blt_ConfigureInfoFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags);

extern int Blt_ConfigureValueFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Blt_ConfigSpec *specs, char *widgRec, Tcl_Obj *objPtr, int flags);

extern int Blt_ConfigureWidgetFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, 
	int flags);

extern int Blt_ConfigureComponentFromObj(Tcl_Interp *interp, 
	Tk_Window tkwin, const char *name, const char *className, 
	Blt_ConfigSpec *specs, int objc, Tcl_Obj *const *objv, char *widgRec, 
	int flags);

extern int Blt_ConfigModified TCL_VARARGS(Blt_ConfigSpec *, specs);

extern const char *Blt_NameOfState(int state);
extern const char *Blt_NameOfSide(int side);

extern void Blt_FreeOptions(Blt_ConfigSpec *specs, char *widgRec, 
	Display *display, int needFlags);

extern int Blt_ObjIsOption(Blt_ConfigSpec *specs, Tcl_Obj *objPtr, 
	int flags);

extern int Blt_GetSideFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *sidePtr);

extern int Blt_GetPixelsFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr, int flags, int *valuePtr);

extern int Blt_GetPadFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr, Blt_Pad *padPtr);

extern int Blt_GetStateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *statePtr);

extern int Blt_GetFillFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *fillPtr);

extern int Blt_GetDashesFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Dashes *dashesPtr);

#endif /* BLT_CONFIG_H */
