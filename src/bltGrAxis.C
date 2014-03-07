/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrAxis.c --
 *
 *	This module implements coordinate axes for the BLT graph widget.
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bltInt.h"
#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltGrAxis.h"
#include "bltGrElem.h"
#include "bltConfig.h"

#define AXIS_PAD_TITLE 2
#define MAXTICKS 10001
#define NUMDIGITS 15	/* Specifies the number of digits of
			 * accuracy used when outputting axis
			 * tick labels. */

#define AXIS_AUTO_MAJOR		(1<<16) /* Auto-generate major ticks. */
#define AXIS_AUTO_MINOR		(1<<17) /* Auto-generate minor ticks. */

#define FCLAMP(x)	((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define UROUND(x,u)		(Round((x)/(u))*(u))
#define UCEIL(x,u)		(ceil((x)/(u))*(u))
#define UFLOOR(x,u)		(floor((x)/(u))*(u))
#define HORIZMARGIN(m)	(!((m)->site & 0x1)) /* Even sites are horizontal */

typedef struct {
  int axis;				/* Length of the axis.  */
  int t1;			        /* Length of a major tick (in
					 * pixels). */
  int t2;			        /* Length of a minor tick (in
					 * pixels). */
  int label;				/* Distance from axis to tick label. */
} AxisInfo;

typedef struct {
  const char *name;
  ClassId classId;
  int margin, invertMargin;
} AxisName;

static AxisName axisNames[] = { 
  { "x",  CID_AXIS_X, MARGIN_BOTTOM, MARGIN_LEFT   },
  { "y",  CID_AXIS_Y, MARGIN_LEFT,   MARGIN_BOTTOM },
  { "x2", CID_AXIS_X, MARGIN_TOP,    MARGIN_RIGHT  },
  { "y2", CID_AXIS_Y, MARGIN_RIGHT,  MARGIN_TOP    }
} ;
static int nAxisNames = sizeof(axisNames) / sizeof(AxisName);

// Defs

static int AxisObjConfigure(Tcl_Interp* interp, Axis* axis,
			    int objc, Tcl_Obj* const objv[]);
static int GetAxisScrollInfo(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[],
			     double *offsetPtr, double windowSize,
			     double scrollUnits, double scale);
static double Clamp(double x);
static int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr);
static int AxisIsHorizontal(Axis *axisPtr);
static void FreeTickLabels(Blt_Chain chain);
static int ConfigureAxis(Axis *axisPtr);
static Axis *NewAxis(Graph* graphPtr, const char *name, int margin);
static void ReleaseAxis(Axis *axisPtr);
static int GetAxisByClass(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr,
			  ClassId classId, Axis **axisPtrPtr);
static void DestroyAxis(Axis *axisPtr);
static Tcl_FreeProc FreeAxis;

static int lastMargin;
typedef int (GraphDefAxisProc)(Tcl_Interp* interp, Axis *axisPtr, 
			       int objc, Tcl_Obj* const objv[]);
typedef int (GraphAxisProc)(Tcl_Interp* interp, Graph* graphPtr, 
			    int objc, Tcl_Obj* const objv[]);

// OptionSpecs

static Tk_CustomOptionSetProc AxisSetProc;
static Tk_CustomOptionGetProc AxisGetProc;
Tk_ObjCustomOption xAxisObjOption =
  {
    "xaxis", AxisSetProc, AxisGetProc, NULL, NULL, (ClientData)CID_AXIS_X
  };
Tk_ObjCustomOption yAxisObjOption =
  {
    "yaxis", AxisSetProc, AxisGetProc, NULL, NULL, (ClientData)CID_AXIS_Y
  };

static int AxisSetProc(ClientData clientData, Tcl_Interp* interp,
		       Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		       int offset, char* save, int flags)
{
  Axis** axisPtrPtr = (Axis**)(widgRec + offset);
  Graph* graphPtr = Blt_GetGraphFromWindowData(tkwin);
  ClassId classId = (ClassId)clientData;
  Axis* axisPtr;
  if (GetAxisByClass(interp, graphPtr, *objPtr, classId, &axisPtr) != TCL_OK)
    return TCL_ERROR;

  ReleaseAxis(*axisPtrPtr);
  *axisPtrPtr = axisPtr;

  return TCL_OK;
};

static Tcl_Obj* AxisGetProc(ClientData clientData, Tk_Window tkwin, 
			    char *widgRec, int offset)
{
  Axis*axisPtr = *(Axis**)(widgRec + offset);
  const char* name = (axisPtr ? axisPtr->obj.name : "");

  return Tcl_NewStringObj(name, -1);
};

static Tk_CustomOptionSetProc LimitSetProc;
static Tk_CustomOptionGetProc LimitGetProc;
Tk_ObjCustomOption limitObjOption =
  {
    "limit", LimitSetProc, LimitGetProc, NULL, NULL, NULL
  };

