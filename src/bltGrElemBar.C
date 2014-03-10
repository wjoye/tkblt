/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrBar.c --
 *
 * This module implements barchart elements for the BLT graph widget.
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
#include "bltGrElem.h"
#include "bltConfig.h"

#define CLAMP(x,l,h)	((x) = (((x)<(l))? (l) : ((x)>(h)) ? (h) : (x)))

#define PointInRectangle(r,x0,y0)					\
  (((x0) <= (int)((r)->x + (r)->width - 1)) && ((x0) >= (int)(r)->x) && \
   ((y0) <= (int)((r)->y + (r)->height - 1)) && ((y0) >= (int)(r)->y))

typedef struct {
  float x1, y1, x2, y2;
} BarRegion;

typedef struct {
  const char *name;			/* Pen style identifier.  If NULL, pen
					 * was statically allocated. */
  ClassId classId;			/* Type of pen */
  const char *typeId;			/* String token identifying the type of
					 * pen */
  unsigned int flags;			/* Indicates if the pen element is
					 * active or normal */
  int refCount;			/* Reference count for elements using
				 * this pen. */
  Tcl_HashEntry *hashPtr;
  Tk_OptionTable optionTable;	/* Configuration specifications */
  PenConfigureProc *configProc;
  PenDestroyProc *destroyProc;
  Graph* graphPtr;			/* Graph that the pen is associated
					 * with. */
  /* Barchart specific pen fields start here. */
  XColor* outlineColor;		/* Outline (foreground) color of bar */
  Tk_3DBorder fill;		/* 3D border and fill (background)
				 * color */
  int borderWidth;			/* 3D border width of bar */
  int relief;				/* Relief of the bar */
  Pixmap stipple;			/* Stipple */
  GC fillGC;				/* Graphics context */
  GC outlineGC;			/* GC for outline of bar. */

  /* Error bar attributes. */
  int errorBarShow;			/* Describes which error bars to
					 * display: none, x, y, or * both. */

  int errorBarLineWidth;		/* Width of the error bar segments. */

  int errorBarCapWidth;
  XColor* errorBarColor;		/* Color of the error bar. */

  GC errorBarGC;			/* Error bar graphics context. */

  /* Show value attributes. */
  int valueShow;			/* Indicates whether to display data
					 * value.  Values are x, y, or none. */

  const char *valueFormat;		/* A printf format string. */
  TextStyle valueStyle;		/* Text attributes (color, font,
				 * rotation, etc.) of the value. */
    
} BarPen;

typedef struct {
  Weight weight;			/* Weight range where this pen is
					 * valid. */
  BarPen* penPtr;			/* Pen to use. */

  XRectangle *bars;			/* Indicates starting location in bar
					 * array for this pen. */
  int nBars;				/* # of bar segments for this pen. */

  GraphSegments xeb, yeb;		/* X and Y error bars. */

  int symbolSize;			/* Size of the pen's symbol scaled to
					 * the current graph size. */
  int errorBarCapWidth;		/* Length of the cap ends on each error
				 * bar. */

} BarStyle;

typedef struct {
  GraphObj obj;			/* Must be first field in element. */
  unsigned int flags;		
  int hide;
  Tcl_HashEntry *hashPtr;

  /* Fields specific to elements. */
  const char *label;			/* Label displayed in legend */
  unsigned short row, col;		/* Position of the entry in the
					 * legend. */
  int legendRelief;			/* Relief of label in legend. */
  Axis2d axes;			/* X-axis and Y-axis mapping the
				 * element */
  ElemValues x, y, w;			/* Contains array of floating point
					 * graph coordinate values. Also holds
					 * min/max and the number of
					 * coordinates */
  int *activeIndices;			/* Array of indices (malloc-ed) which
					 * indicate which data points are active
					 * (drawn * with "active" colors). */
  int nActiveIndices;			/* Number of active data points.
					 * Special case: if nActiveIndices < 0
					 * and the active bit is set in "flags",
					 * then all data * points are drawn
					 * active. */
  ElementProcs *procsPtr;
  Tk_OptionTable optionTable;	/* Configuration specifications. */
  BarPen *activePenPtr;		/* Standard Pens */
  BarPen *normalPenPtr;
  BarPen *builtinPenPtr;
  Blt_Chain stylePalette;		/* Palette of pens. */

  /* Symbol scaling */
  int scaleSymbols;			/* If non-zero, the symbols will scale
					 * in size as the graph is zoomed
					 * in/out.  */
  double xRange, yRange;		/* Initial X-axis and Y-axis ranges:
					 * used to scale the size of element's
					 * symbol. */
  int state;
  Blt_ChainLink link;

  /* Fields specific to the barchart element */

  double barWidth;
  const char *groupName;

  int *barToData;
  XRectangle *bars;		       /* Array of rectangles comprising the bar
					* segments of the element. */
  int *activeToData;
  XRectangle *activeRects;

  int nBars;				/* # of visible bar segments for
					 * element */
  int nActive;

  int xPad;				/* Spacing on either side of bar */

  ElemValues xError;			/* Relative/symmetric X error values. */
  ElemValues yError;			/* Relative/symmetric Y error values. */
  ElemValues xHigh, xLow;		/* Absolute/asymmetric X-coordinate
					 * high/low error values. */
  ElemValues yHigh, yLow;		/* Absolute/asymmetric Y-coordinate
					 * high/low error values. */
  BarPen builtinPen;

  GraphSegments xeb, yeb;

  int errorBarCapWidth;		/* Length of cap on error bars */
} BarElement;

// Defs

static void InitBarPen(Graph* graphPtr, BarPen* penPtr);
static void ResetBar(BarElement* elemPtr);

static PenConfigureProc ConfigurePenProc;
static PenDestroyProc DestroyPenProc;
static ElementClosestProc ClosestBarProc;
static ElementConfigProc ConfigureBarProc;
static ElementDestroyProc DestroyBarProc;
static ElementDrawProc DrawActiveBarProc;
static ElementDrawProc DrawNormalBarProc;
static ElementDrawSymbolProc DrawSymbolProc;
static ElementExtentsProc GetBarExtentsProc;
static ElementToPostScriptProc ActiveBarToPostScriptProc;
static ElementToPostScriptProc NormalBarToPostScriptProc;
static ElementSymbolToPostScriptProc SymbolToPostScriptProc;
static ElementMapProc MapBarProc;

static ElementProcs barProcs = {
  ClosestBarProc,
  ConfigureBarProc,
  DestroyBarProc,
  DrawActiveBarProc,
  DrawNormalBarProc,
  DrawSymbolProc,
  GetBarExtentsProc,
  ActiveBarToPostScriptProc,
  NormalBarToPostScriptProc,
  SymbolToPostScriptProc,
  MapBarProc,
};

// OptionSpecs

static Tk_ObjCustomOption styleObjOption =
  {
    "style", StyleSetProc, StyleGetProc, NULL, NULL, 
    (ClientData)sizeof(BarStyle)

  };

extern Tk_ObjCustomOption barPenObjOption;
extern Tk_ObjCustomOption pairsObjOption;
extern Tk_ObjCustomOption valuesObjOption;
extern Tk_ObjCustomOption xAxisObjOption;
extern Tk_ObjCustomOption yAxisObjOption;