static int LimitSetProc(ClientData clientData, Tcl_Interp* interp,
			Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			int offset, char* save, int flags)
{
  double* limitPtr = (double*)(widgRec + offset);
  const char* string = Tcl_GetString(*objPtr);
  if (string[0] == '\0')
    *limitPtr = NAN;
  else if (Blt_ExprDoubleFromObj(interp, *objPtr, limitPtr) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}

static Tcl_Obj* LimitGetProc(ClientData clientData, Tk_Window tkwin, 
			     char *widgRec, int offset)
{
  double limit = *(double*)(widgRec + offset);
  Tcl_Obj* objPtr;

  if (!isnan(limit))
    objPtr = Tcl_NewDoubleObj(limit);
  else
    objPtr = Tcl_NewStringObj("", -1);

  return objPtr;
}

static Tk_CustomOptionSetProc FormatSetProc;
static Tk_CustomOptionGetProc FormatGetProc;
static Tk_CustomOptionFreeProc FormatFreeProc;
Tk_ObjCustomOption formatObjOption =
  {
    "format", FormatSetProc, FormatGetProc, NULL, FormatFreeProc, NULL
  };

static int FormatSetProc(ClientData clientData, Tcl_Interp* interp,
			 Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			 int offset, char* save, int flags)
{
  Axis *axisPtr = (Axis*)(widgRec);

  const char **argv;
  int argc;
  if (Tcl_SplitList(interp, Tcl_GetString(*objPtr), &argc, &argv) != TCL_OK)
    return TCL_ERROR;

  if (argc > 2) {
    Tcl_AppendResult(interp, "too many elements in limits format list \"",
		     Tcl_GetString(*objPtr), "\"", NULL);
    free(argv);
    return TCL_ERROR;
  }
  if (axisPtr->limitsFormats)
    Tcl_Free((char*)axisPtr->limitsFormats);

  axisPtr->limitsFormats = argv;
  axisPtr->nFormats = argc;

  return TCL_OK;
}

static Tcl_Obj* FormatGetProc(ClientData clientData, Tk_Window tkwin, 
			      char *widgRec, int offset)
{
  Axis *axisPtr = (Axis*)(widgRec);
  Tcl_Obj* objPtr;

  if (axisPtr->nFormats == 0)
    objPtr = Tcl_NewStringObj("", -1);
  else {
    char *string = Tcl_Merge(axisPtr->nFormats, axisPtr->limitsFormats); 
    objPtr = Tcl_NewStringObj(string, -1);
    free(string);
  }
  return objPtr;
}

static void FormatFreeProc(ClientData clientData, Tk_Window tkwin, char* ptr)
{
  //  if (ptr && *ptr)
  //    Tcl_Free(*(char**)ptr);
}

static Tk_CustomOptionSetProc UseSetProc;
static Tk_CustomOptionGetProc UseGetProc;
Tk_ObjCustomOption useObjOption =
  {
    "use", UseSetProc, UseGetProc, NULL, NULL, NULL
  };

static int UseSetProc(ClientData clientData, Tcl_Interp* interp,
		      Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		      int offset, char* save, int flags)
{
  Axis* axisPtr = (Axis*)(widgRec);

  Graph* graphPtr = axisPtr->obj.graphPtr;

  // Clear the axis class if it's not currently used by an element
  if (axisPtr->refCount == 0)
    Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);

  // Remove the axis from the margin's use list and clear its use flag.
  if (axisPtr->link)
    Blt_Chain_UnlinkLink(axisPtr->chain, axisPtr->link);

  axisPtr->use =0;
  const char *string = Tcl_GetString(*objPtr);
  if (!string || (string[0] == '\0'))
    goto done;

  AxisName *p, *pend;
  for (p = axisNames, pend = axisNames + nAxisNames; p < pend; p++) {
    if (strcmp(p->name, string) == 0) {
      break;			/* Found the axis name. */
    }
  }
  if (p == pend) {
    Tcl_AppendResult(interp, "unknown axis type \"", string, "\": "
		     "should be x, y, x1, y2, or \"\".", NULL);
    return TCL_ERROR;
  }

  // Check the axis class
  // Can't use the axis if it's already being used as another type.
  if (axisPtr->obj.classId == CID_NONE)
    Blt_GraphSetObjectClass(&axisPtr->obj, p->classId);
  else if (axisPtr->obj.classId != p->classId) {
    Tcl_AppendResult(interp, "wrong type for axis \"", 
		     axisPtr->obj.name, "\": can't use ", 
		     axisPtr->obj.className, " type axis.", NULL); 
    return TCL_ERROR;
  }

  int margin = (graphPtr->inverted) ? p->invertMargin : p->margin;
  Blt_Chain chain = graphPtr->margins[margin].axes;
  // Move the axis from the old margin's "use" list to the new.
  if (axisPtr->link)
    Blt_Chain_AppendLink(chain, axisPtr->link);
  else
    axisPtr->link = Blt_Chain_Append(chain, axisPtr);

  axisPtr->chain = chain;
  axisPtr->use =1;
  axisPtr->margin = margin;

 done:
  graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
  // When any axis changes, we need to layout the entire graph.
  graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static Tcl_Obj* UseGetProc(ClientData clientData, Tk_Window tkwin, 
			   char *widgRec, int offset)
{
  Axis* axisPtr = (Axis*)(widgRec);
    
  if (axisPtr->margin == MARGIN_NONE)
    return Tcl_NewStringObj("", -1);

  return Tcl_NewStringObj(axisNames[axisPtr->margin].name, -1);
}

static Tk_CustomOptionSetProc TicksSetProc;
static Tk_CustomOptionGetProc TicksGetProc;
Tk_ObjCustomOption majorTicksObjOption =
  {
    "majorTicks", TicksSetProc, TicksGetProc, NULL, NULL, 
    (ClientData)AXIS_AUTO_MAJOR,
  };
Tk_ObjCustomOption minorTicksObjOption =
  {
    "minorTicks", TicksSetProc, TicksGetProc, NULL, NULL, 
    (ClientData)AXIS_AUTO_MINOR,
  };

static int TicksSetProc(ClientData clientData, Tcl_Interp* interp,
			Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			int offset, char* save, int flags)
{
  Axis* axisPtr = (Axis*)widgRec;
  Ticks** ticksPtrPtr = (Ticks**)(widgRec + offset);
  unsigned long mask = (unsigned long)clientData;

  int objc;
  Tcl_Obj** objv;
  if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  axisPtr->flags |= mask;
  Ticks* ticksPtr = NULL;
  if (objc > 0) {
    ticksPtr = malloc(sizeof(Ticks) + (objc*sizeof(double)));
    for (int ii = 0; ii<objc; ii++) {
      double value;
      if (Blt_ExprDoubleFromObj(interp, objv[ii], &value) != TCL_OK) {
	free(ticksPtr);
	return TCL_ERROR;
      }
      ticksPtr->values[ii] = value;
    }
    ticksPtr->nTicks = objc;
    axisPtr->flags &= ~mask;
  }

  if (*ticksPtrPtr)
    free(*ticksPtrPtr);
  *ticksPtrPtr = ticksPtr;

  return TCL_OK;
}

static Tcl_Obj* TicksGetProc(ClientData clientData, Tk_Window tkwin, 
			     char *widgRec, int offset)
{
  Axis* axisPtr = (Axis*)widgRec;
  Ticks* ticksPtr = *(Ticks**) (widgRec + offset);
  unsigned long mask = (unsigned long)clientData;

  if (ticksPtr && !(axisPtr->flags & mask)) {
    int cnt = ticksPtr->nTicks;
    Tcl_Obj** ll = calloc(cnt, sizeof(Tcl_Obj*));
    for (int ii = 0; ii<cnt; ii++)
      ll[ii] = Tcl_NewDoubleObj(ticksPtr->values[ii]);

    Tcl_Obj* listObjPtr = Tcl_NewListObj(cnt, ll);
    free(ll);
    return listObjPtr;
  }
  else
    return Tcl_NewListObj(0, NULL);
}

static Tk_CustomOptionSetProc ObjectSetProc;
static Tk_CustomOptionGetProc ObjectGetProc;
static Tk_CustomOptionFreeProc ObjectFreeProc;
Tk_ObjCustomOption objectObjOption =
  {
    "object", ObjectSetProc, ObjectGetProc, NULL, ObjectFreeProc, NULL,
  };

static int ObjectSetProc(ClientData clientData, Tcl_Interp* interp,
			Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			int offset, char* save, int flags)
{
  Tcl_Obj** objectPtr = (Tcl_Obj**)(widgRec + offset);
  Tcl_IncrRefCount(*objPtr);
  if (*objectPtr)
    Tcl_DecrRefCount(*objectPtr);
  *objectPtr = *objPtr;

  return TCL_OK;
}
    
static Tcl_Obj* ObjectGetProc(ClientData clientData, Tk_Window tkwin, 
			     char *widgRec, int offset)
{
  Tcl_Obj** objectPtr = (Tcl_Obj**)(widgRec + offset);
  return *objectPtr;
}

static void ObjectFreeProc(ClientData clientData, Tk_Window tkwin, char* ptr)
{
  Tcl_Obj** objectPtr = (Tcl_Obj**)ptr;
  if (*objectPtr)
    Tcl_DecrRefCount(*objectPtr);
}

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_COLOR, "-activeforeground", "activeForeground", "ActiveForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(Axis, activeFgColor), 0, NULL, 0}, 
  {TK_OPTION_RELIEF, "-activerelief", "activeRelief", "Relief",
   "flat", -1, Tk_Offset(Axis, activeRelief), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-autorange", "autoRange", "AutoRange",
   "0", -1, Tk_Offset(Axis, windowSize), 0, NULL, 0},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   NULL, -1, Tk_Offset(Axis, normalBg), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags",
   "all", -1, Tk_Offset(Axis, obj.tags), TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   "0", -1, Tk_Offset(Axis, borderWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-checklimits", "checkLimits", "CheckLimits", 
   "no", -1, Tk_Offset(Axis, checkLimits), 0, NULL, 0},
  {TK_OPTION_COLOR, "-color", "color", "Color",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Axis, tickColor), 0, NULL, 0},
  {TK_OPTION_STRING, "-command", "command", "Command",
   NULL, -1, Tk_Offset(Axis, formatCmd), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-descending", "descending", "Descending",
   "no", -1, Tk_Offset(Axis, descending), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-exterior", "exterior", "exterior",
   "yes", -1, Tk_Offset(Axis, exterior), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-color", 0},
  {TK_OPTION_SYNONYM, "-foreground", NULL, NULL, NULL, -1, 0, 0, "-color", 0},
  {TK_OPTION_BOOLEAN, "-grid", "grid", "Grid",
   "yes", -1, Tk_Offset(Axis, showGrid), 0, NULL, 0},
  {TK_OPTION_COLOR, "-gridcolor", "gridColor", "GridColor", 
   "gray64", -1, Tk_Offset(Axis, major.color), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-griddashes", "gridDashes", "GridDashes", 
   "dot", -1, Tk_Offset(Axis, major.dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_PIXELS, "-gridlinewidth", "gridLineWidth", "GridLineWidth",
   "0", -1, Tk_Offset(Axis, major.lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-gridminor", "gridMinor", "GridMinor", 
   "yes", -1, Tk_Offset(Axis, showGridMinor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-gridminorcolor", "gridMinorColor", "GridMinorColor", 
   "gray64", -1, Tk_Offset(Axis, minor.color), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-gridminordashes", "gridMinorDashes", "GridMinorDashes", 
   "dot", -1, Tk_Offset(Axis, minor.dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_PIXELS, "-gridminorlinewidth", "gridMinorLineWidth", 
   "GridMinorLineWidth",
   "0", -1, Tk_Offset(Axis, minor.lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide",
   "no", -1, Tk_Offset(Axis, hide), 0, NULL, 0},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
   "c", -1, Tk_Offset(Axis, titleJustify), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-labeloffset", "labelOffset", "LabelOffset",
   "no", -1, Tk_Offset(Axis, labelOffset), 0, NULL, 0},
  {TK_OPTION_COLOR, "-limitscolor", "limitsColor", "LimitsColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Axis, limitsTextStyle.color), 
   0, NULL, 0},
  {TK_OPTION_FONT, "-limitsfont", "limitsFont", "LimitsFont",
   STD_FONT_SMALL, -1, Tk_Offset(Axis, limitsTextStyle.font), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-limitsformat", "limitsFormat", "LimitsFormat",
   NULL, -1, Tk_Offset(Axis, limitsFormats), 
   TK_OPTION_NULL_OK, &formatObjOption, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(Axis, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-logscale", "logScale", "LogScale",
   "no", -1, Tk_Offset(Axis, logScale), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-loosemin", "looseMin", "LooseMin", 
   "no", -1, Tk_Offset(Axis, looseMin), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-loosemax", "looseMax", "LooseMax", 
   "no", -1, Tk_Offset(Axis, looseMax), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-majorticks", "majorTicks", "MajorTicks",
   NULL, -1, Tk_Offset(Axis, t1Ptr), 
   TK_OPTION_NULL_OK, &majorTicksObjOption, 0},
  {TK_OPTION_CUSTOM, "-max", "max", "Max", 
   NULL, -1, Tk_Offset(Axis, reqMax), TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_CUSTOM, "-min", "min", "Min", 
   NULL, -1, Tk_Offset(Axis, reqMin), TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_CUSTOM, "-minorticks", "minorTicks", "MinorTicks",
   NULL, -1, Tk_Offset(Axis, t2Ptr), 
   TK_OPTION_NULL_OK, &minorTicksObjOption, 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "flat", -1, Tk_Offset(Axis, relief), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-rotate", "rotate", "Rotate", 
   "0", -1, Tk_Offset(Axis, tickAngle), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-scrollcommand", "scrollCommand", "ScrollCommand",
   NULL, -1, Tk_Offset(Axis, scrollCmdObjPtr), 
   TK_OPTION_NULL_OK, &objectObjOption, 0},
  {TK_OPTION_PIXELS, "-scrollincrement", "scrollIncrement", "ScrollIncrement",
   "10", -1, Tk_Offset(Axis, scrollUnits), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-scrollmax", "scrollMax", "ScrollMax", 
   NULL, -1, Tk_Offset(Axis, reqScrollMax),  
   TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_CUSTOM, "-scrollmin", "scrollMin", "ScrollMin", 
   NULL, -1, Tk_Offset(Axis, reqScrollMin), 
   TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_DOUBLE, "-shiftby", "shiftBy", "ShiftBy",
   "0", -1, Tk_Offset(Axis, shiftBy), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-showticks", "showTicks", "ShowTicks",
   "yes", -1, Tk_Offset(Axis, showTicks), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-stepsize", "stepSize", "StepSize",
   "0", -1, Tk_Offset(Axis, reqStep), 0, NULL, 0},
  {TK_OPTION_INT, "-subdivisions", "subdivisions", "Subdivisions",
   "2", -1, Tk_Offset(Axis, reqNumMinorTicks), 0, NULL, 0},
  {TK_OPTION_ANCHOR, "-tickanchor", "tickAnchor", "Anchor",
   "c", -1, Tk_Offset(Axis, reqTickAnchor), 0, NULL, 0},
  {TK_OPTION_FONT, "-tickfont", "tickFont", "Font",
   STD_FONT_SMALL, -1, Tk_Offset(Axis, tickFont), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-ticklength", "tickLength", "TickLength",
   "8", -1, Tk_Offset(Axis, tickLength), 0, NULL, 0},
  {TK_OPTION_INT, "-tickdefault", "tickDefault", "TickDefault",
   "4", -1, Tk_Offset(Axis, reqNumMajorTicks), 0, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title",
   NULL, -1, Tk_Offset(Axis, title), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-titlealternate", "titleAlternate", "TitleAlternate",
   "no", -1, Tk_Offset(Axis, titleAlternate), 0, NULL, 0},
  {TK_OPTION_COLOR, "-titlecolor", "titleColor", "TitleColor", 
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(Axis, titleColor), 0, NULL, 0},
  {TK_OPTION_FONT, "-titlefont", "titleFont", "TitleFont",
   STD_FONT_NORMAL, -1, Tk_Offset(Axis, titleFont), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-use", "use", "Use", 
   NULL, -1, 0, TK_OPTION_NULL_OK, &useObjOption, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

int Blt_CreateAxes(Graph* graphPtr)
{
  for (int ii = 0; ii < 4; ii++) {
    int isNew;
    Tcl_HashEntry* hPtr = 
      Tcl_CreateHashEntry(&graphPtr->axes.table, axisNames[ii].name, &isNew);
    Blt_Chain chain = Blt_Chain_Create();

    Axis* axisPtr = NewAxis(graphPtr, axisNames[ii].name, ii);
    if (!axisPtr)
      return TCL_ERROR;

    axisPtr->hashPtr = hPtr;
    Tcl_SetHashValue(hPtr, axisPtr);

    axisPtr->refCount = 1;	/* Default axes are assumed in use. */
    axisPtr->margin = ii;
    axisPtr->use =1;
    Blt_GraphSetObjectClass(&axisPtr->obj, axisNames[ii].classId);

    if (Tk_InitOptions(graphPtr->interp, (char*)axisPtr, 
		       axisPtr->optionTable, graphPtr->tkwin) != TCL_OK)
      return TCL_ERROR;

    if (ConfigureAxis(axisPtr) != TCL_OK)
      return TCL_ERROR;

    if ((axisPtr->margin == MARGIN_RIGHT) || (axisPtr->margin == MARGIN_TOP))
      axisPtr->hide = 1;

    graphPtr->axisChain[ii] = chain;
    axisPtr->link = Blt_Chain_Append(chain, axisPtr);
    axisPtr->chain = chain;
  }
  return TCL_OK;
}

static int CreateAxis(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  char *string = Tcl_GetString(objv[3]);
  if (string[0] == '-') {
    Tcl_AppendResult(graphPtr->interp, "name of axis \"", string, 
		     "\" can't start with a '-'", NULL);
    return TCL_ERROR;
  }

  Axis* axisPtr;
  int isNew;
  Tcl_HashEntry* hPtr = 
    Tcl_CreateHashEntry(&graphPtr->axes.table, string, &isNew);
  if (!isNew) {
    axisPtr = Tcl_GetHashValue(hPtr);
    if ((axisPtr->flags & DELETE_PENDING) == 0) {
      Tcl_AppendResult(graphPtr->interp, "axis \"", string,
		       "\" already exists in \"", Tcl_GetString(objv[0]),
		       "\"", NULL);
      return TCL_ERROR;
    }
    axisPtr->flags &= ~DELETE_PENDING;
  }
  else {
    axisPtr = NewAxis(graphPtr, Tcl_GetString(objv[3]), MARGIN_NONE);
    if (!axisPtr)
      return TCL_ERROR;
    axisPtr->hashPtr = hPtr;
    Tcl_SetHashValue(hPtr, axisPtr);
  }

  if ((Tk_InitOptions(graphPtr->interp, (char*)axisPtr, axisPtr->optionTable, graphPtr->tkwin) != TCL_OK) || (AxisObjConfigure(interp, axisPtr, objc-4, objv+4) != TCL_OK)) {
    DestroyAxis(axisPtr);
    return TCL_ERROR;
  }

  graphPtr->flags |= CACHE_DIRTY;
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static Axis *NewAxis(Graph* graphPtr, const char *name, int margin)
{
  Axis *axisPtr = calloc(1, sizeof(Axis));
  axisPtr->obj.name = Blt_Strdup(name);
  Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
  axisPtr->obj.graphPtr = graphPtr;
  axisPtr->reqMin =NAN;
  axisPtr->reqMax =NAN;
  axisPtr->reqScrollMin =NAN;
  axisPtr->reqScrollMax =NAN;
  axisPtr->margin =margin;
  axisPtr->use =0;
  axisPtr->flags = (AXIS_AUTO_MAJOR|AXIS_AUTO_MINOR);

  Blt_Ts_InitStyle(axisPtr->limitsTextStyle);
  axisPtr->tickLabels = Blt_Chain_Create();

  axisPtr->optionTable = 
    Tk_CreateOptionTable(graphPtr->interp, optionSpecs);
  return axisPtr;
}

static void DestroyAxis(Axis *axisPtr)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (graphPtr->bindTable)
    Blt_DeleteBindings(graphPtr->bindTable, axisPtr);

  if (axisPtr->link)
    Blt_Chain_DeleteLink(axisPtr->chain, axisPtr->link);

  if (axisPtr->obj.name)
    free((void*)(axisPtr->obj.name));

  if (axisPtr->hashPtr)
    Tcl_DeleteHashEntry(axisPtr->hashPtr);

  Blt_Ts_FreeStyle(graphPtr->display, &axisPtr->limitsTextStyle);

  if (axisPtr->tickGC)
    Tk_FreeGC(graphPtr->display, axisPtr->tickGC);

  if (axisPtr->activeTickGC)
    Tk_FreeGC(graphPtr->display, axisPtr->activeTickGC);

  if (axisPtr->major.gc) 
    Blt_FreePrivateGC(graphPtr->display, axisPtr->major.gc);

  if (axisPtr->minor.gc)
    Blt_FreePrivateGC(graphPtr->display, axisPtr->minor.gc);

  FreeTickLabels(axisPtr->tickLabels);

  Blt_Chain_Destroy(axisPtr->tickLabels);

  if (axisPtr->segments)
    free(axisPtr->segments);

  Tk_FreeConfigOptions((char*)axisPtr, axisPtr->optionTable, graphPtr->tkwin);
  free(axisPtr);
}

// Configure

static int CgetOp(Tcl_Interp* interp, Axis *axisPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)axisPtr, 
				      axisPtr->optionTable,
				      objv[3], graphPtr->tkwin);
  if (!objPtr)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int AxisCgetOp(Tcl_Interp* interp, Graph* graphPtr, 
		      int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return CgetOp(interp, axisPtr, objc-1, objv+1);
}

static int ConfigureOp(Tcl_Interp* interp, Axis *axisPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, (char*)axisPtr, 
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

static int AxisConfigureOp(Tcl_Interp* interp, Graph* graphPtr, int objc, 
			   Tcl_Obj* const objv[])
{
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  return ConfigureOp(interp, axisPtr, objc-1, objv+1);
}

static int AxisObjConfigure(Tcl_Interp* interp, Axis* axisPtr,
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
      Axis *axisPtr = Blt_Chain_GetValue(link);
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

    axisPtr = Blt_Chain_GetValue(link);
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
  {"activate",     1, ActivateOp,     3, 3, "",},
  {"bind",         1, BindOp,         2, 5, "sequence command",},
  {"cget",         2, CgetOp,         4, 4, "option",},
  {"configure",    2, ConfigureOp,    3, 0, "?option value?...",},
  {"deactivate",   1, ActivateOp,     3, 3, "",},
  {"invtransform", 1, InvTransformOp, 4, 4, "value",},
  {"limits",       1, LimitsOp,       3, 3, "",},
  {"transform",    1, TransformOp,    4, 4, "value",},
  {"use",          1, UseOp,          3, 4, "?axisName?",},
  {"view",         1, ViewOp,         3, 6, "?moveto fract? ",},
};

static int nDefAxisOps = sizeof(defAxisOps) / sizeof(Blt_OpSpec);

int Blt_DefAxisOp(Tcl_Interp* interp, Graph* graphPtr, int margin, 
		  int objc, Tcl_Obj* const objv[])
{
  GraphDefAxisProc* proc = Blt_GetOpFromObj(interp, nDefAxisOps, defAxisOps, 
					    BLT_OP_ARG2, objc, objv, 0);
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
      const char *tagName = Tcl_GetHashKey(&graphPtr->axes.tagTable, hPtr);
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
  Axis *axisPtr;
  if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)
    return TCL_ERROR;

  axisPtr->flags |= DELETE_PENDING;
  if (axisPtr->refCount == 0)
    Tcl_EventuallyFree(axisPtr, FreeAxis);

  return TCL_OK;
}

static int AxisFocusOp(Tcl_Interp* interp, Graph* graphPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  if (objc > 3) {
    Axis *axisPtr = NULL;
    const char *string = Tcl_GetString(objv[3]);
    if ((string[0] != '\0') && 
	(GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK))
      return TCL_ERROR;

    graphPtr->focusPtr = NULL;
    if (axisPtr && !axisPtr->hide && axisPtr->use)
      graphPtr->focusPtr = axisPtr;

    Blt_SetFocusItem(graphPtr->bindTable, graphPtr->focusPtr, NULL);
  }
  /* Return the name of the axis that has focus. */
  if (graphPtr->focusPtr)
    Tcl_SetStringObj(Tcl_GetObjResult(interp), graphPtr->focusPtr->obj.name,-1);

  return TCL_OK;
}

static int AxisGetOp(Tcl_Interp* interp, Graph* graphPtr, 
		     int objc, Tcl_Obj* const objv[])
{
  Axis *axisPtr = Blt_GetCurrentItem(graphPtr->bindTable);
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
      Axis *axisPtr = Tcl_GetHashValue(hPtr);
      if (axisPtr->flags & DELETE_PENDING)
	continue;

      Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(axisPtr->obj.name, -1));
    }
  } 
  else {
    Tcl_HashSearch cursor;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
      Axis *axisPtr = Tcl_GetHashValue(hPtr);
      for (int ii=3; ii<objc; ii++) {
	const char *pattern = Tcl_GetString(objv[ii]);
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
  {"activate",     1, AxisActivateOp,     4, 4, "axisName"},
  {"bind",         1, AxisBindOp,         3, 6, "axisName sequence command"},
  {"cget",         2, AxisCgetOp,         5, 5, "axisName option"},
  {"configure",    2, AxisConfigureOp,    4, 0, "axisName ?axisName?... "
   "?option value?..."},
  {"create",       2, AxisCreateOp,       4, 0, "axisName ?option value?..."},
  {"deactivate",   3, AxisActivateOp,     4, 4, "axisName"},
  {"delete",       3, AxisDeleteOp,       3, 0, "?axisName?..."},
  {"focus",        1, AxisFocusOp,        3, 4, "?axisName?"},
  {"get",          1, AxisGetOp,          4, 4, "name"},
  {"invtransform", 1, AxisInvTransformOp, 5, 5, "axisName value"},
  {"limits",       1, AxisLimitsOp,       4, 4, "axisName"},
  {"margin",       1, AxisMarginOp,       4, 4, "axisName"},
  {"names",        1, AxisNamesOp,        3, 0, "?pattern?..."},
  {"transform",    2, AxisTransformOp,    5, 5, "axisName value"},
  {"type",         2, AxisTypeOp,       4, 4, "axisName"},
  {"view",         1, AxisViewOp,         4, 7, "axisName ?moveto fract? "
   "?scroll number what?"},
};
static int nAxisOps = sizeof(axisOps) / sizeof(Blt_OpSpec);

int Blt_AxisOp(Graph* graphPtr, Tcl_Interp* interp, 
	       int objc, Tcl_Obj* const objv[])
{
  GraphAxisProc* proc = 
    Blt_GetOpFromObj(interp, nAxisOps, axisOps, BLT_OP_ARG2, objc, objv, 0);
  if (!proc)
    return TCL_ERROR;

  return (*proc)(interp, graphPtr, objc, objv);
}

// Support

static double Clamp(double x) 
{
  return (x < 0.0) ? 0.0 : (x > 1.0) ? 1.0 : x;
}

static int Round(double x)
{
  return (int) (x + ((x < 0.0) ? -0.5 : 0.5));
}

static void SetAxisRange(AxisRange *rangePtr, double min, double max)
{
  rangePtr->min = min;
  rangePtr->max = max;
  rangePtr->range = max - min;
  if (fabs(rangePtr->range) < DBL_EPSILON) {
    rangePtr->range = 1.0;
  }
  rangePtr->scale = 1.0 / rangePtr->range;
}

static int InRange(double x, AxisRange *rangePtr)
{
  if (rangePtr->range < DBL_EPSILON) {
    return (fabs(rangePtr->max - x) >= DBL_EPSILON);
  } else {
    double norm;

    norm = (x - rangePtr->min) * rangePtr->scale;
    return ((norm >= -DBL_EPSILON) && ((norm - 1.0) < DBL_EPSILON));
  }
}

static int AxisIsHorizontal(Axis *axisPtr)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  return ((axisPtr->obj.classId == CID_AXIS_Y) == graphPtr->inverted);
}

static void ReleaseAxis(Axis *axisPtr)
{
  if (axisPtr) {
    axisPtr->refCount--;
    if (axisPtr->refCount == 0) {
      axisPtr->flags |= DELETE_PENDING;
      Tcl_EventuallyFree(axisPtr, FreeAxis);
    }
  }
}

static void FreeTickLabels(Blt_Chain chain)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(chain); link != NULL; 
       link = Blt_Chain_NextLink(link)) {
    TickLabel *labelPtr;

    labelPtr = Blt_Chain_GetValue(link);
    free(labelPtr);
  }
  Blt_Chain_Reset(chain);
}

static TickLabel *MakeLabel(Axis *axisPtr, double value)
{
#define TICK_LABEL_SIZE		200
  char string[TICK_LABEL_SIZE + 1];
  TickLabel *labelPtr;

  /* Generate a default tick label based upon the tick value.  */
  if (axisPtr->logScale) {
    sprintf_s(string, TICK_LABEL_SIZE, "1E%d", ROUND(value));
  } else {
    sprintf_s(string, TICK_LABEL_SIZE, "%.*G", NUMDIGITS, value);
  }

  if (axisPtr->formatCmd) {
    Graph* graphPtr;
    Tcl_Interp* interp;
    Tk_Window tkwin;
	
    graphPtr = axisPtr->obj.graphPtr;
    interp = graphPtr->interp;
    tkwin = graphPtr->tkwin;
    /*
     * A TCL proc was designated to format tick labels. Append the path
     * name of the widget and the default tick label as arguments when
     * invoking it. Copy and save the new label from interp->result.
     */
    Tcl_ResetResult(interp);
    if (Tcl_VarEval(interp, axisPtr->formatCmd, " ", Tk_PathName(tkwin),
		    " ", string, NULL) != TCL_OK) {
      Tcl_BackgroundError(interp);
    } else {
      /* 
       * The proc could return a string of any length, so arbitrarily
       * limit it to what will fit in the return string.
       */
      strncpy(string, Tcl_GetStringResult(interp), TICK_LABEL_SIZE);
      string[TICK_LABEL_SIZE] = '\0';
	    
      Tcl_ResetResult(interp); /* Clear the interpreter's result. */
    }
  }
  labelPtr = malloc(sizeof(TickLabel) + strlen(string));
  strcpy(labelPtr->string, string);
  labelPtr->anchorPos.x = DBL_MAX;
  labelPtr->anchorPos.y = DBL_MAX;
  return labelPtr;
}

double Blt_InvHMap(Axis *axisPtr, double x)
{
  double value;

  x = (double)(x - axisPtr->screenMin) * axisPtr->screenScale;
  if (axisPtr->descending) {
    x = 1.0 - x;
  }
  value = (x * axisPtr->axisRange.range) + axisPtr->axisRange.min;
  if (axisPtr->logScale) {
    value = EXP10(value);
  }
  return value;
}

double Blt_InvVMap(Axis *axisPtr, double y) /* Screen coordinate */
{
  double value;

  y = (double)(y - axisPtr->screenMin) * axisPtr->screenScale;
  if (axisPtr->descending) {
    y = 1.0 - y;
  }
  value = ((1.0 - y) * axisPtr->axisRange.range) + axisPtr->axisRange.min;
  if (axisPtr->logScale) {
    value = EXP10(value);
  }
  return value;
}

double Blt_HMap(Axis *axisPtr, double x)
{
  if ((axisPtr->logScale) && (x != 0.0)) {
    x = log10(fabs(x));
  }
  /* Map graph coordinate to normalized coordinates [0..1] */
  x = (x - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
  if (axisPtr->descending) {
    x = 1.0 - x;
  }
  return (x * axisPtr->screenRange + axisPtr->screenMin);
}

double Blt_VMap(Axis *axisPtr, double y)
{
  if ((axisPtr->logScale) && (y != 0.0)) {
    y = log10(fabs(y));
  }
  /* Map graph coordinate to normalized coordinates [0..1] */
  y = (y - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
  if (axisPtr->descending) {
    y = 1.0 - y;
  }
  return ((1.0 - y) * axisPtr->screenRange + axisPtr->screenMin);
}

Point2d Blt_Map2D(Graph* graphPtr, double x, double y, Axis2d *axesPtr)
{
  Point2d point;

  if (graphPtr->inverted) {
    point.x = Blt_HMap(axesPtr->y, y);
    point.y = Blt_VMap(axesPtr->x, x);
  } else {
    point.x = Blt_HMap(axesPtr->x, x);
    point.y = Blt_VMap(axesPtr->y, y);
  }
  return point;
}

Point2d Blt_InvMap2D(Graph* graphPtr, double x, double y, Axis2d *axesPtr)
{
  Point2d point;

  if (graphPtr->inverted) {
    point.x = Blt_InvVMap(axesPtr->x, y);
    point.y = Blt_InvHMap(axesPtr->y, x);
  } else {
    point.x = Blt_InvHMap(axesPtr->x, x);
    point.y = Blt_InvVMap(axesPtr->y, y);
  }
  return point;
}

static void GetDataLimits(Axis *axisPtr, double min, double max)
{
  if (axisPtr->valueRange.min > min) {
    axisPtr->valueRange.min = min;
  }
  if (axisPtr->valueRange.max < max) {
    axisPtr->valueRange.max = max;
  }
}

static void FixAxisRange(Axis *axisPtr)
{
  double min, max;

  /*
   * When auto-scaling, the axis limits are the bounds of the element data.
   * If no data exists, set arbitrary limits (wrt to log/linear scale).
   */
  min = axisPtr->valueRange.min;
  max = axisPtr->valueRange.max;

  /* Check the requested axis limits. Can't allow -min to be greater
   * than -max, or have undefined log scale limits.  */
  if (((!isnan(axisPtr->reqMin)) && (!isnan(axisPtr->reqMax))) &&
      (axisPtr->reqMin >= axisPtr->reqMax)) {
    axisPtr->reqMin = axisPtr->reqMax = NAN;
  }
  if (axisPtr->logScale) {
    if ((!isnan(axisPtr->reqMin)) && (axisPtr->reqMin <= 0.0)) {
      axisPtr->reqMin = NAN;
    }
    if ((!isnan(axisPtr->reqMax)) && (axisPtr->reqMax <= 0.0)) {
      axisPtr->reqMax = NAN;
    }
  }

  if (min == DBL_MAX) {
    if (!isnan(axisPtr->reqMin)) {
      min = axisPtr->reqMin;
    } else {
      min = (axisPtr->logScale) ? 0.001 : 0.0;
    }
  }
  if (max == -DBL_MAX) {
    if (!isnan(axisPtr->reqMax)) {
      max = axisPtr->reqMax;
    } else {
      max = 1.0;
    }
  }
  if (min >= max) {
    /*
     * There is no range of data (i.e. min is not less than max), so
     * manufacture one.
     */
    if (min == 0.0) {
      min = 0.0, max = 1.0;
    } else {
      max = min + (fabs(min) * 0.1);
    }
  }
  SetAxisRange(&axisPtr->valueRange, min, max);

  /*   
   * The axis limits are either the current data range or overridden by the
   * values selected by the user with the -min or -max options.
   */
  axisPtr->min = min;
  axisPtr->max = max;
  if (!isnan(axisPtr->reqMin)) {
    axisPtr->min = axisPtr->reqMin;
  }
  if (!isnan(axisPtr->reqMax)) { 
    axisPtr->max = axisPtr->reqMax;
  }
  if (axisPtr->max < axisPtr->min) {
    /*   
     * If the limits still don't make sense, it's because one limit
     * configuration option (-min or -max) was set and the other default
     * (based upon the data) is too small or large.  Remedy this by making
     * up a new min or max from the user-defined limit.
     */
    if (isnan(axisPtr->reqMin)) {
      axisPtr->min = axisPtr->max - (fabs(axisPtr->max) * 0.1);
    }
    if (isnan(axisPtr->reqMax)) {
      axisPtr->max = axisPtr->min + (fabs(axisPtr->max) * 0.1);
    }
  }
  /* 
   * If a window size is defined, handle auto ranging by shifting the axis
   * limits.
   */
  if ((axisPtr->windowSize > 0.0) && 
      (isnan(axisPtr->reqMin)) && (isnan(axisPtr->reqMax))) {
    if (axisPtr->shiftBy < 0.0) {
      axisPtr->shiftBy = 0.0;
    }
    max = axisPtr->min + axisPtr->windowSize;
    if (axisPtr->max >= max) {
      if (axisPtr->shiftBy > 0.0) {
	max = UCEIL(axisPtr->max, axisPtr->shiftBy);
      }
      axisPtr->min = max - axisPtr->windowSize;
    }
    axisPtr->max = max;
  }
  if ((axisPtr->max != axisPtr->prevMax) || 
      (axisPtr->min != axisPtr->prevMin)) {
    /* Indicate if the axis limits have changed */
    axisPtr->flags |= DIRTY;
    /* and save the previous minimum and maximum values */
    axisPtr->prevMin = axisPtr->min;
    axisPtr->prevMax = axisPtr->max;
  }
}

// Reference: Paul Heckbert, "Nice Numbers for Graph Labels",
// Graphics Gems, pp 61-63.  
double NiceNum(double x, int round)
{
  double expt;			/* Exponent of x */
  double frac;			/* Fractional part of x */
  double nice;			/* Nice, rounded fraction */

  expt = floor(log10(x));
  frac = x / EXP10(expt);		/* between 1 and 10 */
  if (round) {
    if (frac < 1.5) {
      nice = 1.0;
    } else if (frac < 3.0) {
      nice = 2.0;
    } else if (frac < 7.0) {
      nice = 5.0;
    } else {
      nice = 10.0;
    }
  } else {
    if (frac <= 1.0) {
      nice = 1.0;
    } else if (frac <= 2.0) {
      nice = 2.0;
    } else if (frac <= 5.0) {
      nice = 5.0;
    } else {
      nice = 10.0;
    }
  }
  return nice * EXP10(expt);
}

static Ticks *GenerateTicks(TickSweep *sweepPtr)
{
  Ticks *ticksPtr;

  ticksPtr = malloc(sizeof(Ticks) + 
		    (sweepPtr->nSteps * sizeof(double)));
  ticksPtr->nTicks = 0;

  if (sweepPtr->step == 0.0) { 
    /* Hack: A zero step indicates to use log values. */
    int i;
    /* Precomputed log10 values [1..10] */
    static double logTable[] = {
      0.0, 
      0.301029995663981, 
      0.477121254719662, 
      0.602059991327962, 
      0.698970004336019, 
      0.778151250383644, 
      0.845098040014257,
      0.903089986991944, 
      0.954242509439325, 
      1.0
    };
    for (i = 0; i < sweepPtr->nSteps; i++) {
      ticksPtr->values[i] = logTable[i];
    }
  } else {
    double value;
    int i;
    
    value = sweepPtr->initial;	/* Start from smallest axis tick */
    for (i = 0; i < sweepPtr->nSteps; i++) {
      value = UROUND(value, sweepPtr->step);
      ticksPtr->values[i] = value;
      value += sweepPtr->step;
    }
  }
  ticksPtr->nTicks = sweepPtr->nSteps;
  return ticksPtr;
}

static void LogScaleAxis(Axis *axisPtr, double min, double max)
{
  double range;
  double tickMin, tickMax;
  double majorStep, minorStep;
  int nMajor, nMinor;

  nMajor = nMinor = 0;
  /* Suppress compiler warnings. */
  majorStep = minorStep = 0.0;
  tickMin = tickMax = NAN;
  if (min < max) {
    min = (min != 0.0) ? log10(fabs(min)) : 0.0;
    max = (max != 0.0) ? log10(fabs(max)) : 1.0;

    tickMin = floor(min);
    tickMax = ceil(max);
    range = tickMax - tickMin;
	
    if (range > 10) {
      /* There are too many decades to display a major tick at every
       * decade.  Instead, treat the axis as a linear scale.  */
      range = NiceNum(range, 0);
      majorStep = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
      tickMin = UFLOOR(tickMin, majorStep);
      tickMax = UCEIL(tickMax, majorStep);
      nMajor = (int)((tickMax - tickMin) / majorStep) + 1;
      minorStep = EXP10(floor(log10(majorStep)));
      if (minorStep == majorStep) {
	nMinor = 4, minorStep = 0.2;
      } else {
	nMinor = Round(majorStep / minorStep) - 1;
      }
    } else {
      if (tickMin == tickMax) {
	tickMax++;
      }
      majorStep = 1.0;
      nMajor = (int)(tickMax - tickMin + 1); /* FIXME: Check this. */
	    
      minorStep = 0.0;		/* This is a special hack to pass
				 * information to the GenerateTicks
				 * routine. An interval of 0.0 tells 1)
				 * this is a minor sweep and 2) the axis
				 * is log scale. */
      nMinor = 10;
    }
    if (!axisPtr->looseMin || (axisPtr->looseMin && !isnan(axisPtr->reqMin))) {
      tickMin = min;
      nMajor++;
    }
    if (!axisPtr->looseMax || (axisPtr->looseMax && !isnan(axisPtr->reqMax))) {
      tickMax = max;
    }
  }
  axisPtr->majorSweep.step = majorStep;
  axisPtr->majorSweep.initial = floor(tickMin);
  axisPtr->majorSweep.nSteps = nMajor;
  axisPtr->minorSweep.initial = axisPtr->minorSweep.step = minorStep;
  axisPtr->minorSweep.nSteps = nMinor;

  SetAxisRange(&axisPtr->axisRange, tickMin, tickMax);
}

static void LinearScaleAxis(Axis *axisPtr, double min, double max)
{
  double step;
  double tickMin, tickMax;
  double axisMin, axisMax;
  unsigned int nTicks;

  nTicks = 0;
  step = 1.0;
  /* Suppress compiler warning. */
  axisMin = axisMax = tickMin = tickMax = NAN;
  if (min < max) {
    double range;

    range = max - min;
    /* Calculate the major tick stepping. */
    if (axisPtr->reqStep > 0.0) {
      /* An interval was designated by the user.  Keep scaling it until
       * it fits comfortably within the current range of the axis.  */
      step = axisPtr->reqStep;
      while ((2 * step) >= range) {
	step *= 0.5;
      }
    } else {
      range = NiceNum(range, 0);
      step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
    }
	
    /* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
    axisMin = tickMin = floor(min / step) * step + 0.0;
    axisMax = tickMax = ceil(max / step) * step + 0.0;
	
    nTicks = Round((tickMax - tickMin) / step) + 1;
  } 
  axisPtr->majorSweep.step = step;
  axisPtr->majorSweep.initial = tickMin;
  axisPtr->majorSweep.nSteps = nTicks;

  /*
   * The limits of the axis are either the range of the data ("tight") or at
   * the next outer tick interval ("loose").  The looseness or tightness has
   * to do with how the axis fits the range of data values.  This option is
   * overridden when the user sets an axis limit (by either -min or -max
   * option).  The axis limit is always at the selected limit (otherwise we
   * assume that user would have picked a different number).
   */
  if (!axisPtr->looseMin || (axisPtr->looseMin && !isnan(axisPtr->reqMin)))
    axisMin = min;

  if (!axisPtr->looseMax || (axisPtr->looseMax && !isnan(axisPtr->reqMax)))
    axisMax = max;

  SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

  /* Now calculate the minor tick step and number. */

  if ((axisPtr->reqNumMinorTicks > 0) && (axisPtr->flags & AXIS_AUTO_MAJOR)) {
    nTicks = axisPtr->reqNumMinorTicks - 1;
    step = 1.0 / (nTicks + 1);
  } else {
    nTicks = 0;			/* No minor ticks. */
    step = 0.5;			/* Don't set the minor tick interval to
				 * 0.0. It makes the GenerateTicks
				 * routine * create minor log-scale tick
				 * marks.  */
  }
  axisPtr->minorSweep.initial = axisPtr->minorSweep.step = step;
  axisPtr->minorSweep.nSteps = nTicks;
}

static void SweepTicks(Axis *axisPtr)
{
  if (axisPtr->flags & AXIS_AUTO_MAJOR) {
    if (axisPtr->t1Ptr)
      free(axisPtr->t1Ptr);

    axisPtr->t1Ptr = GenerateTicks(&axisPtr->majorSweep);
  }
  if (axisPtr->flags & AXIS_AUTO_MINOR) {
    if (axisPtr->t2Ptr)
      free(axisPtr->t2Ptr);

    axisPtr->t2Ptr = GenerateTicks(&axisPtr->minorSweep);
  }
}

void Blt_ResetAxes(Graph* graphPtr)
{
  Blt_ChainLink link;
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;

  /* FIXME: This should be called whenever the display list of
   *	      elements change. Maybe yet another flag INIT_STACKS to
   *	      indicate that the element display list has changed.
   *	      Needs to be done before the axis limits are set.
   */
  Blt_InitBarSetTable(graphPtr);
  if ((graphPtr->barMode == BARS_STACKED) && (graphPtr->nBarGroups > 0))
    Blt_ComputeBarStacks(graphPtr);

  /*
   * Step 1:  Reset all axes. Initialize the data limits of the axis to
   *		impossible values.
   */
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr;

    axisPtr = Tcl_GetHashValue(hPtr);
    axisPtr->min = axisPtr->valueRange.min = DBL_MAX;
    axisPtr->max = axisPtr->valueRange.max = -DBL_MAX;
  }

  /*
   * Step 2:  For each element that's to be displayed, get the smallest
   *		and largest data values mapped to each X and Y-axis.  This
   *		will be the axis limits if the user doesn't override them 
   *		with -min and -max options.
   */
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr;
    Region2d exts;

    elemPtr = Blt_Chain_GetValue(link);
    (*elemPtr->procsPtr->extentsProc) (elemPtr, &exts);
    GetDataLimits(elemPtr->axes.x, exts.left, exts.right);
    GetDataLimits(elemPtr->axes.y, exts.top, exts.bottom);
  }
  /*
   * Step 3:  Now that we know the range of data values for each axis,
   *		set axis limits and compute a sweep to generate tick values.
   */
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr;
    double min, max;

    axisPtr = Tcl_GetHashValue(hPtr);
    FixAxisRange(axisPtr);

    /* Calculate min/max tick (major/minor) layouts */
    min = axisPtr->min;
    max = axisPtr->max;
    if ((!isnan(axisPtr->scrollMin)) && (min < axisPtr->scrollMin)) {
      min = axisPtr->scrollMin;
    }
    if ((!isnan(axisPtr->scrollMax)) && (max > axisPtr->scrollMax)) {
      max = axisPtr->scrollMax;
    }
    if (axisPtr->logScale)
      LogScaleAxis(axisPtr, min, max);
    else
      LinearScaleAxis(axisPtr, min, max);

    if (axisPtr->use && (axisPtr->flags & DIRTY))
      graphPtr->flags |= CACHE_DIRTY;
  }

  graphPtr->flags &= ~RESET_AXES;

  /*
   * When any axis changes, we need to layout the entire graph.
   */
  graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | MAP_ALL | 
		      REDRAW_WORLD);
}

static void ResetTextStyles(Axis *axisPtr)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;

  Blt_Ts_ResetStyle(graphPtr->tkwin, &axisPtr->limitsTextStyle);

  gcMask = (GCForeground | GCLineWidth | GCCapStyle);
  gcValues.foreground = axisPtr->tickColor->pixel;
  gcValues.font = Tk_FontId(axisPtr->tickFont);
  gcValues.line_width = LineWidth(axisPtr->lineWidth);
  gcValues.cap_style = CapProjecting;

  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (axisPtr->tickGC) {
    Tk_FreeGC(graphPtr->display, axisPtr->tickGC);
  }
  axisPtr->tickGC = newGC;

  /* Assuming settings from above GC */
  gcValues.foreground = axisPtr->activeFgColor->pixel;
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (axisPtr->activeTickGC) {
    Tk_FreeGC(graphPtr->display, axisPtr->activeTickGC);
  }
  axisPtr->activeTickGC = newGC;

  gcValues.background = gcValues.foreground = axisPtr->major.color->pixel;
  gcValues.line_width = LineWidth(axisPtr->major.lineWidth);
  gcMask = (GCForeground | GCBackground | GCLineWidth);
  if (LineIsDashed(axisPtr->major.dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(axisPtr->major.dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &axisPtr->major.dashes);
  }
  if (axisPtr->major.gc) {
    Blt_FreePrivateGC(graphPtr->display, axisPtr->major.gc);
  }
  axisPtr->major.gc = newGC;

  gcValues.background = gcValues.foreground = axisPtr->minor.color->pixel;
  gcValues.line_width = LineWidth(axisPtr->minor.lineWidth);
  gcMask = (GCForeground | GCBackground | GCLineWidth);
  if (LineIsDashed(axisPtr->minor.dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(axisPtr->minor.dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &axisPtr->minor.dashes);
  }
  if (axisPtr->minor.gc) {
    Blt_FreePrivateGC(graphPtr->display, axisPtr->minor.gc);
  }
  axisPtr->minor.gc = newGC;
}

static void FreeAxis(char* data)
{
  Axis* axisPtr = (Axis*)data;
  DestroyAxis(axisPtr);
}

static float titleAngle[4] =		/* Rotation for each axis title */
  {
    0.0, 90.0, 0.0, 270.0
  };

static void AxisOffsets(Axis *axisPtr, int margin, int offset,
			AxisInfo *infoPtr)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;
  Margin *marginPtr;
  int pad;				/* Offset of axis from interior
					 * region. This includes a possible
					 * border and the axis line width. */
  int axisLine;
  int t1, t2, labelOffset;
  int tickLabel, axisPad;
  int inset, mark;
  int x, y;
  float fangle;

  axisPtr->titleAngle = titleAngle[margin];
  marginPtr = graphPtr->margins + margin;

  tickLabel = axisLine = t1 = t2 = 0;
  labelOffset = AXIS_PAD_TITLE;
  if (axisPtr->lineWidth > 0) {
    if (axisPtr->showTicks) {
      t1 = axisPtr->tickLength;
      t2 = (t1 * 10) / 15;
    }
    labelOffset = t1 + AXIS_PAD_TITLE;
    if (axisPtr->exterior)
      labelOffset += axisPtr->lineWidth;
  }
  axisPad = 0;
  if (graphPtr->plotRelief != TK_RELIEF_SOLID) {
    axisPad = 0;
  }
  /* Adjust offset for the interior border width and the line width */
  pad = 1;
  if (graphPtr->plotBW > 0) {
    pad += graphPtr->plotBW + 1;
  }
  pad = 0;				/* FIXME: test */
  /*
   * Pre-calculate the x-coordinate positions of the axis, tick labels, and
   * the individual major and minor ticks.
   */
  inset = pad + axisPtr->lineWidth / 2;
  switch (margin) {
  case MARGIN_TOP:
    axisLine = graphPtr->top;
    if (axisPtr->exterior) {
      axisLine -= graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
      tickLabel = axisLine - 2;
      if (axisPtr->lineWidth > 0) {
	tickLabel -= axisPtr->tickLength;
      }
    } else {
      if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
	axisLine--;
      } 
      axisLine -= axisPad + axisPtr->lineWidth / 2;
      tickLabel = graphPtr->top -  graphPtr->plotBW - 2;
    }
    mark = graphPtr->top - offset - pad;
    axisPtr->tickAnchor = TK_ANCHOR_S;
    axisPtr->left = axisPtr->screenMin - inset - 2;
    axisPtr->right = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
    if (graphPtr->stackAxes) {
      axisPtr->top = mark - marginPtr->axesOffset;
    } else {
      axisPtr->top = mark - axisPtr->height;
    }
    axisPtr->bottom = mark;
    if (axisPtr->titleAlternate) {
      x = graphPtr->right + AXIS_PAD_TITLE;
      y = mark - (axisPtr->height  / 2);
      axisPtr->titleAnchor = TK_ANCHOR_W;
    } else {
      x = (axisPtr->right + axisPtr->left) / 2;
      if (graphPtr->stackAxes) {
	y = mark - marginPtr->axesOffset + AXIS_PAD_TITLE;
      } else {
	y = mark - axisPtr->height + AXIS_PAD_TITLE;
      }
      axisPtr->titleAnchor = TK_ANCHOR_N;
    }
    axisPtr->titlePos.x = x;
    axisPtr->titlePos.y = y;
    break;

  case MARGIN_BOTTOM:
    /*
     *  ----------- bottom + plot borderwidth
     *      mark --------------------------------------------
     *          ===================== axisLine (linewidth)
     *                   tick
     *		    title
     *
     *          ===================== axisLine (linewidth)
     *  ----------- bottom + plot borderwidth
     *      mark --------------------------------------------
     *                   tick
     *		    title
     */
    axisLine = graphPtr->bottom;
    if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
      axisLine++;
    } 
    if (axisPtr->exterior) {
      axisLine += graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
      tickLabel = axisLine + 2;
      if (axisPtr->lineWidth > 0) {
	tickLabel += axisPtr->tickLength;
      }
    } else {
      axisLine -= axisPad + axisPtr->lineWidth / 2;
      tickLabel = graphPtr->bottom +  graphPtr->plotBW + 2;
    }
    mark = graphPtr->bottom + offset;
    fangle = fmod(axisPtr->tickAngle, 90.0);
    if (fangle == 0.0) {
      axisPtr->tickAnchor = TK_ANCHOR_N;
    } else {
      int quadrant;

      quadrant = (int)(axisPtr->tickAngle / 90.0);
      if ((quadrant == 0) || (quadrant == 2)) {
	axisPtr->tickAnchor = TK_ANCHOR_NE;
      } else {
	axisPtr->tickAnchor = TK_ANCHOR_NW;
      }
    }
    axisPtr->left = axisPtr->screenMin - inset - 2;
    axisPtr->right = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
    axisPtr->top = graphPtr->bottom + labelOffset - t1;
    if (graphPtr->stackAxes) {
      axisPtr->bottom = mark + marginPtr->axesOffset - 1;
    } else {
      axisPtr->bottom = mark + axisPtr->height - 1;
    }
    if (axisPtr->titleAlternate) {
      x = graphPtr->right + AXIS_PAD_TITLE;
      y = mark + (axisPtr->height / 2);
      axisPtr->titleAnchor = TK_ANCHOR_W; 
    } else {
      x = (axisPtr->right + axisPtr->left) / 2;
      if (graphPtr->stackAxes) {
	y = mark + marginPtr->axesOffset - AXIS_PAD_TITLE;
      } else {
	y = mark + axisPtr->height - AXIS_PAD_TITLE;
      }
      axisPtr->titleAnchor = TK_ANCHOR_S; 
    }
    axisPtr->titlePos.x = x;
    axisPtr->titlePos.y = y;
    break;

  case MARGIN_LEFT:
    /*
     *                    mark
     *                  |  : 
     *                  |  :      
     *                  |  : 
     *                  |  :
     *                  |  : 
     *     axisLine
     */
    /* 
     * Exterior axis 
     *     + plotarea right
     *     |A|B|C|D|E|F|G|H
     *           |right
     * A = plot pad 
     * B = plot border width
     * C = axis pad
     * D = axis line
     * E = tick length
     * F = tick label 
     * G = graph border width
     * H = highlight thickness
     */
    /* 
     * Interior axis 
     *     + plotarea right
     *     |A|B|C|D|E|F|G|H
     *           |right
     * A = plot pad 
     * B = tick length
     * C = axis line width
     * D = axis pad
     * E = plot border width
     * F = tick label 
     * G = graph border width
     * H = highlight thickness
     */
    axisLine = graphPtr->left;
    if (axisPtr->exterior) {
      axisLine -= graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
      tickLabel = axisLine - 2;
      if (axisPtr->lineWidth > 0) {
	tickLabel -= axisPtr->tickLength;
      }
    } else {
      if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
	axisLine--;
      } 
      axisLine += axisPad + axisPtr->lineWidth / 2;
      tickLabel = graphPtr->left - graphPtr->plotBW - 2;
    }
    mark = graphPtr->left - offset;
    axisPtr->tickAnchor = TK_ANCHOR_E;
    if (graphPtr->stackAxes) {
      axisPtr->left = mark - marginPtr->axesOffset;
    } else {
      axisPtr->left = mark - axisPtr->width;
    }
    axisPtr->right = mark - 3;
    axisPtr->top = axisPtr->screenMin - inset - 2;
    axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
    if (axisPtr->titleAlternate) {
      x = mark - (axisPtr->width / 2);
      y = graphPtr->top - AXIS_PAD_TITLE;
      axisPtr->titleAnchor = TK_ANCHOR_SW; 
    } else {
      if (graphPtr->stackAxes) {
	x = mark - marginPtr->axesOffset;
      } else {
	x = mark - axisPtr->width + AXIS_PAD_TITLE;
      }
      y = (axisPtr->bottom + axisPtr->top) / 2;
      axisPtr->titleAnchor = TK_ANCHOR_W; 
    } 
    axisPtr->titlePos.x = x;
    axisPtr->titlePos.y = y;
    break;

  case MARGIN_RIGHT:
    axisLine = graphPtr->right;
    if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
      axisLine++;			/* Draw axis line within solid plot
					 * border. */
    } 
    if (axisPtr->exterior) {
      axisLine += graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
      tickLabel = axisLine + 2;
      if (axisPtr->lineWidth > 0) {
	tickLabel += axisPtr->tickLength;
      }
    } else {
      axisLine -= axisPad + axisPtr->lineWidth / 2;
      tickLabel = graphPtr->right + graphPtr->plotBW + 2;
    }
    mark = graphPtr->right + offset + pad;
    axisPtr->tickAnchor = TK_ANCHOR_W;
    axisPtr->left = mark;
    if (graphPtr->stackAxes) {
      axisPtr->right = mark + marginPtr->axesOffset - 1;
    } else {
      axisPtr->right = mark + axisPtr->width - 1;
    }
    axisPtr->top = axisPtr->screenMin - inset - 2;
    axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset -1;
    if (axisPtr->titleAlternate) {
      x = mark + (axisPtr->width / 2);
      y = graphPtr->top - AXIS_PAD_TITLE;
      axisPtr->titleAnchor = TK_ANCHOR_SE; 
    } else {
      if (graphPtr->stackAxes) {
	x = mark + marginPtr->axesOffset - AXIS_PAD_TITLE;
      } else {
	x = mark + axisPtr->width - AXIS_PAD_TITLE;
      }
      y = (axisPtr->bottom + axisPtr->top) / 2;
      axisPtr->titleAnchor = TK_ANCHOR_E;
    }
    axisPtr->titlePos.x = x;
    axisPtr->titlePos.y = y;
    break;

  case MARGIN_NONE:
    axisLine = 0;
    break;
  }
  if ((margin == MARGIN_LEFT) || (margin == MARGIN_TOP)) {
    t1 = -t1, t2 = -t2;
    labelOffset = -labelOffset;
  }
  infoPtr->axis = axisLine;
  infoPtr->t1 = axisLine + t1;
  infoPtr->t2 = axisLine + t2;
  if (tickLabel > 0) {
    infoPtr->label = tickLabel;
  } else {
    infoPtr->label = axisLine + labelOffset;
  }
  if (!axisPtr->exterior) {
    /*infoPtr->label = axisLine + labelOffset - t1; */
    infoPtr->t1 = axisLine - t1;
    infoPtr->t2 = axisLine - t2;
  } 
}

static void MakeAxisLine(Axis *axisPtr, int line, Segment2d *sp)
{
  double min, max;

  min = axisPtr->axisRange.min;
  max = axisPtr->axisRange.max;
  if (axisPtr->logScale) {
    min = EXP10(min);
    max = EXP10(max);
  }
  if (AxisIsHorizontal(axisPtr)) {
    sp->p.x = Blt_HMap(axisPtr, min);
    sp->q.x = Blt_HMap(axisPtr, max);
    sp->p.y = sp->q.y = line;
  } else {
    sp->q.x = sp->p.x = line;
    sp->p.y = Blt_VMap(axisPtr, min);
    sp->q.y = Blt_VMap(axisPtr, max);
  }
}

static void MakeTick(Axis *axisPtr, double value, int tick, int line, 
		     Segment2d *sp)
{
  if (axisPtr->logScale)
    value = EXP10(value);

  if (AxisIsHorizontal(axisPtr)) {
    sp->p.x = sp->q.x = Blt_HMap(axisPtr, value);
    sp->p.y = line;
    sp->q.y = tick;
  }
  else {
    sp->p.x = line;
    sp->p.y = sp->q.y = Blt_VMap(axisPtr, value);
    sp->q.x = tick;
  }
}

static void MakeSegments(Axis *axisPtr, AxisInfo *infoPtr)
{
  int arraySize;
  int nMajorTicks, nMinorTicks;
  Segment2d *segments;
  Segment2d *sp;

  if (axisPtr->segments) {
    free(axisPtr->segments);
    axisPtr->segments = NULL;	
  }
  nMajorTicks = nMinorTicks = 0;
  if (axisPtr->t1Ptr)
    nMajorTicks = axisPtr->t1Ptr->nTicks;

  if (axisPtr->t2Ptr)
    nMinorTicks = axisPtr->t2Ptr->nTicks;

  arraySize = 1 + (nMajorTicks * (nMinorTicks + 1));
  segments = malloc(arraySize * sizeof(Segment2d));
  sp = segments;
  if (axisPtr->lineWidth > 0) {
    /* Axis baseline */
    MakeAxisLine(axisPtr, infoPtr->axis, sp);
    sp++;
  }
  if (axisPtr->showTicks) {
    Blt_ChainLink link;
    double labelPos;
    int i;
    int isHoriz;

    isHoriz = AxisIsHorizontal(axisPtr);
    for (i = 0; i < nMajorTicks; i++) {
      double t1, t2;
      int j;

      t1 = axisPtr->t1Ptr->values[i];
      /* Minor ticks */
      for (j = 0; j < nMinorTicks; j++) {
	t2 = t1 + (axisPtr->majorSweep.step * 
		   axisPtr->t2Ptr->values[j]);
	if (InRange(t2, &axisPtr->axisRange)) {
	  MakeTick(axisPtr, t2, infoPtr->t2, infoPtr->axis, sp);
	  sp++;
	}
      }
      if (!InRange(t1, &axisPtr->axisRange)) {
	continue;
      }
      /* Major tick */
      MakeTick(axisPtr, t1, infoPtr->t1, infoPtr->axis, sp);
      sp++;
    }

    link = Blt_Chain_FirstLink(axisPtr->tickLabels);
    labelPos = (double)infoPtr->label;

    for (i = 0; i < nMajorTicks; i++) {
      double t1;
      TickLabel *labelPtr;
      Segment2d seg;

      t1 = axisPtr->t1Ptr->values[i];
      if (axisPtr->labelOffset) {
	t1 += axisPtr->majorSweep.step * 0.5;
      }
      if (!InRange(t1, &axisPtr->axisRange)) {
	continue;
      }
      labelPtr = Blt_Chain_GetValue(link);
      link = Blt_Chain_NextLink(link);
      MakeTick(axisPtr, t1, infoPtr->t1, infoPtr->axis, &seg);
      /* Save tick label X-Y position. */
      if (isHoriz) {
	labelPtr->anchorPos.x = seg.p.x;
	labelPtr->anchorPos.y = labelPos;
      } else {
	labelPtr->anchorPos.x = labelPos;
	labelPtr->anchorPos.y = seg.p.y;
      }
    }
  }
  axisPtr->segments = segments;
  axisPtr->nSegments = sp - segments;
}