static Tk_OptionSpec barElemOptionSpecs[] = {
  {TK_OPTION_CUSTOM, "-activepen", "activePen", "ActivePen",
   "activeBar", -1, Tk_Offset(BarElement, activePenPtr), 
   TK_OPTION_NULL_OK, &barPenObjOption, 0},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarElement, builtinPen.fill),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-barwidth", "barWidth", "BarWidth",
   0, -1, Tk_Offset(BarElement, barWidth), 0, NULL, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags",
   "all", -1, Tk_Offset(BarElement, obj.tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(BarElement, builtinPen.borderWidth),
   0, NULL, 0},
  {TK_OPTION_SYNONYM, "-color", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-data", "data", "Data", 
   NULL, -1, 0, 0, &pairsObjOption, MAP_ITEM},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(BarElement, builtinPen.errorBarColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS,"-errorbarwidth", "errorBarWidth", "ErrorBarWidth",
   "1", -1, Tk_Offset(BarElement, builtinPen.errorBarLineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(BarElement, builtinPen.errorBarCapWidth), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarElement, builtinPen.outlineColor),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(BarElement, hide), 0, NULL, MAP_ITEM},
  {TK_OPTION_STRING, "-label", "label", "Label",
   NULL, -1, Tk_Offset(BarElement, label), TK_OPTION_NULL_OK, NULL, MAP_ITEM},
  {TK_OPTION_RELIEF, "-legendrelief", "legendRelief", "LegendRelief",
   "flat", -1, Tk_Offset(BarElement, legendRelief), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX", 
   "x", -1, Tk_Offset(BarElement, axes.x), 0, &xAxisObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY",
   "y", -1, Tk_Offset(BarElement, axes.y), 0, &yAxisObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_CUSTOM, "-pen", "pen", "Pen", 
   NULL, -1, Tk_Offset(BarElement, normalPenPtr), 
   TK_OPTION_NULL_OK, &barPenObjOption},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "raised", -1, Tk_Offset(BarElement, builtinPen.relief), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(BarElement, builtinPen.errorBarShow), 
   0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(BarElement, builtinPen.valueShow), 
   0, &fillObjOption, 0},
  {TK_OPTION_STRING, "-stack", "stack", "Stack", 
   NULL, -1, Tk_Offset(BarElement, groupName), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple",
   NULL, -1, Tk_Offset(BarElement, builtinPen.stipple), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-styles", "styles", "Styles",
   "", -1, Tk_Offset(BarElement, stylePalette), 0, &styleObjOption, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(BarElement, builtinPen.valueStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarElement,builtinPen.valueStyle.color),
   0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(BarElement, builtinPen.valueStyle.font),
   0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(BarElement, builtinPen.valueFormat),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(BarElement, builtinPen.valueStyle.angle), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-weights", "weights", "Weights",
   NULL, -1, Tk_Offset(BarElement, w), 0, &valuesObjOption, 0},
  {TK_OPTION_CUSTOM, "-x", "x", "X", 
   NULL, -1, Tk_Offset(BarElement, x), 0, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xdata", "xData", "XData", 
   NULL, -1, Tk_Offset(BarElement, x), 0, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xerror", "xError", "XError", 
   NULL, -1, Tk_Offset(BarElement, xError), 0, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xhigh", "xHigh", "XHigh", 
   NULL, -1, Tk_Offset(BarElement, xHigh), 0, &valuesObjOption, 0},
  {TK_OPTION_CUSTOM, "-xlow", "xLow", "XLow", 
   NULL, -1, Tk_Offset(BarElement, xLow), 0, &valuesObjOption, 0},
  {TK_OPTION_CUSTOM, "-y", "y", "Y",
   NULL, -1, Tk_Offset(BarElement, y), 0, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-ydata", "yData", "YData", 
   NULL, -1, Tk_Offset(BarElement, y), 0, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-yerror", "yError", "YError", 
   NULL, -1, Tk_Offset(BarElement, yError), 0, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-yhigh", "yHigh", "YHigh",
   NULL, -1, Tk_Offset(BarElement, yHigh), 0, &valuesObjOption, 0},
  {TK_OPTION_CUSTOM, "-ylow", "yLow", "YLow", 
   NULL, -1, Tk_Offset(BarElement, yLow), 0, &valuesObjOption, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

static Tk_OptionSpec barPenOptionSpecs[] = {
  {TK_OPTION_BORDER, "-background", "background", "Background",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPen, fill), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(BarPen, borderWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(BarPen, errorBarColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarwidth", "errorBarWidth","ErrorBarWidth",
   "1", -1, Tk_Offset(BarPen, errorBarLineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(BarPen, errorBarCapWidth), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPen, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "raised", -1, Tk_Offset(BarPen, relief), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(BarPen, errorBarShow), 0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(BarPen, valueShow), 0, &fillObjOption, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple", 
   NULL, -1, Tk_Offset(BarPen, stipple), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING, "-type", "type", "Type",
   "bar", -1, Tk_Offset(BarPen, typeId), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(BarPen, valueStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPen, valueStyle.color), 0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(BarPen, valueStyle.font), 0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(BarPen, valueFormat), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(BarPen, valueStyle.angle), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

Element* Blt_BarElement(Graph* graphPtr, const char* name, ClassId classId)
{
  BarElement* elemPtr = calloc(1, sizeof(BarElement));
  elemPtr->procsPtr = &barProcs;
  elemPtr->obj.name = Blt_Strdup(name);
  Blt_GraphSetObjectClass(&elemPtr->obj, classId);
  elemPtr->obj.graphPtr = graphPtr;
  // this is an option and will be freed via Tk_FreeConfigOptions
  // By default an element's name and label are the same
  elemPtr->label = ckalloc(strlen(name)+1);
  if (name)
    strcpy((char*)elemPtr->label,(char*)name);

  elemPtr->builtinPenPtr = &elemPtr->builtinPen;
  InitBarPen(graphPtr, elemPtr->builtinPenPtr);
  Tk_InitOptions(graphPtr->interp, (char*)elemPtr->builtinPenPtr,
		 elemPtr->builtinPenPtr->optionTable, graphPtr->tkwin);
  elemPtr->stylePalette = Blt_Chain_Create();

  elemPtr->optionTable = 
    Tk_CreateOptionTable(graphPtr->interp, barElemOptionSpecs);

  return (Element*)elemPtr;
}

Pen* Blt_BarPen(Graph* graphPtr, const char *penName)
{
  BarPen* penPtr = calloc(1, sizeof(BarPen));
  InitBarPen(graphPtr, penPtr);
  penPtr->name = Blt_Strdup(penName);
  if (!strcmp(penName, "activeBar"))
    penPtr->flags = ACTIVE_PEN;

  return (Pen*)penPtr;
}

static void InitBarPen(Graph* graphPtr, BarPen* penPtr)
{
  penPtr->configProc = ConfigurePenProc;
  penPtr->destroyProc = DestroyPenProc;
  penPtr->flags = NORMAL_PEN;

  Blt_Ts_InitStyle(penPtr->valueStyle);

  penPtr->optionTable = 
    Tk_CreateOptionTable(graphPtr->interp, barPenOptionSpecs);
}

static void DestroyBarProc(Graph* graphPtr, Element* basePtr)
{
  BarElement* elemPtr = (BarElement*)basePtr;

  DestroyPenProc(graphPtr, (Pen*)&elemPtr->builtinPen);
  if (elemPtr->activePenPtr)
    Blt_FreePen((Pen *)elemPtr->activePenPtr);
  if (elemPtr->normalPenPtr)
    Blt_FreePen((Pen *)elemPtr->normalPenPtr);

  ResetBar(elemPtr);
  if (elemPtr->stylePalette) {
    Blt_FreeStylePalette(elemPtr->stylePalette);
    Blt_Chain_Destroy(elemPtr->stylePalette);
  }
  if (elemPtr->activeIndices) {
    free(elemPtr->activeIndices);
  }

  Tk_FreeConfigOptions((char*)elemPtr, elemPtr->optionTable, graphPtr->tkwin);
}

static void DestroyPenProc(Graph* graphPtr, Pen* basePtr)
{
  BarPen* penPtr = (BarPen*)basePtr;

  Blt_Ts_FreeStyle(graphPtr->display, &penPtr->valueStyle);
  if (penPtr->outlineGC)
    Tk_FreeGC(graphPtr->display, penPtr->outlineGC);

  if (penPtr->fillGC)
    Tk_FreeGC(graphPtr->display, penPtr->fillGC);

  if (penPtr->errorBarGC)
    Tk_FreeGC(graphPtr->display, penPtr->errorBarGC);

  Tk_FreeConfigOptions((char*)penPtr, penPtr->optionTable, graphPtr->tkwin);
}

// Configure

static int ConfigureBarProc(Graph* graphPtr, Element *basePtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  Blt_ChainLink link;
  BarStyle *stylePtr;

  if (ConfigurePenProc(graphPtr, (Pen*)&elemPtr->builtinPen)!= TCL_OK) {
    return TCL_ERROR;
  }

  // Point to the static normal pen if no external pens have been selected.
  link = Blt_Chain_FirstLink(elemPtr->stylePalette);
  if (!link) {
    link = Blt_Chain_AllocLink(sizeof(BarStyle));
    Blt_Chain_LinkAfter(elemPtr->stylePalette, link, NULL);
  }
  stylePtr = Blt_Chain_GetValue(link);
  stylePtr->penPtr = NORMALPEN(elemPtr);

  return TCL_OK;
}

static int ConfigurePenProc(Graph* graphPtr, Pen *basePtr)
{
  BarPen* penPtr = (BarPen*)basePtr;
  int screenNum = Tk_ScreenNumber(graphPtr->tkwin);
  XGCValues gcValues;
  unsigned long gcMask;
  GC newGC;

  // outlineGC
  gcMask = GCForeground | GCLineWidth;
  gcValues.line_width = LineWidth(penPtr->errorBarLineWidth);

  if (penPtr->outlineColor)
    gcValues.foreground = penPtr->outlineColor->pixel;
  else if (penPtr->fill)
    gcValues.foreground = Tk_3DBorderColor(penPtr->fill)->pixel;

  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (penPtr->outlineGC)
    Tk_FreeGC(graphPtr->display, penPtr->outlineGC);
  penPtr->outlineGC = newGC;

  newGC = NULL;
  if (penPtr->stipple != None) {
    // Handle old-style -stipple specially
    gcMask = GCForeground | GCBackground | GCFillStyle | GCStipple;
    gcValues.foreground = BlackPixel(graphPtr->display, screenNum);
    gcValues.background = WhitePixel(graphPtr->display, screenNum);
    if (penPtr->fill)
      gcValues.foreground = Tk_3DBorderColor(penPtr->fill)->pixel;
    else if (penPtr->outlineColor)
      gcValues.foreground = penPtr->outlineColor->pixel;

    gcValues.stipple = penPtr->stipple;
    gcValues.fill_style = FillStippled;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (penPtr->fillGC)
    Tk_FreeGC(graphPtr->display, penPtr->fillGC);
  penPtr->fillGC = newGC;

  // errorBarGC
  gcMask = GCForeground | GCLineWidth;
  XColor* colorPtr = penPtr->errorBarColor;
  if (!colorPtr)
    colorPtr = penPtr->outlineColor;
  gcValues.foreground = colorPtr->pixel;
  gcValues.line_width = LineWidth(penPtr->errorBarLineWidth);
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (penPtr->errorBarGC)
    Tk_FreeGC(graphPtr->display, penPtr->errorBarGC);
  penPtr->errorBarGC = newGC;

  return TCL_OK;
}

// Support

static void ResetStylePalette(Blt_Chain stylePalette)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(stylePalette); link; 
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr;

    stylePtr = Blt_Chain_GetValue(link);
    stylePtr->xeb.length = stylePtr->yeb.length = 0;
    stylePtr->nBars = 0;
  }
}

static void CheckBarStacks(Graph* graphPtr, Axis2d *pairPtr, 
			   double *minPtr, double *maxPtr)
{
  BarGroup *gp, *gend;

  if ((graphPtr->barMode != BARS_STACKED) || (graphPtr->nBarGroups == 0))
    return;

  for (gp = graphPtr->barGroups, gend = gp + graphPtr->nBarGroups; gp < gend;
       gp++) {
    if ((gp->axes.x == pairPtr->x) && (gp->axes.y == pairPtr->y)) {
      /*
       * Check if any of the y-values (because of stacking) are greater
       * than the current limits of the graph.
       */
      if (gp->sum < 0.0f) {
	if (*minPtr > gp->sum) {
	  *minPtr = gp->sum;
	}
      } else {
	if (*maxPtr < gp->sum) {
	  *maxPtr = gp->sum;
	}
      }
    }
  }
}

static void GetBarExtentsProc(Element *basePtr, Region2d *regPtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  Graph* graphPtr;
  double middle, barWidth;
  int nPoints;

  graphPtr = elemPtr->obj.graphPtr;
  regPtr->top = regPtr->left = DBL_MAX;
  regPtr->bottom = regPtr->right = -DBL_MAX;

  nPoints = NUMBEROFPOINTS(elemPtr);
  if (nPoints < 1) {
    return;				/* No data points */
  }
  barWidth = graphPtr->barWidth;
  if (elemPtr->barWidth > 0.0f)
    barWidth = elemPtr->barWidth;

  middle = 0.5;
  regPtr->left = elemPtr->x.min - middle;
  regPtr->right = elemPtr->x.max + middle;

  regPtr->top = elemPtr->y.min;
  regPtr->bottom = elemPtr->y.max;
  if (regPtr->bottom < graphPtr->baseline) {
    regPtr->bottom = graphPtr->baseline;
  }
  /*
   * Handle stacked bar elements specially.
   *
   * If element is stacked, the sum of its ordinates may be outside the
   * minimum/maximum limits of the element's data points.
   */
  if ((graphPtr->barMode == BARS_STACKED) && (graphPtr->nBarGroups > 0)) {
    CheckBarStacks(graphPtr, &elemPtr->axes, &regPtr->top, &regPtr->bottom);
  }
  /* Warning: You get what you deserve if the x-axis is logScale */
  if (elemPtr->axes.x->logScale) {
    regPtr->left = Blt_FindElemValuesMinimum(&elemPtr->x, DBL_MIN) + 
      middle;
  }
  /* Fix y-min limits for barchart */
  if (elemPtr->axes.y->logScale) {
    if ((regPtr->top <= 0.0) || (regPtr->top > 1.0)) {
      regPtr->top = 1.0;
    }
  } else {
    if (regPtr->top > 0.0) {
      regPtr->top = 0.0;
    }
  }
  /* Correct the extents for error bars if they exist. */
  if (elemPtr->xError.nValues > 0) {
    int i;
	
    /* Correct the data limits for error bars */
    nPoints = MIN(elemPtr->xError.nValues, nPoints);
    for (i = 0; i < nPoints; i++) {
      double x;

      x = elemPtr->x.values[i] + elemPtr->xError.values[i];
      if (x > regPtr->right) {
	regPtr->right = x;
      }
      x = elemPtr->x.values[i] - elemPtr->xError.values[i];
      if (elemPtr->axes.x->logScale) {
	if (x < 0.0) {
	  x = -x;		/* Mirror negative values, instead of
				 * ignoring them. */
	}
	if ((x > DBL_MIN) && (x < regPtr->left)) {
	  regPtr->left = x;
	}
      } else if (x < regPtr->left) {
	regPtr->left = x;
      }
    }		     
  } else {
    if ((elemPtr->xHigh.nValues > 0) && 
	(elemPtr->xHigh.max > regPtr->right)) {
      regPtr->right = elemPtr->xHigh.max;
    }
    if (elemPtr->xLow.nValues > 0) {
      double left;
	    
      if ((elemPtr->xLow.min <= 0.0) && 
	  (elemPtr->axes.x->logScale)) {
	left = Blt_FindElemValuesMinimum(&elemPtr->xLow, DBL_MIN);
      } else {
	left = elemPtr->xLow.min;
      }
      if (left < regPtr->left) {
	regPtr->left = left;
      }
    }
  }
  if (elemPtr->yError.nValues > 0) {
    int i;
	
    nPoints = MIN(elemPtr->yError.nValues, nPoints);
    for (i = 0; i < nPoints; i++) {
      double y;

      y = elemPtr->y.values[i] + elemPtr->yError.values[i];
      if (y > regPtr->bottom) {
	regPtr->bottom = y;
      }
      y = elemPtr->y.values[i] - elemPtr->yError.values[i];
      if (elemPtr->axes.y->logScale) {
	if (y < 0.0) {
	  y = -y;		/* Mirror negative values, instead of
				 * ignoring them. */
	}
	if ((y > DBL_MIN) && (y < regPtr->left)) {
	  regPtr->top = y;
	}
      } else if (y < regPtr->top) {
	regPtr->top = y;
      }
    }		     
  } else {
    if ((elemPtr->yHigh.nValues > 0) && 
	(elemPtr->yHigh.max > regPtr->bottom)) {
      regPtr->bottom = elemPtr->yHigh.max;
    }
    if (elemPtr->yLow.nValues > 0) {
      double top;
	    
      if ((elemPtr->yLow.min <= 0.0) && 
	  (elemPtr->axes.y->logScale)) {
	top = Blt_FindElemValuesMinimum(&elemPtr->yLow, DBL_MIN);
      } else {
	top = elemPtr->yLow.min;
      }
      if (top < regPtr->top) {
	regPtr->top = top;
      }
    }
  }
}

static void ClosestBarProc(Graph* graphPtr, Element *basePtr)
{
  ClosestSearch* searchPtr = &graphPtr->search;

  BarElement* elemPtr = (BarElement *)basePtr;
  XRectangle *bp;
  double minDist;
  int imin;
  int i;

  minDist = searchPtr->dist;
  imin = 0;
    
  for (bp = elemPtr->bars, i = 0; i < elemPtr->nBars; i++, bp++) {
    Point2d *pp, *pend;
    Point2d outline[5];
    double left, right, top, bottom;

    if (PointInRectangle(bp, searchPtr->x, searchPtr->y)) {
      imin = elemPtr->barToData[i];
      minDist = 0.0;
      break;
    }
    left = bp->x, top = bp->y;
    right = (double)(bp->x + bp->width);
    bottom = (double)(bp->y + bp->height);
    outline[4].x = outline[3].x = outline[0].x = left;
    outline[4].y = outline[1].y = outline[0].y = top;
    outline[2].x = outline[1].x = right;
    outline[3].y = outline[2].y = bottom;

    for (pp = outline, pend = outline + 4; pp < pend; pp++) {
      Point2d t;
      double dist;

      t = Blt_GetProjection(searchPtr->x, searchPtr->y, pp, pp + 1);
      if (t.x > right) {
	t.x = right;
      } else if (t.x < left) {
	t.x = left;
      }
      if (t.y > bottom) {
	t.y = bottom;
      } else if (t.y < top) {
	t.y = top;
      }
      dist = hypot((t.x - searchPtr->x), (t.y - searchPtr->y));
      if (dist < minDist) {
	minDist = dist;
	imin = elemPtr->barToData[i];
      }
    }
  }
  if (minDist < searchPtr->dist) {
    searchPtr->elemPtr = (Element *)elemPtr;
    searchPtr->dist = minDist;
    searchPtr->index = imin;
    searchPtr->point.x = (double)elemPtr->x.values[imin];
    searchPtr->point.y = (double)elemPtr->y.values[imin];
  }
}

static void MergePens(BarElement* elemPtr, BarStyle **dataToStyle)
{
  if (Blt_Chain_GetLength(elemPtr->stylePalette) < 2) {
    Blt_ChainLink link;
    BarStyle *stylePtr;

    link = Blt_Chain_FirstLink(elemPtr->stylePalette);

    stylePtr = Blt_Chain_GetValue(link);
    stylePtr->nBars = elemPtr->nBars;
    stylePtr->bars = elemPtr->bars;
    stylePtr->symbolSize = elemPtr->bars->width / 2;
    stylePtr->xeb.length = elemPtr->xeb.length;
    stylePtr->xeb.segments = elemPtr->xeb.segments;
    stylePtr->yeb.length = elemPtr->yeb.length;
    stylePtr->yeb.segments = elemPtr->yeb.segments;
    return;
  }
  /* We have more than one style. Group bar segments of like pen styles
   * together.  */

  if (elemPtr->nBars > 0) {
    Blt_ChainLink link;
    XRectangle *bars, *bp;
    int *ip, *barToData;

    bars = malloc(elemPtr->nBars * sizeof(XRectangle));
    barToData = malloc(elemPtr->nBars * sizeof(int));
    bp = bars, ip = barToData;
    for (link = Blt_Chain_FirstLink(elemPtr->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr;
      int i;

      stylePtr = Blt_Chain_GetValue(link);
      stylePtr->symbolSize = bp->width / 2;
      stylePtr->bars = bp;
      for (i = 0; i < elemPtr->nBars; i++) {
	int iData;

	iData = elemPtr->barToData[i];
	if (dataToStyle[iData] == stylePtr) {
	  *bp++ = elemPtr->bars[i];
	  *ip++ = iData;
	}
      }
      stylePtr->nBars = bp - stylePtr->bars;
    }
    free(elemPtr->bars);
    free(elemPtr->barToData);
    elemPtr->bars = bars;
    elemPtr->barToData = barToData;
  }

  if (elemPtr->xeb.length > 0) {
    Blt_ChainLink link;
    Segment2d *bars, *sp;
    int *map, *ip;

    bars = malloc(elemPtr->xeb.length * sizeof(Segment2d));
    map = malloc(elemPtr->xeb.length * sizeof(int));
    sp = bars, ip = map;
    for (link = Blt_Chain_FirstLink(elemPtr->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr;
      int i;

      stylePtr = Blt_Chain_GetValue(link);
      stylePtr->xeb.segments = sp;
      for (i = 0; i < elemPtr->xeb.length; i++) {
	int iData;

	iData = elemPtr->xeb.map[i];
	if (dataToStyle[iData] == stylePtr) {
	  *sp++ = elemPtr->xeb.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->xeb.length = sp - stylePtr->xeb.segments;
    }
    free(elemPtr->xeb.segments);
    elemPtr->xeb.segments = bars;
    free(elemPtr->xeb.map);
    elemPtr->xeb.map = map;
  }
  if (elemPtr->yeb.length > 0) {
    Blt_ChainLink link;
    Segment2d *bars, *sp;
    int *map, *ip;

    bars = malloc(elemPtr->yeb.length * sizeof(Segment2d));
    map = malloc(elemPtr->yeb.length * sizeof(int));
    sp = bars, ip = map;
    for (link = Blt_Chain_FirstLink(elemPtr->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr;
      int i;

      stylePtr = Blt_Chain_GetValue(link);
      stylePtr->yeb.segments = sp;
      for (i = 0; i < elemPtr->yeb.length; i++) {
	int iData;

	iData = elemPtr->yeb.map[i];
	if (dataToStyle[iData] == stylePtr) {
	  *sp++ = elemPtr->yeb.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->yeb.length = sp - stylePtr->yeb.segments;
    }
    free(elemPtr->yeb.segments);
    elemPtr->yeb.segments = bars;
    free(elemPtr->yeb.map);
    elemPtr->yeb.map = map;
  }
}

static void MapActiveBars(BarElement* elemPtr)
{
  if (elemPtr->activeRects) {
    free(elemPtr->activeRects);
    elemPtr->activeRects = NULL;
  }
  if (elemPtr->activeToData) {
    free(elemPtr->activeToData);
    elemPtr->activeToData = NULL;
  }
  elemPtr->nActive = 0;

  if (elemPtr->nActiveIndices > 0) {
    XRectangle *activeRects;
    int *activeToData;
    int i;
    int count;

    activeRects = malloc(sizeof(XRectangle) * 
			 elemPtr->nActiveIndices);
    activeToData = malloc(sizeof(int) * 
			  elemPtr->nActiveIndices);
    count = 0;
    for (i = 0; i < elemPtr->nBars; i++) {
      int *ip, *iend;

      for (ip = elemPtr->activeIndices, 
	     iend = ip + elemPtr->nActiveIndices; ip < iend; ip++) {
	if (elemPtr->barToData[i] == *ip) {
	  activeRects[count] = elemPtr->bars[i];
	  activeToData[count] = i;
	  count++;
	}
      }
    }
    elemPtr->nActive = count;
    elemPtr->activeRects = activeRects;
    elemPtr->activeToData = activeToData;
  }
  elemPtr->flags &= ~ACTIVE_PENDING;
}

static void ResetBar(BarElement* elemPtr)
{
  /* Release any storage associated with the display of the bar */
  ResetStylePalette(elemPtr->stylePalette);
  if (elemPtr->activeRects) {
    free(elemPtr->activeRects);
  }
  if (elemPtr->activeToData) {
    free(elemPtr->activeToData);
  }
  if (elemPtr->xeb.segments) {
    free(elemPtr->xeb.segments);
  }
  if (elemPtr->xeb.map) {
    free(elemPtr->xeb.map);
  }
  if (elemPtr->yeb.segments) {
    free(elemPtr->yeb.segments);
  }
  if (elemPtr->yeb.map) {
    free(elemPtr->yeb.map);
  }
  if (elemPtr->bars) {
    free(elemPtr->bars);
  }
  if (elemPtr->barToData) {
    free(elemPtr->barToData);
  }
  elemPtr->activeToData = elemPtr->xeb.map = elemPtr->yeb.map = 
    elemPtr->barToData = NULL;
  elemPtr->activeRects = elemPtr->bars = NULL;
  elemPtr->xeb.segments = elemPtr->yeb.segments = NULL;
  elemPtr->nActive = elemPtr->xeb.length = elemPtr->yeb.length = 
    elemPtr->nBars = 0;
}

static void MapErrorBars(Graph* graphPtr, BarElement* elemPtr, 
			 BarStyle **dataToStyle)
{
  int n, nPoints;
  Region2d reg;

  Blt_GraphExtents(graphPtr, &reg);
  nPoints = NUMBEROFPOINTS(elemPtr);
  if (elemPtr->xError.nValues > 0) {
    n = MIN(elemPtr->xError.nValues, nPoints);
  } else {
    n = MIN3(elemPtr->xHigh.nValues, elemPtr->xLow.nValues, nPoints);
  }
  if (n > 0) {
    Segment2d *bars;
    Segment2d *segPtr;
    int *map;
    int *indexPtr;
    int i;
		
    segPtr = bars = malloc(n * 3 * sizeof(Segment2d));
    indexPtr = map = malloc(n * 3 * sizeof(int));
    for (i = 0; i < n; i++) {
      double x, y;
      double high, low;
      BarStyle *stylePtr;

      x = elemPtr->x.values[i];
      y = elemPtr->y.values[i];
      stylePtr = dataToStyle[i];
      if ((isfinite(x)) && (isfinite(y))) {
	if (elemPtr->xError.nValues > 0) {
	  high = x + elemPtr->xError.values[i];
	  low = x - elemPtr->xError.values[i];
	} else {
	  high = elemPtr->xHigh.values[i];
	  low = elemPtr->xLow.values[i];
	}
	if ((isfinite(high)) && (isfinite(low)))  {
	  Point2d p, q;

	  p = Blt_Map2D(graphPtr, high, y, &elemPtr->axes);
	  q = Blt_Map2D(graphPtr, low, y, &elemPtr->axes);
	  segPtr->p = p;
	  segPtr->q = q;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Left cap */
	  segPtr->p.x = segPtr->q.x = p.x;
	  segPtr->p.y = p.y - stylePtr->errorBarCapWidth;
	  segPtr->q.y = p.y + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Right cap */
	  segPtr->p.x = segPtr->q.x = q.x;
	  segPtr->p.y = q.y - stylePtr->errorBarCapWidth;
	  segPtr->q.y = q.y + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	}
      }
    }
    elemPtr->xeb.segments = bars;
    elemPtr->xeb.length = segPtr - bars;
    elemPtr->xeb.map = map;
  }
  if (elemPtr->yError.nValues > 0) {
    n = MIN(elemPtr->yError.nValues, nPoints);
  } else {
    n = MIN3(elemPtr->yHigh.nValues, elemPtr->yLow.nValues, nPoints);
  }
  if (n > 0) {
    Segment2d *bars;
    Segment2d *segPtr;
    int *map;
    int *indexPtr;
    int i;
		
    segPtr = bars = malloc(n * 3 * sizeof(Segment2d));
    indexPtr = map = malloc(n * 3 * sizeof(int));
    for (i = 0; i < n; i++) {
      double x, y;
      double high, low;
      BarStyle *stylePtr;

      x = elemPtr->x.values[i];
      y = elemPtr->y.values[i];
      stylePtr = dataToStyle[i];
      if ((isfinite(x)) && (isfinite(y))) {
	if (elemPtr->yError.nValues > 0) {
	  high = y + elemPtr->yError.values[i];
	  low = y - elemPtr->yError.values[i];
	} else {
	  high = elemPtr->yHigh.values[i];
	  low = elemPtr->yLow.values[i];
	}
	if ((isfinite(high)) && (isfinite(low)))  {
	  Point2d p, q;
		    
	  p = Blt_Map2D(graphPtr, x, high, &elemPtr->axes);
	  q = Blt_Map2D(graphPtr, x, low, &elemPtr->axes);
	  segPtr->p = p;
	  segPtr->q = q;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Top cap. */
	  segPtr->p.y = segPtr->q.y = p.y;
	  segPtr->p.x = p.x - stylePtr->errorBarCapWidth;
	  segPtr->q.x = p.x + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Bottom cap. */
	  segPtr->p.y = segPtr->q.y = q.y;
	  segPtr->p.x = q.x - stylePtr->errorBarCapWidth;
	  segPtr->q.x = q.x + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	}
      }
    }
    elemPtr->yeb.segments = bars;
    elemPtr->yeb.length = segPtr - bars;
    elemPtr->yeb.map = map;
  }
}

static void MapBarProc(Graph* graphPtr, Element *basePtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  BarStyle **dataToStyle;
  double *x, *y;
  double barWidth, barOffset;
  double baseline, ybot;
  int *barToData;			/* Maps bars to data point indices */
  int invertBar;
  int nPoints, count;
  XRectangle *rp, *bars;
  int i;
  int size;

  ResetBar(elemPtr);
  nPoints = NUMBEROFPOINTS(elemPtr);
  if (nPoints < 1)
    return;				/* No data points */

  barWidth = graphPtr->barWidth;
  barWidth = (elemPtr->barWidth > 0.0f) ? elemPtr->barWidth:graphPtr->barWidth;
  baseline = (elemPtr->axes.y->logScale) ? 0.0 : graphPtr->baseline;
  barOffset = barWidth * 0.5;

  /*
   * Create an array of bars representing the screen coordinates of all the
   * segments in the bar.
   */
  bars = calloc(nPoints, sizeof(XRectangle));
  barToData = calloc(nPoints, sizeof(int));

  x = elemPtr->x.values, y = elemPtr->y.values;
  count = 0;
  for (rp = bars, i = 0; i < nPoints; i++) {
    Point2d c1, c2;			/* Two opposite corners of the rectangle
					 * in graph coordinates. */
    double dx, dy;
    int height;
    double right, left, top, bottom;

    if (((x[i] - barWidth) > elemPtr->axes.x->axisRange.max) ||
	((x[i] + barWidth) < elemPtr->axes.x->axisRange.min)) {
      continue;			/* Abscissa is out of range of the
				 * x-axis */
    }
    c1.x = x[i] - barOffset;
    c1.y = y[i];
    c2.x = c1.x + barWidth;
    c2.y = baseline;

    /*
     * If the mode is "aligned" or "stacked" we need to adjust the x or y
     * coordinates of the two corners.
     */

    if ((graphPtr->nBarGroups > 0) && (graphPtr->barMode != BARS_INFRONT) && 
	(!graphPtr->stackAxes)) {
      Tcl_HashEntry *hPtr;
      BarSetKey key;

      key.value = (float)x[i];
      key.axes = elemPtr->axes;
      key.axes.y = NULL;
      hPtr = Tcl_FindHashEntry(&graphPtr->setTable, (char *)&key);
      if (hPtr) {
	Tcl_HashTable *tablePtr;
	const char *name;

	tablePtr = Tcl_GetHashValue(hPtr);
	name = (elemPtr->groupName) ? elemPtr->groupName : 
	  elemPtr->axes.y->obj.name;
	hPtr = Tcl_FindHashEntry(tablePtr, name);
	if (hPtr) {
	  BarGroup *groupPtr;
	  double slice, width, offset;
		    
	  groupPtr = Tcl_GetHashValue(hPtr);
	  slice = barWidth / (double)graphPtr->maxBarSetSize;
	  offset = (slice * groupPtr->index);
	  if (graphPtr->maxBarSetSize > 1) {
	    offset += slice * 0.05;
	    slice *= 0.90;
	  }
	  switch (graphPtr->barMode) {
	  case BARS_STACKED:
	    groupPtr->count++;
	    c2.y = groupPtr->lastY;
	    c1.y += c2.y;
	    groupPtr->lastY = c1.y;
	    c1.x += offset;
	    c2.x = c1.x + slice;
	    break;
			
	  case BARS_ALIGNED:
	    slice /= groupPtr->nSegments;
	    c1.x += offset + (slice * groupPtr->count);
	    c2.x = c1.x + slice;
	    groupPtr->count++;
	    break;
			
	  case BARS_OVERLAP:
	    slice /= (groupPtr->nSegments + 1);
	    width = slice + slice;
	    groupPtr->count++;
	    c1.x += offset + 
	      (slice * (groupPtr->nSegments - groupPtr->count));
	    c2.x = c1.x + width;
	    break;
			
	  case BARS_INFRONT:
	    break;
	  }
	}
      }
    }
    invertBar = FALSE;
    if (c1.y < c2.y) {
      double temp;

      /* Handle negative bar values by swapping ordinates */
      temp = c1.y, c1.y = c2.y, c2.y = temp;
      invertBar = TRUE;
    }
    /*
     * Get the two corners of the bar segment and compute the rectangle
     */
    ybot = c2.y;
    c1 = Blt_Map2D(graphPtr, c1.x, c1.y, &elemPtr->axes);
    c2 = Blt_Map2D(graphPtr, c2.x, c2.y, &elemPtr->axes);
    if ((ybot == 0.0) && (elemPtr->axes.y->logScale)) {
      c2.y = graphPtr->bottom;
    }
	    
    if (c2.y < c1.y) {
      double t;
      t = c1.y, c1.y = c2.y, c2.y = t;
    }
    if (c2.x < c1.x) {
      double t;
      t = c1.x, c1.x = c2.x, c2.x = t;
    }
    if ((c1.x > graphPtr->right) || (c2.x < graphPtr->left) || 
	(c1.y > graphPtr->bottom) || (c2.y < graphPtr->top)) {
      continue;
    }
    /* Bound the bars horizontally by the width of the graph window */
    /* Bound the bars vertically by the position of the axis. */
    if (graphPtr->stackAxes) {
      top = elemPtr->axes.y->screenMin;
      bottom = elemPtr->axes.y->screenMin + elemPtr->axes.y->screenRange;
      left = graphPtr->left;
      right = graphPtr->right;
    } else {
      left = top = 0;
      bottom = right = 10000;
      /* Shouldn't really have a call to Tk_Width or Tk_Height in
       * mapping routine.  We only want to clamp the bar segment to the
       * size of the window if we're actually mapped onscreen. */
      if (Tk_Height(graphPtr->tkwin) > 1) {
	bottom = Tk_Height(graphPtr->tkwin);
      }
      if (Tk_Width(graphPtr->tkwin) > 1) {
	right = Tk_Width(graphPtr->tkwin);
      }
    }
    CLAMP(c1.y, top, bottom);
    CLAMP(c2.y, top, bottom);
    CLAMP(c1.x, left, right);
    CLAMP(c2.x, left, right);
    dx = fabs(c1.x - c2.x);
    dy = fabs(c1.y - c2.y);
    if ((dx == 0) || (dy == 0)) {
      continue;
    }
    height = (int)dy;
    if (invertBar) {
      rp->y = (short int)MIN(c1.y, c2.y);
    } else {
      rp->y = (short int)(MAX(c1.y, c2.y)) - height;
    }
    rp->x = (short int)MIN(c1.x, c2.x);
    rp->width = (short int)dx + 1;
    rp->width |= 0x1;
    if (rp->width < 1) {
      rp->width = 1;
    }
    rp->height = height + 1;
    if (rp->height < 1) {
      rp->height = 1;
    }
    barToData[count] = i;		/* Save the data index corresponding to
					 * the rectangle */
    count++;
    rp++;
  }
  elemPtr->nBars = count;
  elemPtr->bars = bars;
  elemPtr->barToData = barToData;
  if (elemPtr->nActiveIndices > 0) {
    MapActiveBars(elemPtr);
  }
	
  size = 20;
  if (count > 0) {
    size = bars->width;
  }
  {
    Blt_ChainLink link;

    /* Set the symbol size of all the pen styles. */
    for (link = Blt_Chain_FirstLink(elemPtr->stylePalette); link;
	 link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr;
	    
      stylePtr = Blt_Chain_GetValue(link);
      stylePtr->symbolSize = size;
      stylePtr->errorBarCapWidth = 
	(stylePtr->penPtr->errorBarCapWidth > 0) 
	? stylePtr->penPtr->errorBarCapWidth : (size * 66666) / 100000;
      stylePtr->errorBarCapWidth /= 2;
    }
  }
  dataToStyle = (BarStyle **)Blt_StyleMap((Element *)elemPtr);
  if (((elemPtr->yHigh.nValues > 0) && (elemPtr->yLow.nValues > 0)) ||
      ((elemPtr->xHigh.nValues > 0) && (elemPtr->xLow.nValues > 0)) ||
      (elemPtr->xError.nValues > 0) || (elemPtr->yError.nValues > 0)) {
    MapErrorBars(graphPtr, elemPtr, dataToStyle);
  }
  MergePens(elemPtr, dataToStyle);
  free(dataToStyle);
}

static void DrawSymbolProc(Graph* graphPtr, Drawable drawable, 
			   Element *basePtr, int x, int y, int size)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  BarPen* penPtr;
  int radius;

  penPtr = NORMALPEN(elemPtr);
  if (!penPtr->fill && !penPtr->outlineColor)
    return;

  radius = (size / 2);
  size--;

  x -= radius;
  y -= radius;
  if (penPtr->fillGC)
    XSetTSOrigin(graphPtr->display, penPtr->fillGC, x, y);

  if (penPtr->stipple != None)
    XFillRectangle(graphPtr->display, drawable, penPtr->fillGC, x, y, 
		   size, size);
  else
    Tk_Fill3DRectangle(graphPtr->tkwin, drawable, penPtr->fill, 
		       x, y, size, size, penPtr->borderWidth, penPtr->relief);

  XDrawRectangle(graphPtr->display, drawable, penPtr->outlineGC, x, y, 
		 size, size);
  if (penPtr->fillGC)
    XSetTSOrigin(graphPtr->display, penPtr->fillGC, 0, 0);
}

static void SetBackgroundClipRegion(Tk_Window tkwin, Tk_3DBorder border, 
				    TkRegion rgn)
{
  Display *display;
  GC gc;

  display = Tk_Display(tkwin);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
  TkSetRegion(display, gc, rgn);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
  TkSetRegion(display, gc, rgn);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
  TkSetRegion(display, gc, rgn);
}

static void UnsetBackgroundClipRegion(Tk_Window tkwin, Tk_3DBorder border)
{
  Display *display;
  GC gc;

  display = Tk_Display(tkwin);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
  XSetClipMask(display, gc, None);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
  XSetClipMask(display, gc, None);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
  XSetClipMask(display, gc, None);
}

static void DrawBarSegments(Graph* graphPtr, Drawable drawable, BarPen* penPtr,
			    XRectangle *bars, int nBars)
{
  TkRegion rgn;

  {
    XRectangle clip;
    clip.x = graphPtr->left;
    clip.y = graphPtr->top;
    clip.width  = graphPtr->right - graphPtr->left + 1;
    clip.height = graphPtr->bottom - graphPtr->top + 1;
    rgn = TkCreateRegion();
    TkUnionRectWithRegion(&clip, rgn, rgn);
  }

  if (penPtr->fill) {
    int relief = (penPtr->relief == TK_RELIEF_SOLID) ? 
      TK_RELIEF_FLAT: penPtr->relief;

    int hasOutline = ((relief == TK_RELIEF_FLAT) && penPtr->outlineColor);
    if (penPtr->stipple != None)
      TkSetRegion(graphPtr->display, penPtr->fillGC, rgn);

    SetBackgroundClipRegion(graphPtr->tkwin, penPtr->fill, rgn);

    if (hasOutline)
      TkSetRegion(graphPtr->display, penPtr->outlineGC, rgn);

    XRectangle *rp, *rend;
    for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
      if (penPtr->stipple != None)
	XFillRectangle(graphPtr->display, drawable, penPtr->fillGC, 
		       rp->x, rp->y, rp->width, rp->height);
      else
	Tk_Fill3DRectangle(graphPtr->tkwin, drawable, 
			   penPtr->fill, rp->x, rp->y, rp->width, rp->height, 
			   penPtr->borderWidth, relief);

      if (hasOutline)
	XDrawRectangle(graphPtr->display, drawable, penPtr->outlineGC, 
		       rp->x, rp->y, rp->width, rp->height);
    }

    UnsetBackgroundClipRegion(graphPtr->tkwin, penPtr->fill);

    if (hasOutline)
      XSetClipMask(graphPtr->display, penPtr->outlineGC, None);

    if (penPtr->stipple != None)
      XSetClipMask(graphPtr->display, penPtr->fillGC, None);

  }
  else if (penPtr->outlineColor) {
    TkSetRegion(graphPtr->display, penPtr->outlineGC, rgn);
    XDrawRectangles(graphPtr->display, drawable, penPtr->outlineGC, bars, 
		    nBars);
    XSetClipMask(graphPtr->display, penPtr->outlineGC, None);
  }

  TkDestroyRegion(rgn);
}

static void DrawBarValues(Graph* graphPtr, Drawable drawable, 
			  BarElement* elemPtr,
			  BarPen* penPtr, XRectangle *bars, int nBars, 
			  int *barToData)
{
  XRectangle *rp, *rend;
  int count;
  const char *fmt;
    
  fmt = penPtr->valueFormat;
  if (!fmt) {
    fmt = "%g";
  }
  count = 0;
  for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
    Point2d anchorPos;
    double x, y;
    char string[TCL_DOUBLE_SPACE * 2 + 2];

    x = elemPtr->x.values[barToData[count]];
    y = elemPtr->y.values[barToData[count]];

    count++;
    if (penPtr->valueShow == SHOW_X) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x); 
    } else if (penPtr->valueShow == SHOW_Y) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, y); 
    } else if (penPtr->valueShow == SHOW_BOTH) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x);
      strcat(string, ",");
      sprintf_s(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
    }
    if (graphPtr->inverted) {
      anchorPos.y = rp->y + rp->height * 0.5;
      anchorPos.x = rp->x + rp->width;
      if (x < graphPtr->baseline) {
	anchorPos.x -= rp->width;
      } 
    } else {
      anchorPos.x = rp->x + rp->width * 0.5;
      anchorPos.y = rp->y;
      if (y < graphPtr->baseline) {			
	anchorPos.y += rp->height;
      }
    }
    Blt_DrawText(graphPtr->tkwin, drawable, string, &penPtr->valueStyle, 
		 (int)anchorPos.x, (int)anchorPos.y);
  }
}

static void DrawNormalBarProc(Graph* graphPtr, Drawable drawable, 
			      Element *basePtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  int count;
  Blt_ChainLink link;

  count = 0;
  for (link = Blt_Chain_FirstLink(elemPtr->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr;
    BarPen* penPtr;

    stylePtr = Blt_Chain_GetValue(link);
    penPtr = stylePtr->penPtr;
    if (stylePtr->nBars > 0) {
      DrawBarSegments(graphPtr, drawable, penPtr, stylePtr->bars,
		      stylePtr->nBars);
    }
    if ((stylePtr->xeb.length > 0) && (penPtr->errorBarShow & SHOW_X)) {
      Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->errorBarGC, 
			 stylePtr->xeb.segments, stylePtr->xeb.length);
    }
    if ((stylePtr->yeb.length > 0) && (penPtr->errorBarShow & SHOW_Y)) {
      Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->errorBarGC, 
			 stylePtr->yeb.segments, stylePtr->yeb.length);
    }
    if (penPtr->valueShow != SHOW_NONE) {
      DrawBarValues(graphPtr, drawable, elemPtr, penPtr, 
		    stylePtr->bars, stylePtr->nBars, 
		    elemPtr->barToData + count);
    }
    count += stylePtr->nBars;
  }
}

static void DrawActiveBarProc(Graph* graphPtr, Drawable drawable, 
			      Element *basePtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;

  if (elemPtr->activePenPtr) {
    BarPen* penPtr = elemPtr->activePenPtr;

    if (elemPtr->nActiveIndices > 0) {
      if (elemPtr->flags & ACTIVE_PENDING) {
	MapActiveBars(elemPtr);
      }
      DrawBarSegments(graphPtr, drawable, penPtr, elemPtr->activeRects, 
		      elemPtr->nActive);
      if (penPtr->valueShow != SHOW_NONE) {
	DrawBarValues(graphPtr, drawable, elemPtr, penPtr, 
		      elemPtr->activeRects, elemPtr->nActive, 
		      elemPtr->activeToData);
      }
    } else if (elemPtr->nActiveIndices < 0) {
      DrawBarSegments(graphPtr, drawable, penPtr, elemPtr->bars, 
		      elemPtr->nBars);
      if (penPtr->valueShow != SHOW_NONE) {
	DrawBarValues(graphPtr, drawable, elemPtr, penPtr, 
		      elemPtr->bars, elemPtr->nBars, elemPtr->barToData);
      }
    }
  }
}

static void SymbolToPostScriptProc(Graph* graphPtr, Blt_Ps ps, Element *basePtr,
				   double x, double y, int size)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  BarPen* penPtr;

  penPtr = NORMALPEN(elemPtr);
  if (!penPtr->fill && !penPtr->outlineColor)
    return;

  /*
   * Build a PostScript procedure to draw the fill and outline of the symbol
   * after the path of the symbol shape has been formed
   */
  Blt_Ps_Append(ps, "\n"
		"/DrawSymbolProc {\n"
		"gsave\n    ");
  if (penPtr->stipple != None) {
    if (penPtr->fill) {
      Blt_Ps_XSetBackground(ps, Tk_3DBorderColor(penPtr->fill));
      Blt_Ps_Append(ps, "    gsave fill grestore\n    ");
    }
    if (penPtr->outlineColor) {
      Blt_Ps_XSetForeground(ps, penPtr->outlineColor);
    } else {
      Blt_Ps_XSetForeground(ps, Tk_3DBorderColor(penPtr->fill));
    }
    Blt_Ps_XSetStipple(ps, graphPtr->display, penPtr->stipple);
  } else if (penPtr->outlineColor) {
    Blt_Ps_XSetForeground(ps, penPtr->outlineColor);
    Blt_Ps_Append(ps, "    fill\n");
  }
  Blt_Ps_Append(ps, "  grestore\n");
  Blt_Ps_Append(ps, "} def\n\n");
  Blt_Ps_Format(ps, "%g %g %d Sq\n", x, y, size);
}

static void SegmentsToPostScript(Graph* graphPtr, Blt_Ps ps, BarPen* penPtr, 
				 XRectangle *bars, int nBars)
{
  XRectangle *rp, *rend;

  if (!penPtr->fill && !penPtr->outlineColor)
    return;

  for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
    if ((rp->width < 1) || (rp->height < 1)) {
      continue;
    }
    if (penPtr->stipple != None) {
      Blt_Ps_Rectangle(ps, rp->x, rp->y, rp->width - 1, rp->height - 1);
      if (penPtr->fill) {
	Blt_Ps_XSetBackground(ps, Tk_3DBorderColor(penPtr->fill));
	Blt_Ps_Append(ps, "gsave fill grestore\n");
      }
      if (penPtr->outlineColor) {
	Blt_Ps_XSetForeground(ps, penPtr->outlineColor);
      } else {
	Blt_Ps_XSetForeground(ps, Tk_3DBorderColor(penPtr->fill));
      }
      Blt_Ps_XSetStipple(ps, graphPtr->display, penPtr->stipple);
    } else if (penPtr->outlineColor) {
      Blt_Ps_XSetForeground(ps, penPtr->outlineColor);
      Blt_Ps_XFillRectangle(ps, (double)rp->x, (double)rp->y, 
			    (int)rp->width - 1, (int)rp->height - 1);
    }
    if ((penPtr->fill) && (penPtr->borderWidth > 0) && 
	(penPtr->relief != TK_RELIEF_FLAT)) {
      Blt_Ps_Draw3DRectangle(ps, penPtr->fill, (double)rp->x, (double)rp->y,
			     (int)rp->width, (int)rp->height,
			     penPtr->borderWidth, penPtr->relief);
    }
  }
}