static void MapAxis(Axis *axisPtr, int offset, int margin)
{
  AxisInfo info;
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (AxisIsHorizontal(axisPtr)) {
    axisPtr->screenMin = graphPtr->hOffset;
    axisPtr->width = graphPtr->right - graphPtr->left;
    axisPtr->screenRange = graphPtr->hRange;
  } else {
    axisPtr->screenMin = graphPtr->vOffset;
    axisPtr->height = graphPtr->bottom - graphPtr->top;
    axisPtr->screenRange = graphPtr->vRange;
  }
  axisPtr->screenScale = 1.0 / axisPtr->screenRange;
  AxisOffsets(axisPtr, margin, offset, &info);
  MakeSegments(axisPtr, &info);
}

static void MapStackedAxis(Axis *axisPtr, int count, int margin)
{
  AxisInfo info;
  Graph* graphPtr = axisPtr->obj.graphPtr;
  unsigned int slice, w, h;

  if ((graphPtr->margins[axisPtr->margin].axes->nLinks > 1) ||
      (axisPtr->reqNumMajorTicks <= 0)) {
    axisPtr->reqNumMajorTicks = 4;
  }
  if (AxisIsHorizontal(axisPtr)) {
    slice = graphPtr->hRange / graphPtr->margins[margin].axes->nLinks;
    axisPtr->screenMin = graphPtr->hOffset;
    axisPtr->width = slice;
  } else {
    slice = graphPtr->vRange / graphPtr->margins[margin].axes->nLinks;
    axisPtr->screenMin = graphPtr->vOffset;
    axisPtr->height = slice;
  }
#define AXIS_PAD 2
  Blt_GetTextExtents(axisPtr->tickFont, 0, "0", 1, &w, &h);
  axisPtr->screenMin += (slice * count) + AXIS_PAD + h / 2;
  axisPtr->screenRange = slice - 2 * AXIS_PAD - h;
  axisPtr->screenScale = 1.0f / axisPtr->screenRange;
  AxisOffsets(axisPtr, margin, 0, &info);
  MakeSegments(axisPtr, &info);
}

static double AdjustViewport(double offset, double windowSize)
{
  /*
   * Canvas-style scrolling allows the world to be scrolled within the window.
   */
  if (windowSize > 1.0) {
    if (windowSize < (1.0 - offset)) {
      offset = 1.0 - windowSize;
    }
    if (offset > 0.0) {
      offset = 0.0;
    }
  } else {
    if ((offset + windowSize) > 1.0) {
      offset = 1.0 - windowSize;
    }
    if (offset < 0.0) {
      offset = 0.0;
    }
  }
  return offset;
}

static int GetAxisScrollInfo(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[],
			     double *offsetPtr, double windowSize,
			     double scrollUnits, double scale)
{
  const char *string;
  char c;
  double offset;
  int length;

  offset = *offsetPtr;
  string = Tcl_GetStringFromObj(objv[0], &length);
  c = string[0];
  scrollUnits *= scale;
  if ((c == 's') && (strncmp(string, "scroll", length) == 0)) {
    int count;
    double fract;

    /* Scroll number unit/page */
    if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK) {
      return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    if ((c == 'u') && (strncmp(string, "units", length) == 0)) {
      fract = count * scrollUnits;
    } else if ((c == 'p') && (strncmp(string, "pages", length) == 0)) {
      /* A page is 90% of the view-able window. */
      fract = (int)(count * windowSize * 0.9 + 0.5);
    } else if ((c == 'p') && (strncmp(string, "pixels", length) == 0)) {
      fract = count * scale;
    } else {
      Tcl_AppendResult(interp, "unknown \"scroll\" units \"", string,
		       "\"", NULL);
      return TCL_ERROR;
    }
    offset += fract;
  } else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
    double fract;

    /* moveto fraction */
    if (Tcl_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
      return TCL_ERROR;
    }
    offset = fract;
  } else {
    int count;
    double fract;

    /* Treat like "scroll units" */
    if (Tcl_GetIntFromObj(interp, objv[0], &count) != TCL_OK) {
      return TCL_ERROR;
    }
    fract = (double)count * scrollUnits;
    offset += fract;
    /* CHECK THIS: return TCL_OK; */
  }
  *offsetPtr = AdjustViewport(offset, windowSize);
  return TCL_OK;
}

static void DrawAxis(Axis *axisPtr, Drawable drawable)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (axisPtr->normalBg) {
    Tk_Fill3DRectangle(graphPtr->tkwin, drawable, 
		       axisPtr->normalBg, 
		       axisPtr->left, axisPtr->top, 
		       axisPtr->right - axisPtr->left, 
		       axisPtr->bottom - axisPtr->top,
		       axisPtr->borderWidth, 
		       axisPtr->relief);
  }
  if (axisPtr->title) {
    TextStyle ts;

    Blt_Ts_InitStyle(ts);
    Blt_Ts_SetAngle(ts, axisPtr->titleAngle);
    Blt_Ts_SetFont(ts, axisPtr->titleFont);
    Blt_Ts_SetPadding(ts, 1, 0);
    Blt_Ts_SetAnchor(ts, axisPtr->titleAnchor);
    Blt_Ts_SetJustify(ts, axisPtr->titleJustify);
    if (axisPtr->flags & ACTIVE)
      Blt_Ts_SetForeground(ts, axisPtr->activeFgColor);
    else
      Blt_Ts_SetForeground(ts, axisPtr->titleColor);

    if ((axisPtr->titleAngle == 90.0) || (axisPtr->titleAngle == 270.0))
      Blt_Ts_SetMaxLength(ts, axisPtr->height);
    else
      Blt_Ts_SetMaxLength(ts, axisPtr->width);

    Blt_Ts_DrawText(graphPtr->tkwin, drawable, axisPtr->title, -1, &ts, 
		    (int)axisPtr->titlePos.x, (int)axisPtr->titlePos.y);
  }
  if (axisPtr->scrollCmdObjPtr) {
    double viewWidth, viewMin, viewMax;
    double worldWidth, worldMin, worldMax;
    double fract;
    int isHoriz;

    worldMin = axisPtr->valueRange.min;
    worldMax = axisPtr->valueRange.max;
    if (!isnan(axisPtr->scrollMin)) {
      worldMin = axisPtr->scrollMin;
    }
    if (!isnan(axisPtr->scrollMax)) {
      worldMax = axisPtr->scrollMax;
    }
    viewMin = axisPtr->min;
    viewMax = axisPtr->max;
    if (viewMin < worldMin) {
      viewMin = worldMin;
    }
    if (viewMax > worldMax) {
      viewMax = worldMax;
    }
    if (axisPtr->logScale) {
      worldMin = log10(worldMin);
      worldMax = log10(worldMax);
      viewMin = log10(viewMin);
      viewMax = log10(viewMax);
    }
    worldWidth = worldMax - worldMin;	
    viewWidth = viewMax - viewMin;
    isHoriz = AxisIsHorizontal(axisPtr);

    if (isHoriz != axisPtr->descending) {
      fract = (viewMin - worldMin) / worldWidth;
    } else {
      fract = (worldMax - viewMax) / worldWidth;
    }
    fract = AdjustViewport(fract, viewWidth / worldWidth);

    if (isHoriz != axisPtr->descending) {
      viewMin = (fract * worldWidth);
      axisPtr->min = viewMin + worldMin;
      axisPtr->max = axisPtr->min + viewWidth;
      viewMax = viewMin + viewWidth;
      if (axisPtr->logScale) {
	axisPtr->min = EXP10(axisPtr->min);
	axisPtr->max = EXP10(axisPtr->max);
      }
      Blt_UpdateScrollbar(graphPtr->interp, axisPtr->scrollCmdObjPtr,
			  viewMin, viewMax, worldWidth);
    } else {
      viewMax = (fract * worldWidth);
      axisPtr->max = worldMax - viewMax;
      axisPtr->min = axisPtr->max - viewWidth;
      viewMin = viewMax + viewWidth;
      if (axisPtr->logScale) {
	axisPtr->min = EXP10(axisPtr->min);
	axisPtr->max = EXP10(axisPtr->max);
      }
      Blt_UpdateScrollbar(graphPtr->interp, axisPtr->scrollCmdObjPtr,
			  viewMax, viewMin, worldWidth);
    }
  }
  if (axisPtr->showTicks) {
    Blt_ChainLink link;
    TextStyle ts;

    Blt_Ts_InitStyle(ts);
    Blt_Ts_SetAngle(ts, axisPtr->tickAngle);
    Blt_Ts_SetFont(ts, axisPtr->tickFont);
    Blt_Ts_SetPadding(ts, 2, 0);
    Blt_Ts_SetAnchor(ts, axisPtr->tickAnchor);
    if (axisPtr->flags & ACTIVE)
      Blt_Ts_SetForeground(ts, axisPtr->activeFgColor);
    else
      Blt_Ts_SetForeground(ts, axisPtr->tickColor);

    for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL;
	 link = Blt_Chain_NextLink(link)) {	
      TickLabel *labelPtr;

      labelPtr = Blt_Chain_GetValue(link);
      /* Draw major tick labels */
      Blt_DrawText(graphPtr->tkwin, drawable, labelPtr->string, &ts, 
		   (int)labelPtr->anchorPos.x, (int)labelPtr->anchorPos.y);
    }
  }
  if ((axisPtr->nSegments > 0) && (axisPtr->lineWidth > 0)) {	
    GC gc = (axisPtr->flags & ACTIVE) ? axisPtr->activeTickGC : axisPtr->tickGC;

    // Draw the tick marks and axis line.
    Blt_Draw2DSegments(graphPtr->display, drawable, gc, axisPtr->segments, 
		       axisPtr->nSegments);
  }
}