static void BarValuesToPostScript(Graph* graphPtr, Blt_Ps ps, 
				  BarElement* elemPtr,
				  BarPen* penPtr, XRectangle *bars, int nBars, 
				  int *barToData)
{
  XRectangle *rp, *rend;
  int count;
  const char *fmt;
  char string[TCL_DOUBLE_SPACE * 2 + 2];
  double x, y;
  Point2d anchorPos;
    
  count = 0;
  fmt = penPtr->valueFormat;
  if (!fmt)
    fmt = "%g";

  for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
    x = elemPtr->x.values[barToData[count]];
    y = elemPtr->y.values[barToData[count]];
    count++;
    if (penPtr->valueShow == SHOW_X) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x); 
    } else if (penPtr->valueShow == SHOW_Y) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, y); 
    } else if (penPtr->valueShow == SHOW_BOTH) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x);
      strcat(string, ",");
      sprintf_s(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
    }
    if (graphPtr->inverted) {
      anchorPos.y = rp->y + rp->height * 0.5;
      anchorPos.x = rp->x + rp->width;
      if (x < graphPtr->baseline) {
	anchorPos.x -= rp->width;
      } 
    } else {
      anchorPos.x = rp->x + rp->width * 0.5;
      anchorPos.y = rp->y;
      if (y < graphPtr->baseline) {			
	anchorPos.y += rp->height;
      }
    }
    Blt_Ps_DrawText(ps, string, &penPtr->valueStyle, anchorPos.x, 
		    anchorPos.y);
  }
}