static void AxisToPostScript(Blt_Ps ps, Axis *axisPtr)
{
  Blt_Ps_Format(ps, "%% Axis \"%s\"\n", axisPtr->obj.name);
  if (axisPtr->normalBg)
    Blt_Ps_Fill3DRectangle(ps, axisPtr->normalBg, 
			   (double)axisPtr->left, (double)axisPtr->top, 
			   axisPtr->right - axisPtr->left, 
			   axisPtr->bottom - axisPtr->top, 
			   axisPtr->borderWidth, axisPtr->relief);

  if (axisPtr->title) {
    TextStyle ts;

    Blt_Ts_InitStyle(ts);
    Blt_Ts_SetAngle(ts, axisPtr->titleAngle);
    Blt_Ts_SetFont(ts, axisPtr->titleFont);
    Blt_Ts_SetPadding(ts, 1, 0);
    Blt_Ts_SetAnchor(ts, axisPtr->titleAnchor);
    Blt_Ts_SetJustify(ts, axisPtr->titleJustify);
    Blt_Ts_SetForeground(ts, axisPtr->titleColor);
    Blt_Ps_DrawText(ps, axisPtr->title, &ts, axisPtr->titlePos.x, 
		    axisPtr->titlePos.y);
  }
  if (axisPtr->showTicks) {
    Blt_ChainLink link;
    TextStyle ts;

    Blt_Ts_InitStyle(ts);
    Blt_Ts_SetAngle(ts, axisPtr->tickAngle);
    Blt_Ts_SetFont(ts, axisPtr->tickFont);
    Blt_Ts_SetPadding(ts, 2, 0);
    Blt_Ts_SetAnchor(ts, axisPtr->tickAnchor);
    Blt_Ts_SetForeground(ts, axisPtr->tickColor);

    for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
      TickLabel *labelPtr;

      labelPtr = Blt_Chain_GetValue(link);
      Blt_Ps_DrawText(ps, labelPtr->string, &ts, labelPtr->anchorPos.x, 
		      labelPtr->anchorPos.y);
    }
  }
  if ((axisPtr->nSegments > 0) && (axisPtr->lineWidth > 0)) {
    Blt_Ps_XSetLineAttributes(ps, axisPtr->tickColor, axisPtr->lineWidth, 
			      (Blt_Dashes *)NULL, CapButt, JoinMiter);
    Blt_Ps_Draw2DSegments(ps, axisPtr->segments, axisPtr->nSegments);
  }
}

static void MakeGridLine(Axis *axisPtr, double value, Segment2d *sp)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;

  if (axisPtr->logScale) {
    value = EXP10(value);
  }
  /* Grid lines run orthogonally to the axis */
  if (AxisIsHorizontal(axisPtr)) {
    sp->p.y = graphPtr->top;
    sp->q.y = graphPtr->bottom;
    sp->p.x = sp->q.x = Blt_HMap(axisPtr, value);
  } else {
    sp->p.x = graphPtr->left;
    sp->q.x = graphPtr->right;
    sp->p.y = sp->q.y = Blt_VMap(axisPtr, value);
  }
}

static void MapGridlines(Axis *axisPtr)
{
  Segment2d *s1, *s2;
  Ticks *t1Ptr, *t2Ptr;
  int needed;
  int i;

  if (!axisPtr)
    return;

  t1Ptr = axisPtr->t1Ptr;
  if (!t1Ptr)
    t1Ptr = GenerateTicks(&axisPtr->majorSweep);

  t2Ptr = axisPtr->t2Ptr;
  if (!t2Ptr)
    t2Ptr = GenerateTicks(&axisPtr->minorSweep);

  needed = t1Ptr->nTicks;
  if (axisPtr->showGridMinor)
    needed += (t1Ptr->nTicks * t2Ptr->nTicks);

  if (needed == 0)
    return;			

  needed = t1Ptr->nTicks;
  if (needed != axisPtr->major.nAllocated) {
    if (axisPtr->major.segments) {
      free(axisPtr->major.segments);
      axisPtr->major.segments = NULL;
    }
    axisPtr->major.segments = malloc(sizeof(Segment2d) * needed);
    axisPtr->major.nAllocated = needed;
  }
  needed = (t1Ptr->nTicks * t2Ptr->nTicks);
  if (needed != axisPtr->minor.nAllocated) {
    if (axisPtr->minor.segments) {
      free(axisPtr->minor.segments);
      axisPtr->minor.segments = NULL;
    }
    axisPtr->minor.segments = malloc(sizeof(Segment2d) * needed);
    axisPtr->minor.nAllocated = needed;
  }
  s1 = axisPtr->major.segments, s2 = axisPtr->minor.segments;
  for (i = 0; i < t1Ptr->nTicks; i++) {
    double value;

    value = t1Ptr->values[i];
    if (axisPtr->showGridMinor) {
      int j;

      for (j = 0; j < t2Ptr->nTicks; j++) {
	double subValue;

	subValue = value + (axisPtr->majorSweep.step * 
			    t2Ptr->values[j]);
	if (InRange(subValue, &axisPtr->axisRange)) {
	  MakeGridLine(axisPtr, subValue, s2);
	  s2++;
	}
      }
    }
    if (InRange(value, &axisPtr->axisRange)) {
      MakeGridLine(axisPtr, value, s1);
      s1++;
    }
  }
  if (t1Ptr != axisPtr->t1Ptr) {
    free(t1Ptr);		/* Free generated ticks. */
  }
  if (t2Ptr != axisPtr->t2Ptr) {
    free(t2Ptr);		/* Free generated ticks. */
  }
  axisPtr->major.nUsed = s1 - axisPtr->major.segments;
  axisPtr->minor.nUsed = s2 - axisPtr->minor.segments;
}

static void GetAxisGeometry(Graph* graphPtr, Axis *axisPtr)
{
  unsigned int y;

  FreeTickLabels(axisPtr->tickLabels);
  y = 0;

  // Leave room for axis baseline and padding
  if (axisPtr->exterior && (graphPtr->plotRelief != TK_RELIEF_SOLID))
    y += axisPtr->lineWidth + 2;

  axisPtr->maxTickHeight = axisPtr->maxTickWidth = 0;
  if (axisPtr->showTicks) {
    unsigned int pad;
    unsigned int i, nLabels, nTicks;

    SweepTicks(axisPtr);
	
    nTicks = 0;
    if (axisPtr->t1Ptr) {
      nTicks = axisPtr->t1Ptr->nTicks;
    }
	
    nLabels = 0;
    for (i = 0; i < nTicks; i++) {
      TickLabel *labelPtr;
      double x, x2;
      unsigned int lw, lh;	/* Label width and height. */

      x2 = x = axisPtr->t1Ptr->values[i];
      if (axisPtr->labelOffset) {
	x2 += axisPtr->majorSweep.step * 0.5;
      }
      if (!InRange(x2, &axisPtr->axisRange)) {
	continue;
      }
      labelPtr = MakeLabel(axisPtr, x);
      Blt_Chain_Append(axisPtr->tickLabels, labelPtr);
      nLabels++;
      /* 
       * Get the dimensions of each tick label.  Remember tick labels
       * can be multi-lined and/or rotated.
       */
      Blt_GetTextExtents(axisPtr->tickFont, 0, labelPtr->string, -1, 
			 &lw, &lh);
      labelPtr->width  = lw;
      labelPtr->height = lh;

      if (axisPtr->tickAngle != 0.0f) {
	double rlw, rlh;	/* Rotated label width and height. */
	Blt_GetBoundingBox(lw, lh, axisPtr->tickAngle, &rlw, &rlh,NULL);
	lw = ROUND(rlw), lh = ROUND(rlh);
      }
      if (axisPtr->maxTickWidth < lw) {
	axisPtr->maxTickWidth = lw;
      }
      if (axisPtr->maxTickHeight < lh) {
	axisPtr->maxTickHeight = lh;
      }
    }
	
    pad = 0;
    if (axisPtr->exterior) {
      /* Because the axis cap style is "CapProjecting", we need to
       * account for an extra 1.5 linewidth at the end of each line.  */
      pad = ((axisPtr->lineWidth * 12) / 8);
    }
    if (AxisIsHorizontal(axisPtr)) {
      y += axisPtr->maxTickHeight + pad;
    } else {
      y += axisPtr->maxTickWidth + pad;
      if (axisPtr->maxTickWidth > 0) {
	y += 5;			/* Pad either size of label. */
      }  
    }
    y += 2 * AXIS_PAD_TITLE;
    if ((axisPtr->lineWidth > 0) && axisPtr->exterior) {
      /* Distance from axis line to tick label. */
      y += axisPtr->tickLength;
    }
  }

  if (axisPtr->title) {
    if (axisPtr->titleAlternate) {
      if (y < axisPtr->titleHeight) {
	y = axisPtr->titleHeight;
      }
    } else {
      y += axisPtr->titleHeight + AXIS_PAD_TITLE;
    }
  }

  /* Correct for orientation of the axis. */
  if (AxisIsHorizontal(axisPtr)) {
    axisPtr->height = y;
  } else {
    axisPtr->width = y;
  }
}

static int GetMarginGeometry(Graph* graphPtr, Margin *marginPtr)
{
  Blt_ChainLink link;
  unsigned int l, w, h;		/* Length, width, and height. */
  int isHoriz;
  unsigned int nVisible;

  isHoriz = HORIZMARGIN(marginPtr);

  /* Count the visible axes. */
  nVisible = 0;
  l = w = h = 0;
  marginPtr->maxTickWidth = marginPtr->maxTickHeight = 0;
  if (graphPtr->stackAxes) {
    for (link = Blt_Chain_FirstLink(marginPtr->axes); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr;
	    
      axisPtr = Blt_Chain_GetValue(link);
      if (!axisPtr->hide && axisPtr->use) {
	nVisible++;
	if (graphPtr->flags & GET_AXIS_GEOMETRY) {
	  GetAxisGeometry(graphPtr, axisPtr);
	}
	if (isHoriz) {
	  if (h < axisPtr->height) {
	    h = axisPtr->height;
	  }
	} else {
	  if (w < axisPtr->width) {
	    w = axisPtr->width;
	  }
	}
	if (axisPtr->maxTickWidth > marginPtr->maxTickWidth) {
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth;
	}
	if (axisPtr->maxTickHeight > marginPtr->maxTickHeight) {
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight;
	}
      }
    }
  } else {
    for (link = Blt_Chain_FirstLink(marginPtr->axes); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr;
	    
      axisPtr = Blt_Chain_GetValue(link);
      if (!axisPtr->hide && axisPtr->use) {
	nVisible++;
	if (graphPtr->flags & GET_AXIS_GEOMETRY) {
	  GetAxisGeometry(graphPtr, axisPtr);
	}
	if ((axisPtr->titleAlternate) && (l < axisPtr->titleWidth)) {
	  l = axisPtr->titleWidth;
	}
	if (isHoriz) {
	  h += axisPtr->height;
	} else {
	  w += axisPtr->width;
	}
	if (axisPtr->maxTickWidth > marginPtr->maxTickWidth) {
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth;
	}
	if (axisPtr->maxTickHeight > marginPtr->maxTickHeight) {
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight;
	}
      }
    }
  }
  /* Enforce a minimum size for margins. */
  if (w < 3) {
    w = 3;
  }
  if (h < 3) {
    h = 3;
  }
  marginPtr->nAxes = nVisible;
  marginPtr->axesTitleLength = l;
  marginPtr->width = w;
  marginPtr->height = h;
  marginPtr->axesOffset = (isHoriz) ? h : w;
  return marginPtr->axesOffset;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LayoutGraph --
 *
 *	Calculate the layout of the graph.  Based upon the data, axis limits,
 *	X and Y titles, and title height, determine the cavity left which is
 *	the plotting surface.  The first step get the data and axis limits for
 *	calculating the space needed for the top, bottom, left, and right
 *	margins.
 *
 * 	1) The LEFT margin is the area from the left border to the Y axis 
 *	   (not including ticks). It composes the border width, the width an 
 *	   optional Y axis label and its padding, and the tick numeric labels. 
 *	   The Y axis label is rotated 90 degrees so that the width is the 
 *	   font height.
 *
 * 	2) The RIGHT margin is the area from the end of the graph
 *	   to the right window border. It composes the border width,
 *	   some padding, the font height (this may be dubious. It
 *	   appears to provide a more even border), the max of the
 *	   legend width and 1/2 max X tick number. This last part is
 *	   so that the last tick label is not clipped.
 *
 *           Window Width
 *      ___________________________________________________________
 *      |          |                               |               |
 *      |          |   TOP  height of title        |               |
 *      |          |                               |               |
 *      |          |           x2 title            |               |
 *      |          |                               |               |
 *      |          |        height of x2-axis      |               |
 *      |__________|_______________________________|_______________|  W
 *      |          | -plotpady                     |               |  i
 *      |__________|_______________________________|_______________|  n
 *      |          | top                   right   |               |  d
 *      |          |                               |               |  o
 *      |   LEFT   |                               |     RIGHT     |  w
 *      |          |                               |               |
 *      | y        |     Free area = 104%          |      y2       |  H
 *      |          |     Plotting surface = 100%   |               |  e
 *      | t        |     Tick length = 2 + 2%      |      t        |  i
 *      | i        |                               |      i        |  g
 *      | t        |                               |      t  legend|  h
 *      | l        |                               |      l   width|  t
 *      | e        |                               |      e        |
 *      |    height|                               |height         |
 *      |       of |                               | of            |
 *      |    y-axis|                               |y2-axis        |
 *      |          |                               |               |
 *      |          |origin 0,0                     |               |
 *      |__________|_left_________________bottom___|_______________|
 *      |          |-plotpady                      |               |
 *      |__________|_______________________________|_______________|
 *      |          | (xoffset, yoffset)            |               |
 *      |          |                               |               |
 *      |          |       height of x-axis        |               |
 *      |          |                               |               |
 *      |          |   BOTTOM   x title            |               |
 *      |__________|_______________________________|_______________|
 *
 * 3) The TOP margin is the area from the top window border to the top
 *    of the graph. It composes the border width, twice the height of
 *    the title font (if one is given) and some padding between the
 *    title.
 *
 * 4) The BOTTOM margin is area from the bottom window border to the
 *    X axis (not including ticks). It composes the border width, the height
 *    an optional X axis label and its padding, the height of the font
 *    of the tick labels.
 *
 * The plotting area is between the margins which includes the X and Y axes
 * including the ticks but not the tick numeric labels. The length of the
 * ticks and its padding is 5% of the entire plotting area.  Hence the entire
 * plotting area is scaled as 105% of the width and height of the area.
 *
 * The axis labels, ticks labels, title, and legend may or may not be
 * displayed which must be taken into account.
 *
 * if reqWidth > 0 : set outer size
 * if reqPlotWidth > 0 : set plot size
 *---------------------------------------------------------------------------
 */

void Blt_LayoutGraph(Graph* graphPtr)
{
  unsigned int titleY;
  unsigned int left, right, top, bottom;
  unsigned int plotWidth, plotHeight;
  unsigned int inset, inset2;
  int width, height;
  int pad;

  width = graphPtr->width;
  height = graphPtr->height;

  /* 
   * Step 1:  Compute the amount of space needed to display the axes
   *		associated with each margin.  They can be overridden by 
   *		-leftmargin, -rightmargin, -bottommargin, and -topmargin
   *		graph options, respectively.
   */
  left   = GetMarginGeometry(graphPtr, &graphPtr->leftMargin);
  right  = GetMarginGeometry(graphPtr, &graphPtr->rightMargin);
  top    = GetMarginGeometry(graphPtr, &graphPtr->topMargin);
  bottom = GetMarginGeometry(graphPtr, &graphPtr->bottomMargin);

  pad = graphPtr->bottomMargin.maxTickWidth;
  if (pad < graphPtr->topMargin.maxTickWidth)
    pad = graphPtr->topMargin.maxTickWidth;

  pad = pad / 2 + 3;
  if (right < pad)
    right = pad;

  if (left < pad)
    left = pad;

  pad = graphPtr->leftMargin.maxTickHeight;
  if (pad < graphPtr->rightMargin.maxTickHeight)
    pad = graphPtr->rightMargin.maxTickHeight;

  pad = pad / 2;
  if (top < pad)
    top = pad;

  if (bottom < pad)
    bottom = pad;

  if (graphPtr->leftMargin.reqSize > 0)
    left = graphPtr->leftMargin.reqSize;

  if (graphPtr->rightMargin.reqSize > 0)
    right = graphPtr->rightMargin.reqSize;

  if (graphPtr->topMargin.reqSize > 0)
    top = graphPtr->topMargin.reqSize;

  if (graphPtr->bottomMargin.reqSize > 0)
    bottom = graphPtr->bottomMargin.reqSize;

  /* 
   * Step 2:  Add the graph title height to the top margin. 
   */
  if (graphPtr->title) {
    top += graphPtr->titleHeight + 6;
  }
  inset = (graphPtr->inset + graphPtr->plotBW);
  inset2 = 2 * inset;

  /* 
   * Step 3: Estimate the size of the plot area from the remaining
   *	       space.  This may be overridden by the -plotwidth and
   *	       -plotheight graph options.  We use this to compute the
   *	       size of the legend. 
   */
  if (width == 0) {
    width = 400;
  }
  if (height == 0) {
    height = 400;
  }
  plotWidth  = (graphPtr->reqPlotWidth > 0) ? graphPtr->reqPlotWidth :
    width - (inset2 + left + right); /* Plot width. */
  plotHeight = (graphPtr->reqPlotHeight > 0) ? graphPtr->reqPlotHeight : 
    height - (inset2 + top + bottom); /* Plot height. */
  Blt_MapLegend(graphPtr, plotWidth, plotHeight);

  /* 
   * Step 2:  Add the legend to the appropiate margin. 
   */
  if (!Blt_Legend_IsHidden(graphPtr)) {
    switch (Blt_Legend_Site(graphPtr)) {
    case LEGEND_RIGHT:
      right += Blt_Legend_Width(graphPtr) + 2;
      break;
    case LEGEND_LEFT:
      left += Blt_Legend_Width(graphPtr) + 2;
      break;
    case LEGEND_TOP:
      top += Blt_Legend_Height(graphPtr) + 2;
      break;
    case LEGEND_BOTTOM:
      bottom += Blt_Legend_Height(graphPtr) + 2;
      break;
    case LEGEND_XY:
    case LEGEND_PLOT:
    case LEGEND_WINDOW:
      /* Do nothing. */
      break;
    }
  }

  /* 
   * Recompute the plotarea or graph size, now accounting for the legend. 
   */
  if (graphPtr->reqPlotWidth == 0) {
    plotWidth = width  - (inset2 + left + right);
    if (plotWidth < 1) {
      plotWidth = 1;
    }
  }
  if (graphPtr->reqPlotHeight == 0) {
    plotHeight = height - (inset2 + top + bottom);
    if (plotHeight < 1) {
      plotHeight = 1;
    }
  }

  /*
   * Step 5: If necessary, correct for the requested plot area aspect
   *	       ratio.
   */
  if ((graphPtr->reqPlotWidth == 0) && (graphPtr->reqPlotHeight == 0) && 
      (graphPtr->aspect > 0.0f)) {
    float ratio;

    /* 
     * Shrink one dimension of the plotarea to fit the requested
     * width/height aspect ratio.
     */
    ratio = (float)plotWidth / (float)plotHeight;
    if (ratio > graphPtr->aspect) {
      int scaledWidth;

      /* Shrink the width. */
      scaledWidth = (int)(plotHeight * graphPtr->aspect);
      if (scaledWidth < 1) {
	scaledWidth = 1;
      }
      /* Add the difference to the right margin. */
      /* CHECK THIS: w = scaledWidth; */
      right += (plotWidth - scaledWidth);
    } else {
      int scaledHeight;

      /* Shrink the height. */
      scaledHeight = (int)(plotWidth / graphPtr->aspect);
      if (scaledHeight < 1) {
	scaledHeight = 1;
      }
      /* Add the difference to the top margin. */
      /* CHECK THIS: h = scaledHeight; */
      top += (plotHeight - scaledHeight); 
    }
  }

  /* 
   * Step 6: If there's multiple axes in a margin, the axis titles will be
   *	       displayed in the adjoining margins.  Make sure there's room 
   *	       for the longest axis titles.
   */

  if (top < graphPtr->leftMargin.axesTitleLength) {
    top = graphPtr->leftMargin.axesTitleLength;
  }
  if (right < graphPtr->bottomMargin.axesTitleLength) {
    right = graphPtr->bottomMargin.axesTitleLength;
  }
  if (top < graphPtr->rightMargin.axesTitleLength) {
    top = graphPtr->rightMargin.axesTitleLength;
  }
  if (right < graphPtr->topMargin.axesTitleLength) {
    right = graphPtr->topMargin.axesTitleLength;
  }

  /* 
   * Step 7: Override calculated values with requested margin sizes.
   */
  if (graphPtr->leftMargin.reqSize > 0) {
    left = graphPtr->leftMargin.reqSize;
  }
  if (graphPtr->rightMargin.reqSize > 0) {
    right = graphPtr->rightMargin.reqSize;
  }
  if (graphPtr->topMargin.reqSize > 0) {
    top = graphPtr->topMargin.reqSize;
  }
  if (graphPtr->bottomMargin.reqSize > 0) {
    bottom = graphPtr->bottomMargin.reqSize;
  }
  if (graphPtr->reqPlotWidth > 0) {	
    int w;

    /* 
     * Width of plotarea is constained.  If there's extra space, add it to
     * th left and/or right margins.  If there's too little, grow the
     * graph width to accomodate it.
     */
    w = plotWidth + inset2 + left + right;
    if (width > w) {		/* Extra space in window. */
      int extra;

      extra = (width - w) / 2;
      if (graphPtr->leftMargin.reqSize == 0) { 
	left += extra;
	if (graphPtr->rightMargin.reqSize == 0) { 
	  right += extra;
	} else {
	  left += extra;
	}
      } else if (graphPtr->rightMargin.reqSize == 0) {
	right += extra + extra;
      }
    } else if (width < w) {
      width = w;
    }
  } 
  if (graphPtr->reqPlotHeight > 0) {	/* Constrain the plotarea height. */
    int h;

    /* 
     * Height of plotarea is constained.  If there's extra space, 
     * add it to th top and/or bottom margins.  If there's too little,
     * grow the graph height to accomodate it.
     */
    h = plotHeight + inset2 + top + bottom;
    if (height > h) {		/* Extra space in window. */
      int extra;

      extra = (height - h) / 2;
      if (graphPtr->topMargin.reqSize == 0) { 
	top += extra;
	if (graphPtr->bottomMargin.reqSize == 0) { 
	  bottom += extra;
	} else {
	  top += extra;
	}
      } else if (graphPtr->bottomMargin.reqSize == 0) {
	bottom += extra + extra;
      }
    } else if (height < h) {
      height = h;
    }
  }	
  graphPtr->width  = width;
  graphPtr->height = height;
  graphPtr->left   = left + inset;
  graphPtr->top    = top + inset;
  graphPtr->right  = width - right - inset;
  graphPtr->bottom = height - bottom - inset;

  graphPtr->leftMargin.width    = left   + graphPtr->inset;
  graphPtr->rightMargin.width   = right  + graphPtr->inset;
  graphPtr->topMargin.height    = top    + graphPtr->inset;
  graphPtr->bottomMargin.height = bottom + graphPtr->inset;
	    
  graphPtr->vOffset = graphPtr->top + graphPtr->yPad;
  graphPtr->vRange  = plotHeight - 2*graphPtr->yPad;
  graphPtr->hOffset = graphPtr->left + graphPtr->xPad;
  graphPtr->hRange  = plotWidth  - 2*graphPtr->xPad;

  if (graphPtr->vRange < 1) {
    graphPtr->vRange = 1;
  }
  if (graphPtr->hRange < 1) {
    graphPtr->hRange = 1;
  }
  graphPtr->hScale = 1.0f / (float)graphPtr->hRange;
  graphPtr->vScale = 1.0f / (float)graphPtr->vRange;

  /*
   * Calculate the placement of the graph title so it is centered within the
   * space provided for it in the top margin
   */
  titleY = graphPtr->titleHeight;
  graphPtr->titleY = 3 + graphPtr->inset;
  graphPtr->titleX = (graphPtr->right + graphPtr->left) / 2;

}

static int ConfigureAxis(Axis *axisPtr)
{
  Graph* graphPtr = axisPtr->obj.graphPtr;
  float angle;

  /* Check the requested axis limits. Can't allow -min to be greater than
   * -max.  Do this regardless of -checklimits option. We want to always 
   * detect when the user has zoomed in beyond the precision of the data.*/
  if (((!isnan(axisPtr->reqMin)) && (!isnan(axisPtr->reqMax))) &&
      (axisPtr->reqMin >= axisPtr->reqMax)) {
    char msg[200];
    sprintf_s(msg, 200, 
	      "impossible axis limits (-min %g >= -max %g) for \"%s\"",
	      axisPtr->reqMin, axisPtr->reqMax, axisPtr->obj.name);
    Tcl_AppendResult(graphPtr->interp, msg, NULL);
    return TCL_ERROR;
  }
  axisPtr->scrollMin = axisPtr->reqScrollMin;
  axisPtr->scrollMax = axisPtr->reqScrollMax;
  if (axisPtr->logScale) {
    if (axisPtr->checkLimits) {
      /* Check that the logscale limits are positive.  */
      if ((!isnan(axisPtr->reqMin)) && (axisPtr->reqMin <= 0.0)) {
	Tcl_AppendResult(graphPtr->interp,"bad logscale -min limit \"", 
			 Blt_Dtoa(graphPtr->interp, axisPtr->reqMin), 
			 "\" for axis \"", axisPtr->obj.name, "\"", 
			 NULL);
	return TCL_ERROR;
      }
    }
    if ((!isnan(axisPtr->scrollMin)) && (axisPtr->scrollMin <= 0.0)) {
      axisPtr->scrollMin = NAN;
    }
    if ((!isnan(axisPtr->scrollMax)) && (axisPtr->scrollMax <= 0.0)) {
      axisPtr->scrollMax = NAN;
    }
  }
  angle = fmod(axisPtr->tickAngle, 360.0);
  if (angle < 0.0f) {
    angle += 360.0f;
  }
  axisPtr->tickAngle = angle;
  ResetTextStyles(axisPtr);

  axisPtr->titleWidth = axisPtr->titleHeight = 0;
  if (axisPtr->title) {
    unsigned int w, h;

    Blt_GetTextExtents(axisPtr->titleFont, 0, axisPtr->title, -1, &w, &h);
    axisPtr->titleWidth = (unsigned short int)w;
    axisPtr->titleHeight = (unsigned short int)h;
  }

  /* 
   * Don't bother to check what configuration options have changed.  Almost
   * every option changes the size of the plotting area (except for -color
   * and -titlecolor), requiring the graph and its contents to be completely
   * redrawn.
   *
   * Recompute the scale and offset of the axis in case -min, -max options
   * have changed.
   */
  graphPtr->flags |= REDRAW_WORLD;
  graphPtr->flags |= MAP_WORLD | RESET_AXES | CACHE_DIRTY;
  axisPtr->flags |= DIRTY;
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int GetAxisFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			  Axis **axisPtrPtr)
{
  Tcl_HashEntry *hPtr;
  const char *name;

  *axisPtrPtr = NULL;
  name = Tcl_GetString(objPtr);
  hPtr = Tcl_FindHashEntry(&graphPtr->axes.table, name);
  if (hPtr) {
    Axis *axisPtr;

    axisPtr = Tcl_GetHashValue(hPtr);
    if ((axisPtr->flags & DELETE_PENDING) == 0) {
      *axisPtrPtr = axisPtr;
      return TCL_OK;
    }
  }
  if (interp) {
    Tcl_AppendResult(interp, "can't find axis \"", name, "\" in \"", 
		     Tk_PathName(graphPtr->tkwin), "\"", NULL);
  }
  return TCL_ERROR;
}