static void ActiveBarToPostScriptProc(Graph* graphPtr, Blt_Ps ps, 
				      Element *basePtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;

  if (elemPtr->activePenPtr) {
    BarPen* penPtr = elemPtr->activePenPtr;
	
    if (elemPtr->nActiveIndices > 0) {
      if (elemPtr->flags & ACTIVE_PENDING) {
	MapActiveBars(elemPtr);
      }
      SegmentsToPostScript(graphPtr, ps, penPtr, elemPtr->activeRects,
			   elemPtr->nActive);
      if (penPtr->valueShow != SHOW_NONE) {
	BarValuesToPostScript(graphPtr, ps, elemPtr, penPtr, 
			      elemPtr->activeRects, elemPtr->nActive, elemPtr->activeToData);
      }
    } else if (elemPtr->nActiveIndices < 0) {
      SegmentsToPostScript(graphPtr, ps, penPtr, elemPtr->bars, 
			   elemPtr->nBars);
      if (penPtr->valueShow != SHOW_NONE) {
	BarValuesToPostScript(graphPtr, ps, elemPtr, penPtr, 
			      elemPtr->bars, elemPtr->nBars, elemPtr->barToData);
      }
    }
  }
}

static void NormalBarToPostScriptProc(Graph* graphPtr, Blt_Ps ps, 
				      Element *basePtr)
{
  BarElement* elemPtr = (BarElement *)basePtr;
  Blt_ChainLink link;
  int count;

  count = 0;
  for (link = Blt_Chain_FirstLink(elemPtr->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr;
    BarPen* penPtr;
    XColor* colorPtr;

    stylePtr = Blt_Chain_GetValue(link);
    penPtr = stylePtr->penPtr;
    if (stylePtr->nBars > 0) {
      SegmentsToPostScript(graphPtr, ps, penPtr, stylePtr->bars, 
			   stylePtr->nBars);
    }
    colorPtr = penPtr->errorBarColor;
    if (!colorPtr)
      colorPtr = penPtr->outlineColor;

    if ((stylePtr->xeb.length > 0) && (penPtr->errorBarShow & SHOW_X)) {
      Blt_Ps_XSetLineAttributes(ps, colorPtr, penPtr->errorBarLineWidth, 
				NULL, CapButt, JoinMiter);
      Blt_Ps_Draw2DSegments(ps, stylePtr->xeb.segments,
			    stylePtr->xeb.length);
    }
    if ((stylePtr->yeb.length > 0) && (penPtr->errorBarShow & SHOW_Y)) {
      Blt_Ps_XSetLineAttributes(ps, colorPtr, penPtr->errorBarLineWidth, 
				NULL, CapButt, JoinMiter);
      Blt_Ps_Draw2DSegments(ps, stylePtr->yeb.segments, 
			    stylePtr->yeb.length);
    }
    if (penPtr->valueShow != SHOW_NONE) {
      BarValuesToPostScript(graphPtr, ps, elemPtr, penPtr, 
			    stylePtr->bars, stylePtr->nBars, 
			    elemPtr->barToData + count);
    }
    count += stylePtr->nBars;
  }
}

void Blt_InitBarSetTable(Graph* graphPtr)
{
  Blt_ChainLink link;
  int nStacks, nSegs;
  Tcl_HashTable setTable;
  int sum, max;
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch iter;

  /*
   * Free resources associated with a previous frequency table. This includes
   * the array of frequency information and the table itself
   */
  Blt_DestroyBarSets(graphPtr);
  if (graphPtr->barMode == BARS_INFRONT) {
    return;				/* No set table is needed for
					 * "infront" mode */
  }
  Tcl_InitHashTable(&graphPtr->setTable, sizeof(BarSetKey) / sizeof(int));

  /*
   * Initialize a hash table and fill it with unique abscissas.  Keep track
   * of the frequency of each x-coordinate and how many abscissas have
   * duplicate mappings.
   */
  Tcl_InitHashTable(&setTable, sizeof(BarSetKey) / sizeof(int));
  nSegs = nStacks = 0;
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
       link; link = Blt_Chain_NextLink(link)) {
    BarElement* elemPtr;
    double *x, *xend;
    int nPoints;

    elemPtr = Blt_Chain_GetValue(link);
    if ((elemPtr->hide) || (elemPtr->obj.classId != CID_ELEM_BAR)) {
      continue;
    }
    nSegs++;
    nPoints = NUMBEROFPOINTS(elemPtr);
    for (x = elemPtr->x.values, xend = x + nPoints; x < xend; x++) {
      Tcl_HashEntry *hPtr;
      Tcl_HashTable *tablePtr;
      BarSetKey key;
      int isNew;
      size_t count;
      const char *name;

      key.value = *x;
      key.axes = elemPtr->axes;
      key.axes.y = NULL;
      hPtr = Tcl_CreateHashEntry(&setTable, (char *)&key, &isNew);
      if (isNew) {
	tablePtr = malloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(tablePtr, TCL_STRING_KEYS);
	Tcl_SetHashValue(hPtr, tablePtr);
      } else {
	tablePtr = Tcl_GetHashValue(hPtr);
      }
      name = (elemPtr->groupName) ? elemPtr->groupName : 
	elemPtr->axes.y->obj.name;
      hPtr = Tcl_CreateHashEntry(tablePtr, name, &isNew);
      if (isNew) {
	count = 1;
      } else {
	count = (size_t)Tcl_GetHashValue(hPtr);
	count++;
      }		
      Tcl_SetHashValue(hPtr, (ClientData)count);
    }
  }
  if (setTable.numEntries == 0) {
    return;				/* No bar elements to be displayed */
  }
  sum = max = 0;
  for (hPtr = Tcl_FirstHashEntry(&setTable, &iter); hPtr;
       hPtr = Tcl_NextHashEntry(&iter)) {
    Tcl_HashTable *tablePtr;
    Tcl_HashEntry *hPtr2;
    BarSetKey *keyPtr;
    int isNew;

    keyPtr = (BarSetKey *)Tcl_GetHashKey(&setTable, hPtr);
    hPtr2 = Tcl_CreateHashEntry(&graphPtr->setTable, (char *)keyPtr,&isNew);
    tablePtr = Tcl_GetHashValue(hPtr);
    Tcl_SetHashValue(hPtr2, tablePtr);
    if (max < tablePtr->numEntries) {
      max = tablePtr->numEntries;	/* # of stacks in group. */
    }
    sum += tablePtr->numEntries;
  }
  Tcl_DeleteHashTable(&setTable);
  if (sum > 0) {
    BarGroup *groupPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    graphPtr->barGroups = calloc(sum, sizeof(BarGroup));
    groupPtr = graphPtr->barGroups;
    for (hPtr = Tcl_FirstHashEntry(&graphPtr->setTable, &iter); 
	 hPtr; hPtr = Tcl_NextHashEntry(&iter)) {
      BarSetKey *keyPtr;
      Tcl_HashTable *tablePtr;
      Tcl_HashEntry *hPtr2;
      Tcl_HashSearch iter2;
      size_t xcount;

      tablePtr = Tcl_GetHashValue(hPtr);
      keyPtr = (BarSetKey *)Tcl_GetHashKey(&setTable, hPtr);
      xcount = 0;
      for (hPtr2 = Tcl_FirstHashEntry(tablePtr, &iter2); hPtr2!=NULL;
	   hPtr2 = Tcl_NextHashEntry(&iter2)) {
	size_t count;

	count = (size_t)Tcl_GetHashValue(hPtr2);
	groupPtr->nSegments = count;
	groupPtr->axes = keyPtr->axes;
	Tcl_SetHashValue(hPtr2, groupPtr);
	groupPtr->index = xcount++;
	groupPtr++;
      }
    }
  }
  graphPtr->maxBarSetSize = max;
  graphPtr->nBarGroups = sum;
}