static int GetAxisByClass(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr,
			  ClassId classId, Axis **axisPtrPtr)
{
  Axis *axisPtr;

  if (GetAxisFromObj(interp, graphPtr, objPtr, &axisPtr) != TCL_OK)
    return TCL_ERROR;

  if (classId != CID_NONE) {
    // Set the axis type on the first use of it.
    if ((axisPtr->refCount == 0) || (axisPtr->obj.classId == CID_NONE))
      Blt_GraphSetObjectClass(&axisPtr->obj, classId);

    else if (axisPtr->obj.classId != classId) {
      if (!interp)
	Tcl_AppendResult(interp, "axis \"", Tcl_GetString(objPtr),
			 "\" is already in use on an opposite ", 
			 axisPtr->obj.className, "-axis", 
			 NULL);
      return TCL_ERROR;
    }
    axisPtr->refCount++;
  }

  *axisPtrPtr = axisPtr;
  return TCL_OK;
}

void Blt_DestroyAxes(Graph* graphPtr)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = Tcl_GetHashValue(hPtr);
    axisPtr->hashPtr = NULL;
    DestroyAxis(axisPtr);
  }
  Tcl_DeleteHashTable(&graphPtr->axes.table);

  for (int ii=0; ii<4; ii++)
    Blt_Chain_Destroy(graphPtr->axisChain[ii]);

  Tcl_DeleteHashTable(&graphPtr->axes.tagTable);
  Blt_Chain_Destroy(graphPtr->axes.displayList);
}

void Blt_ConfigureAxes(Graph* graphPtr)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = Tcl_GetHashValue(hPtr);
    ConfigureAxis(axisPtr);
  }
}

void Blt_MapAxes(Graph* graphPtr)
{
  for (int margin = 0; margin < 4; margin++) {
    Blt_Chain chain;
    Blt_ChainLink link;
    int count, offset;

    chain = graphPtr->margins[margin].axes;
    count = offset = 0;
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr;

      axisPtr = Blt_Chain_GetValue(link);
      if (!axisPtr->use || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (graphPtr->stackAxes) {
	if (axisPtr->reqNumMajorTicks <= 0) {
	  axisPtr->reqNumMajorTicks = 4;
	}
	MapStackedAxis(axisPtr, count, margin);
      } else {
	if (axisPtr->reqNumMajorTicks <= 0) {
	  axisPtr->reqNumMajorTicks = 4;
	}
	MapAxis(axisPtr, offset, margin);
      }
      if (axisPtr->showGrid)
	MapGridlines(axisPtr);

      offset += (AxisIsHorizontal(axisPtr)) 
	? axisPtr->height : axisPtr->width;
      count++;
    }
  }
}

void Blt_DrawAxes(Graph* graphPtr, Drawable drawable)
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_LastLink(graphPtr->margins[i].axes); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
      Axis *axisPtr = Blt_Chain_GetValue(link);
      if (!axisPtr->hide && axisPtr->use && !(axisPtr->flags & DELETE_PENDING))
	DrawAxis(axisPtr, drawable);
    }
  }
}

void Blt_DrawGrids(Graph* graphPtr, Drawable drawable) 
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->margins[i].axes); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = Blt_Chain_GetValue(link);
      if (axisPtr->hide || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (axisPtr->use && axisPtr->showGrid) {
	Blt_Draw2DSegments(graphPtr->display, drawable, 
			   axisPtr->major.gc, axisPtr->major.segments, 
			   axisPtr->major.nUsed);

	if (axisPtr->showGridMinor)
	  Blt_Draw2DSegments(graphPtr->display, drawable, 
			     axisPtr->minor.gc, axisPtr->minor.segments, 
			     axisPtr->minor.nUsed);
      }
    }
  }
}

void Blt_GridsToPostScript(Graph* graphPtr, Blt_Ps ps) 
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->margins[i].axes); link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = Blt_Chain_GetValue(link);
      if (axisPtr->hide || !axisPtr->showGrid || !axisPtr->use || 
	  (axisPtr->flags & DELETE_PENDING))
	continue;

      Blt_Ps_Format(ps, "%% Axis %s: grid line attributes\n",
		    axisPtr->obj.name);
      Blt_Ps_XSetLineAttributes(ps, axisPtr->major.color, 
				axisPtr->major.lineWidth, 
				&axisPtr->major.dashes, CapButt, 
				JoinMiter);
      Blt_Ps_Format(ps, "%% Axis %s: major grid line segments\n",
		    axisPtr->obj.name);
      Blt_Ps_Draw2DSegments(ps, axisPtr->major.segments, 
			    axisPtr->major.nUsed);
      if (axisPtr->showGridMinor) {
	Blt_Ps_XSetLineAttributes(ps, axisPtr->minor.color, 
				  axisPtr->minor.lineWidth, 
				  &axisPtr->minor.dashes, CapButt, 
				  JoinMiter);
	Blt_Ps_Format(ps, "%% Axis %s: minor grid line segments\n",
		      axisPtr->obj.name);
	Blt_Ps_Draw2DSegments(ps, axisPtr->minor.segments, 
			      axisPtr->minor.nUsed);
      }
    }
  }
}

void Blt_AxesToPostScript(Graph* graphPtr, Blt_Ps ps) 
{
  Margin *mp, *mend;

  for (mp = graphPtr->margins, mend = mp + 4; mp < mend; mp++) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(mp->axes); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr;

      axisPtr = Blt_Chain_GetValue(link);
      if (!axisPtr->hide && axisPtr->use && !(axisPtr->flags & DELETE_PENDING))
	AxisToPostScript(ps, axisPtr);
    }
  }
}

void Blt_DrawAxisLimits(Graph* graphPtr, Drawable drawable)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
  char minString[200], maxString[200];
  int vMin, hMin, vMax, hMax;

#define SPACING 8
  vMin = vMax = graphPtr->left + graphPtr->xPad + 2;
  hMin = hMax = graphPtr->bottom - graphPtr->yPad - 2;	/* Offsets */

  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr;
    Dim2D textDim;
    const char *minFmt, *maxFmt;
    char *minPtr, *maxPtr;
    int isHoriz;

    axisPtr = Tcl_GetHashValue(hPtr);
    if (axisPtr->flags & DELETE_PENDING) {
      continue;
    } 
    if (axisPtr->nFormats == 0) {
      continue;
    }
    isHoriz = AxisIsHorizontal(axisPtr);
    minPtr = maxPtr = NULL;
    minFmt = maxFmt = axisPtr->limitsFormats[0];
    if (axisPtr->nFormats > 1) {
      maxFmt = axisPtr->limitsFormats[1];
    }
    if (minFmt[0] != '\0') {
      minPtr = minString;
      sprintf_s(minString, 200, minFmt, axisPtr->axisRange.min);
    }
    if (maxFmt[0] != '\0') {
      maxPtr = maxString;
      sprintf_s(maxString, 200, maxFmt, axisPtr->axisRange.max);
    }
    if (axisPtr->descending) {
      char *tmp;

      tmp = minPtr, minPtr = maxPtr, maxPtr = tmp;
    }
    if (maxPtr) {
      if (isHoriz) {
	Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
	Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SE);
	Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		      &axisPtr->limitsTextStyle, graphPtr->right, hMax, &textDim);
	hMax -= (textDim.height + SPACING);
      } else {
	Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
	Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_NW);
	Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		      &axisPtr->limitsTextStyle, vMax, graphPtr->top, &textDim);
	vMax += (textDim.width + SPACING);
      }
    }
    if (minPtr) {
      Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SW);
      if (isHoriz) {
	Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
	Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		      &axisPtr->limitsTextStyle, graphPtr->left, hMin, &textDim);
	hMin -= (textDim.height + SPACING);
      } else {
	Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
	Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		      &axisPtr->limitsTextStyle, vMin, graphPtr->bottom, &textDim);
	vMin += (textDim.width + SPACING);
      }
    }
  } /* Loop on axes */
}

void Blt_AxisLimitsToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
  double vMin, hMin, vMax, hMax;
  char string[200];

#define SPACING 8
  vMin = vMax = graphPtr->left + graphPtr->xPad + 2;
  hMin = hMax = graphPtr->bottom - graphPtr->yPad - 2;	/* Offsets */
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr;
    const char *minFmt, *maxFmt;
    unsigned int textWidth, textHeight;

    axisPtr = Tcl_GetHashValue(hPtr);
    if (axisPtr->flags & DELETE_PENDING) {
      continue;
    } 
    if (axisPtr->nFormats == 0) {
      continue;
    }
    minFmt = maxFmt = axisPtr->limitsFormats[0];
    if (axisPtr->nFormats > 1) {
      maxFmt = axisPtr->limitsFormats[1];
    }
    if (*maxFmt != '\0') {
      sprintf_s(string, 200, maxFmt, axisPtr->axisRange.max);
      Blt_GetTextExtents(axisPtr->tickFont, 0, string, -1, &textWidth,
			 &textHeight);
      if ((textWidth > 0) && (textHeight > 0)) {
	if (axisPtr->obj.classId == CID_AXIS_X) {
	  Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
	  Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SE);
	  Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
			  (double)graphPtr->right, hMax);
	  hMax -= (textWidth + SPACING);
	} else {
	  Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
	  Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_NW);
	  Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle,
			  vMax, (double)graphPtr->top);
	  vMax += (textWidth + SPACING);
	}
      }
    }
    if (*minFmt != '\0') {
      sprintf_s(string, 200, minFmt, axisPtr->axisRange.min);
      Blt_GetTextExtents(axisPtr->tickFont, 0, string, -1, &textWidth,
			 &textHeight);
      if ((textWidth > 0) && (textHeight > 0)) {
	Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SW);
	if (axisPtr->obj.classId == CID_AXIS_X) {
	  Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
	  Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
			  (double)graphPtr->left, hMin);
	  hMin -= (textWidth + SPACING);
	} else {
	  Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
	  Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
			  vMin, (double)graphPtr->bottom);
	  vMin += (textWidth + SPACING);
	}
      }
    }
  }
}

Axis *Blt_GetFirstAxis(Blt_Chain chain)
{
  Blt_ChainLink link = Blt_Chain_FirstLink(chain);
  if (!link)
    return NULL;

  return Blt_Chain_GetValue(link);
}

Axis *Blt_NearestAxis(Graph* graphPtr, int x, int y)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
    
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); 
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr;

    axisPtr = Tcl_GetHashValue(hPtr);
    if (axisPtr->hide || !axisPtr->use || (axisPtr->flags & DELETE_PENDING))
      continue;

    if (axisPtr->showTicks) {
      Blt_ChainLink link;

      for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
	   link = Blt_Chain_NextLink(link)) {	
	TickLabel *labelPtr;
	Point2d t;
	double rw, rh;
	Point2d bbox[5];

	labelPtr = Blt_Chain_GetValue(link);
	Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			   axisPtr->tickAngle, &rw, &rh, bbox);
	t = Blt_AnchorPoint(labelPtr->anchorPos.x, 
			    labelPtr->anchorPos.y, rw, rh, axisPtr->tickAnchor);
	t.x = x - t.x - (rw * 0.5);
	t.y = y - t.y - (rh * 0.5);

	bbox[4] = bbox[0];
	if (Blt_PointInPolygon(&t, bbox, 5)) {
	  axisPtr->detail = "label";
	  return axisPtr;
	}
      }
    }
    if (axisPtr->title) {	/* and then the title string. */
      Point2d bbox[5];
      Point2d t;
      double rw, rh;
      unsigned int w, h;

      Blt_GetTextExtents(axisPtr->titleFont, 0, axisPtr->title,-1,&w,&h);
      Blt_GetBoundingBox(w, h, axisPtr->titleAngle, &rw, &rh, bbox);
      t = Blt_AnchorPoint(axisPtr->titlePos.x, axisPtr->titlePos.y, 
			  rw, rh, axisPtr->titleAnchor);
      /* Translate the point so that the 0,0 is the upper left 
       * corner of the bounding box.  */
      t.x = x - t.x - (rw * 0.5);
      t.y = y - t.y - (rh * 0.5);
	    
      bbox[4] = bbox[0];
      if (Blt_PointInPolygon(&t, bbox, 5)) {
	axisPtr->detail = "title";
	return axisPtr;
      }
    }
    if (axisPtr->lineWidth > 0) {	/* Check for the axis region */
      if ((x <= axisPtr->right) && (x >= axisPtr->left) && 
	  (y <= axisPtr->bottom) && (y >= axisPtr->top)) {
	axisPtr->detail = "line";
	return axisPtr;
      }
    }
  }
  return NULL;
}
 
ClientData Blt_MakeAxisTag(Graph* graphPtr, const char *tagName)
{
  Tcl_HashEntry *hPtr;
  int isNew;

  hPtr = Tcl_CreateHashEntry(&graphPtr->axes.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->axes.tagTable, hPtr);
}

static Blt_OptionFreeProc  FreeAxisProc;
static Blt_OptionPrintProc AxisToObjProc;
static Blt_OptionParseProc ObjToAxisProc;
Blt_CustomOption bltXAxisOption = {
  ObjToAxisProc, AxisToObjProc, FreeAxisProc, (ClientData)CID_AXIS_X
};
Blt_CustomOption bltYAxisOption = {
  ObjToAxisProc, AxisToObjProc, FreeAxisProc, (ClientData)CID_AXIS_Y
};

static void FreeAxisProc(ClientData clientData, Display *display,
			 char *widgRec, int offset)
{
  Axis **axisPtrPtr = (Axis **)(widgRec + offset);

  if (*axisPtrPtr) {
    ReleaseAxis(*axisPtrPtr);
    *axisPtrPtr = NULL;
  }
}

static int ObjToAxisProc(ClientData clientData, Tcl_Interp* interp,
			 Tk_Window tkwin, Tcl_Obj *objPtr,
			 char *widgRec, int offset, int flags)	
{
  ClassId classId = (ClassId)clientData;
  Axis **axisPtrPtr = (Axis **)(widgRec + offset);
  Axis *axisPtr;
  Graph* graphPtr;

  if (flags & BLT_CONFIG_NULL_OK) {
    const char *string;

    string  = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
      ReleaseAxis(*axisPtrPtr);
      *axisPtrPtr = NULL;
      return TCL_OK;
    }
  }
  graphPtr = Blt_GetGraphFromWindowData(tkwin);
  if (GetAxisByClass(interp, graphPtr, objPtr, classId, &axisPtr) 
      != TCL_OK) {
    return TCL_ERROR;
  }
  ReleaseAxis(*axisPtrPtr);
  *axisPtrPtr = axisPtr;
  return TCL_OK;
}

static Tcl_Obj *AxisToObjProc(ClientData clientData, Tcl_Interp* interp,
			      Tk_Window tkwin, char *widgRec, 
			      int offset, int flags)
{
  Axis* axisPtr = *(Axis **)(widgRec + offset);
  const char* name = axisPtr ? axisPtr->obj.name : "";
  return Tcl_NewStringObj(name, -1);
}