void Blt_ComputeBarStacks(Graph* graphPtr)
{
  Blt_ChainLink link;
  if ((graphPtr->barMode != BARS_STACKED) || (graphPtr->nBarGroups == 0)) {
    return;
  }

  /* Initialize the stack sums to zero. */
  {
    BarGroup *gp, *gend;

    for (gp = graphPtr->barGroups, gend = gp + graphPtr->nBarGroups; 
	 gp < gend; gp++) {
      gp->sum = 0.0;
    }
  }

  /* Consider each bar x-y coordinate. Add the ordinates of duplicate
   * abscissas. */

  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    BarElement* elemPtr;
    double *x, *y, *xend;

    elemPtr = Blt_Chain_GetValue(link);
    if ((elemPtr->hide) || (elemPtr->obj.classId != CID_ELEM_BAR)) {
      continue;
    }
    for (x = elemPtr->x.values, y = elemPtr->y.values, 
	   xend = x + NUMBEROFPOINTS(elemPtr); x < xend; x++, y++) {
      BarSetKey key;
      BarGroup *groupPtr;
      Tcl_HashEntry *hPtr;
      Tcl_HashTable *tablePtr;
      const char *name;

      key.value = *x;
      key.axes = elemPtr->axes;
      key.axes.y = NULL;
      hPtr = Tcl_FindHashEntry(&graphPtr->setTable, (char *)&key);
      if (!hPtr)
	continue;

      tablePtr = Tcl_GetHashValue(hPtr);
      name = (elemPtr->groupName) ? elemPtr->groupName : 
	elemPtr->axes.y->obj.name;
      hPtr = Tcl_FindHashEntry(tablePtr, name);
      if (!hPtr)
	continue;

      groupPtr = Tcl_GetHashValue(hPtr);
      groupPtr->sum += *y;
    }
  }
}

void Blt_ResetBarGroups(Graph* graphPtr)
{
  BarGroup* gp;
  BarGroup* gend;
  for (gp = graphPtr->barGroups, gend = gp + graphPtr->nBarGroups; gp < gend; 
       gp++) {
    gp->lastY = 0.0;
    gp->count = 0;
  }
}

void Blt_DestroyBarSets(Graph* graphPtr)
{
  if (graphPtr->barGroups) {
    free(graphPtr->barGroups);
    graphPtr->barGroups = NULL;
  }

  graphPtr->nBarGroups = 0;
  Tcl_HashSearch iter;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->setTable, &iter); 
       hPtr; hPtr = Tcl_NextHashEntry(&iter)) {
    Tcl_HashTable* tablePtr = Tcl_GetHashValue(hPtr);
    Tcl_DeleteHashTable(tablePtr);
    free(tablePtr);
  }
  Tcl_DeleteHashTable(&graphPtr->setTable);
  Tcl_InitHashTable(&graphPtr->setTable, sizeof(BarSetKey) / sizeof(int));
}
