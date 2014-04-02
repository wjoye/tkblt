/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
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

#include "bltMath.h"

#include "bltGrElemBar.h"
#include "bltGrElemOption.h"
#include "bltGrPenOp.h"

#define CLAMP(x,l,h)	((x) = (((x)<(l))? (l) : ((x)>(h)) ? (h) : (x)))

#define PointInRectangle(r,x0,y0)					\
  (((x0) <= (int)((r)->x + (r)->width - 1)) && ((x0) >= (int)(r)->x) && \
   ((y0) <= (int)((r)->y + (r)->height - 1)) && ((y0) >= (int)(r)->y))

typedef struct {
  float x1, y1, x2, y2;
} BarRegion;

typedef struct {
  Weight weight;
  BarPen* penPtr;
  XRectangle *bars;
  int nBars;
  GraphSegments xeb;
  GraphSegments yeb;
  int symbolSize;
  int errorBarCapWidth;
} BarStyle;

// Defs

static void ResetBar(BarElement* elemPtr);

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
    "style", StyleSetProc, StyleGetProc, StyleRestoreProc, StyleFreeProc, 
    (ClientData)sizeof(BarStyle)
  };

extern Tk_ObjCustomOption barPenObjOption;
extern Tk_ObjCustomOption pairsObjOption;
extern Tk_ObjCustomOption valuesObjOption;
extern Tk_ObjCustomOption xAxisObjOption;
extern Tk_ObjCustomOption yAxisObjOption;

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-activepen", "activePen", "ActivePen",
   "activeBar", -1, Tk_Offset(BarElementOptions, activePenPtr), 
   TK_OPTION_NULL_OK, &barPenObjOption, 0},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarElementOptions, builtinPen.fill),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-barwidth", "barWidth", "BarWidth",
   0, -1, Tk_Offset(BarElementOptions, barWidth), 0, NULL, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags",
   "all", -1, Tk_Offset(BarElementOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(BarElementOptions, builtinPen.borderWidth),
   0, NULL, 0},
  {TK_OPTION_SYNONYM, "-color", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-data", "data", "Data", 
   NULL, -1, Tk_Offset(BarElementOptions, coords),
   TK_OPTION_NULL_OK, &pairsObjOption, MAP_ITEM},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(BarElementOptions, builtinPen.errorBarColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS,"-errorbarwidth", "errorBarWidth", "ErrorBarWidth",
   "1", -1, Tk_Offset(BarElementOptions, builtinPen.errorBarLineWidth),
   0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(BarElementOptions, builtinPen.errorBarCapWidth),
   0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, 
   Tk_Offset(BarElementOptions, builtinPen.outlineColor),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(BarElementOptions, hide), 0, NULL, MAP_ITEM},
  {TK_OPTION_STRING, "-label", "label", "Label",
   NULL, -1, Tk_Offset(BarElementOptions, label), 
   TK_OPTION_NULL_OK, NULL, MAP_ITEM},
  {TK_OPTION_RELIEF, "-legendrelief", "legendRelief", "LegendRelief",
   "flat", -1, Tk_Offset(BarElementOptions, legendRelief), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX", 
   "x", -1, Tk_Offset(BarElementOptions, axes.x), 0, &xAxisObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY",
   "y", -1, Tk_Offset(BarElementOptions, axes.y), 0, &yAxisObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_CUSTOM, "-pen", "pen", "Pen", 
   NULL, -1, Tk_Offset(BarElementOptions, normalPenPtr), 
   TK_OPTION_NULL_OK, &barPenObjOption},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "raised", -1, Tk_Offset(BarElementOptions, builtinPen.relief), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(BarElementOptions, builtinPen.errorBarShow), 
   0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(BarElementOptions, builtinPen.valueShow), 
   0, &fillObjOption, 0},
  {TK_OPTION_STRING, "-stack", "stack", "Stack", 
   NULL, -1, Tk_Offset(BarElementOptions, groupName),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple",
   NULL, -1, Tk_Offset(BarElementOptions, builtinPen.stipple), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_CUSTOM, "-styles", "styles", "Styles",
   "", -1, Tk_Offset(BarElementOptions, stylePalette), 0, &styleObjOption, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(BarElementOptions, builtinPen.valueStyle.anchor),
   0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, 
   Tk_Offset(BarElementOptions,builtinPen.valueStyle.color),
   0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(BarElementOptions, builtinPen.valueStyle.font),
   0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(BarElementOptions, builtinPen.valueFormat),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(BarElementOptions, builtinPen.valueStyle.angle),
   0, NULL, 0},
  {TK_OPTION_CUSTOM, "-weights", "weights", "Weights",
   NULL, -1, Tk_Offset(BarElementOptions, w), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-x", NULL, NULL, NULL, -1, 0, 0, "-xdata", 0},
  {TK_OPTION_CUSTOM, "-xdata", "xData", "XData", 
   NULL, -1, Tk_Offset(BarElementOptions, coords.x), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xerror", "xError", "XError", 
   NULL, -1, Tk_Offset(BarElementOptions, xError), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xhigh", "xHigh", "XHigh", 
   NULL, -1, Tk_Offset(BarElementOptions, xHigh), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xlow", "xLow", "XLow", 
   NULL, -1, Tk_Offset(BarElementOptions, xLow), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-y", NULL, NULL, NULL, -1, 0, 0, "-ydata", 0},
  {TK_OPTION_CUSTOM, "-ydata", "yData", "YData", 
   NULL, -1, Tk_Offset(BarElementOptions, coords.y), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-yerror", "yError", "YError", 
   NULL, -1, Tk_Offset(BarElementOptions, yError), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-yhigh", "yHigh", "YHigh",
   NULL, -1, Tk_Offset(BarElementOptions, yHigh), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-ylow", "yLow", "YLow", 
   NULL, -1, Tk_Offset(BarElementOptions, yLow), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

Element* Blt_BarElement(Graph* graphPtr)
{
  BarElement* bePtr = (BarElement*)calloc(1, sizeof(BarElement));
  bePtr->ops = (BarElementOptions*)calloc(1, sizeof(BarElementOptions));
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;
  ops->elemPtr = (Element*)bePtr;

  bePtr->procsPtr = &barProcs;
  ops->builtinPenPtr = &bePtr->builtinPen;
  bePtr->builtinPen.ops = &ops->builtinPen;
  bePtr->builtinPen.manageOptions =0;

  InitBarPen(graphPtr, &bePtr->builtinPen, "builtin");
  Tk_InitOptions(graphPtr->interp, (char*)&(ops->builtinPen),
		 bePtr->builtinPen.optionTable, graphPtr->tkwin);

  bePtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  return (Element*)bePtr;
}

static void DestroyBarProc(Graph* graphPtr, Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  DestroyBarPenProc(graphPtr, (Pen*)&bePtr->builtinPen);

  if (ops->activePenPtr)
    Blt_FreePen((Pen*)ops->activePenPtr);
  if (ops->normalPenPtr)
    Blt_FreePen((Pen*)ops->normalPenPtr);

  ResetBar(bePtr);

  if (ops->stylePalette) {
    Blt_FreeStylePalette(ops->stylePalette);
    Blt_Chain_Destroy(ops->stylePalette);
  }

  if (bePtr->activeIndices)
    free(bePtr->activeIndices);
}

// Configure

static int ConfigureBarProc(Graph* graphPtr, Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  if (ConfigureBarPenProc(graphPtr, (Pen*)&bePtr->builtinPen)!= TCL_OK)
    return TCL_ERROR;

  // Point to the static normal pen if no external pens have been selected.
  Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette);
  if (!link) {
    link = Blt_Chain_AllocLink(sizeof(BarStyle));
    Blt_Chain_LinkAfter(ops->stylePalette, link, NULL);
  }
  BarStyle* stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
  stylePtr->penPtr = NORMALPEN(ops);

  return TCL_OK;
}

// Support

static void ResetStylePalette(Blt_Chain stylePalette)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette); link; 
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    stylePtr->xeb.length = 0;
    stylePtr->yeb.length = 0;
    stylePtr->nBars = 0;
  }
}

static void CheckBarStacks(Graph* graphPtr, Axis2d *pairPtr, 
			   double *minPtr, double *maxPtr)
{
  if ((graphPtr->barMode != BARS_STACKED) || (graphPtr->nBarGroups == 0))
    return;

  BarGroup *gp, *gend;
  for (gp = graphPtr->barGroups, gend = gp + graphPtr->nBarGroups; gp < gend;
       gp++) {
    if ((gp->axes.x == pairPtr->x) && (gp->axes.y == pairPtr->y)) {

      // Check if any of the y-values (because of stacking) are greater
      // than the current limits of the graph.
      if (gp->sum < 0.0f) {
	if (*minPtr > gp->sum)
	  *minPtr = gp->sum;
      }
      else {
	if (*maxPtr < gp->sum)
	  *maxPtr = gp->sum;
      }
    }
  }
}

static void GetBarExtentsProc(Element* elemPtr, Region2d *regPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  Graph* graphPtr = bePtr->obj.graphPtr;
  regPtr->top = regPtr->left = DBL_MAX;
  regPtr->bottom = regPtr->right = -DBL_MAX;

  if (!ops->coords.x || !ops->coords.y ||
      !ops->coords.x->nValues || !ops->coords.y->nValues)
    return;

  int nPoints = NUMBEROFPOINTS(ops);
  double barWidth = graphPtr->barWidth;
  if (ops->barWidth > 0.0f)
    barWidth = ops->barWidth;

  double middle = 0.5;
  regPtr->left = ops->coords.x->min - middle;
  regPtr->right = ops->coords.x->max + middle;

  regPtr->top = ops->coords.y->min;
  regPtr->bottom = ops->coords.y->max;
  if (regPtr->bottom < graphPtr->baseline)
    regPtr->bottom = graphPtr->baseline;

  // Handle stacked bar elements specially.
  // If element is stacked, the sum of its ordinates may be outside the
  // minimum/maximum limits of the element's data points.
  if ((graphPtr->barMode == BARS_STACKED) && (graphPtr->nBarGroups > 0))
    CheckBarStacks(graphPtr, &ops->axes, &regPtr->top, &regPtr->bottom);

  // Warning: You get what you deserve if the x-axis is logScale
  if (ops->axes.x->logScale)
    regPtr->left = Blt_FindElemValuesMinimum(ops->coords.x, DBL_MIN) + middle;

  // Fix y-min limits for barchart
  if (ops->axes.y->logScale) {
    if ((regPtr->top <= 0.0) || (regPtr->top > 1.0))
      regPtr->top = 1.0;
  }
  else {
    if (regPtr->top > 0.0)
      regPtr->top = 0.0;
  }

  // Correct the extents for error bars if they exist
  if (ops->xError && (ops->xError->nValues > 0)) {
    nPoints = MIN(ops->xError->nValues, nPoints);
    for (int ii=0; ii<nPoints; ii++) {
      double x = ops->coords.x->values[ii] + ops->xError->values[ii];
      if (x > regPtr->right)
	regPtr->right = x;

      x = ops->coords.x->values[ii] - ops->xError->values[ii];
      if (ops->axes.x->logScale) {
	// Mirror negative values, instead of ignoring them
	if (x < 0.0)
	  x = -x;

	if ((x > DBL_MIN) && (x < regPtr->left))
	  regPtr->left = x;

      } 
      else if (x < regPtr->left)
	regPtr->left = x;
    }		     
  }
  else {
    if ((ops->xHigh) &&
	(ops->xHigh->nValues > 0) && 
	(ops->xHigh->max > regPtr->right))
      regPtr->right = ops->xHigh->max;

    if (ops->xLow && (ops->xLow->nValues > 0)) {
      double left;
      if ((ops->xLow->min <= 0.0) && (ops->axes.x->logScale))
	left = Blt_FindElemValuesMinimum(ops->xLow, DBL_MIN);
      else
	left = ops->xLow->min;

      if (left < regPtr->left)
	regPtr->left = left;
    }
  }

  if (ops->yError && (ops->yError->nValues > 0)) {
    nPoints = MIN(ops->yError->nValues, nPoints);

    for (int ii=0; ii<nPoints; ii++) {
      double y = ops->coords.y->values[ii] + ops->yError->values[ii];
      if (y > regPtr->bottom)
	regPtr->bottom = y;

      y = ops->coords.y->values[ii] - ops->yError->values[ii];
      if (ops->axes.y->logScale) {
	// Mirror negative values, instead of ignoring them
	if (y < 0.0) 
	  y = -y;

	if ((y > DBL_MIN) && (y < regPtr->left))
	  regPtr->top = y;

      }
      else if (y < regPtr->top)
	regPtr->top = y;
    }		     
  }
  else {
    if ((ops->yHigh) &&
	(ops->yHigh->nValues > 0) && 
	(ops->yHigh->max > regPtr->bottom))
      regPtr->bottom = ops->yHigh->max;

    if (ops->yLow && ops->yLow->nValues > 0) {
      double top;
      if ((ops->yLow->min <= 0.0) && 
	  (ops->axes.y->logScale))
	top = Blt_FindElemValuesMinimum(ops->yLow, DBL_MIN);
      else
	top = ops->yLow->min;

      if (top < regPtr->top)
	regPtr->top = top;
    }
  }
}

static void ClosestBarProc(Graph* graphPtr, Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  ClosestSearch* searchPtr = &graphPtr->search;
  double minDist = searchPtr->dist;
  int imin = 0;
    
  int i;
  XRectangle *bp;
  for (bp = bePtr->bars, i = 0; i < bePtr->nBars; i++, bp++) {
    if (PointInRectangle(bp, searchPtr->x, searchPtr->y)) {
      imin = bePtr->barToData[i];
      minDist = 0.0;
      break;
    }
    double left = bp->x;
    double top = bp->y;
    double right = (double)(bp->x + bp->width);
    double bottom = (double)(bp->y + bp->height);

    Point2d outline[5];
    outline[4].x = outline[3].x = outline[0].x = left;
    outline[4].y = outline[1].y = outline[0].y = top;
    outline[2].x = outline[1].x = right;
    outline[3].y = outline[2].y = bottom;

    Point2d *pp, *pend;
    for (pp = outline, pend = outline + 4; pp < pend; pp++) {
      Point2d t = Blt_GetProjection(searchPtr->x, searchPtr->y, pp, pp + 1);
      if (t.x > right)
	t.x = right;
      else if (t.x < left)
	t.x = left;

      if (t.y > bottom)
	t.y = bottom;
      else if (t.y < top)
	t.y = top;

      double dist = hypot((t.x - searchPtr->x), (t.y - searchPtr->y));
      if (dist < minDist) {
	minDist = dist;
	imin = bePtr->barToData[i];
      }
    }
  }
  if (minDist < searchPtr->dist) {
    searchPtr->elemPtr = (Element*)bePtr;
    searchPtr->dist = minDist;
    searchPtr->index = imin;
    searchPtr->point.x = 
      ops->coords.x ? (double)ops->coords.x->values[imin] : 0;
    searchPtr->point.y = 
      ops->coords.y ? (double)ops->coords.y->values[imin] : 0;
  }
}

static void MergePens(BarElement* bePtr, BarStyle **dataToStyle)
{
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  if (Blt_Chain_GetLength(ops->stylePalette) < 2) {
    Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette);
    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    stylePtr->nBars = bePtr->nBars;
    stylePtr->bars = bePtr->bars;
    stylePtr->symbolSize = bePtr->bars->width / 2;
    stylePtr->xeb.length = bePtr->xeb.length;
    stylePtr->xeb.segments = bePtr->xeb.segments;
    stylePtr->yeb.length = bePtr->yeb.length;
    stylePtr->yeb.segments = bePtr->yeb.segments;
    return;
  }

  // We have more than one style. Group bar segments of like pen styles
  // together
  if (bePtr->nBars > 0) {
    Blt_ChainLink link;
    XRectangle *bars, *bp;
    int *ip, *barToData;

    bars = (XRectangle*)malloc(bePtr->nBars * sizeof(XRectangle));
    barToData = (int*)malloc(bePtr->nBars * sizeof(int));
    bp = bars, ip = barToData;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      int i;

      BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
      stylePtr->symbolSize = bp->width / 2;
      stylePtr->bars = bp;
      for (i = 0; i < bePtr->nBars; i++) {
	int iData;

	iData = bePtr->barToData[i];
	if (dataToStyle[iData] == stylePtr) {
	  *bp++ = bePtr->bars[i];
	  *ip++ = iData;
	}
      }
      stylePtr->nBars = bp - stylePtr->bars;
    }
    free(bePtr->bars);
    free(bePtr->barToData);
    bePtr->bars = bars;
    bePtr->barToData = barToData;
  }

  if (bePtr->xeb.length > 0) {
    Blt_ChainLink link;
    Segment2d *sp;
    int *ip;

    Segment2d* bars = 
      (Segment2d*)malloc(bePtr->xeb.length * sizeof(Segment2d));
    int* map = (int*)malloc(bePtr->xeb.length * sizeof(int));
    sp = bars, ip = map;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      int i;

      BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
      stylePtr->xeb.segments = sp;
      for (i = 0; i < bePtr->xeb.length; i++) {
	int iData;

	iData = bePtr->xeb.map[i];
	if (dataToStyle[iData] == stylePtr) {
	  *sp++ = bePtr->xeb.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->xeb.length = sp - stylePtr->xeb.segments;
    }
    free(bePtr->xeb.segments);
    bePtr->xeb.segments = bars;
    free(bePtr->xeb.map);
    bePtr->xeb.map = map;
  }
  if (bePtr->yeb.length > 0) {
    Blt_ChainLink link;
    Segment2d *sp;
    int *ip;

    Segment2d* bars = 
      (Segment2d*)malloc(bePtr->yeb.length * sizeof(Segment2d));
    int* map = (int*)malloc(bePtr->yeb.length * sizeof(int));
    sp = bars, ip = map;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      int i;

      BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
      stylePtr->yeb.segments = sp;
      for (i = 0; i < bePtr->yeb.length; i++) {
	int iData;

	iData = bePtr->yeb.map[i];
	if (dataToStyle[iData] == stylePtr) {
	  *sp++ = bePtr->yeb.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->yeb.length = sp - stylePtr->yeb.segments;
    }
    free(bePtr->yeb.segments);
    bePtr->yeb.segments = bars;
    free(bePtr->yeb.map);
    bePtr->yeb.map = map;
  }
}

static void MapActiveBars(BarElement* bePtr)
{
  if (bePtr->activeRects) {
    free(bePtr->activeRects);
    bePtr->activeRects = NULL;
  }
  if (bePtr->activeToData) {
    free(bePtr->activeToData);
    bePtr->activeToData = NULL;
  }
  bePtr->nActive = 0;

  if (bePtr->nActiveIndices > 0) {
    int i;
    int count;

    XRectangle *activeRects = 
      (XRectangle*)malloc(sizeof(XRectangle) * bePtr->nActiveIndices);
    int* activeToData = (int*)malloc(sizeof(int) * bePtr->nActiveIndices);
    count = 0;
    for (i = 0; i < bePtr->nBars; i++) {
      int *ip, *iend;

      for (ip = bePtr->activeIndices, 
	     iend = ip + bePtr->nActiveIndices; ip < iend; ip++) {
	if (bePtr->barToData[i] == *ip) {
	  activeRects[count] = bePtr->bars[i];
	  activeToData[count] = i;
	  count++;
	}
      }
    }
    bePtr->nActive = count;
    bePtr->activeRects = activeRects;
    bePtr->activeToData = activeToData;
  }
  bePtr->flags &= ~ACTIVE_PENDING;
}

static void ResetBar(BarElement* bePtr)
{
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  ResetStylePalette(ops->stylePalette);

  if (bePtr->activeRects)
    free(bePtr->activeRects);

  if (bePtr->activeToData)
    free(bePtr->activeToData);

  if (bePtr->xeb.segments)
    free(bePtr->xeb.segments);

  if (bePtr->xeb.map)
    free(bePtr->xeb.map);

  if (bePtr->yeb.segments)
    free(bePtr->yeb.segments);

  if (bePtr->yeb.map)
    free(bePtr->yeb.map);

  if (bePtr->bars)
    free(bePtr->bars);

  if (bePtr->barToData)
    free(bePtr->barToData);

  bePtr->activeToData = bePtr->xeb.map = bePtr->yeb.map = 
    bePtr->barToData = NULL;
  bePtr->activeRects = NULL;
  bePtr->bars = NULL;
  bePtr->xeb.segments = NULL;
  bePtr->yeb.segments = NULL;
  bePtr->nActive = 0;
  bePtr->xeb.length = 0;
  bePtr->yeb.length = 0;
  bePtr->nBars = 0;
}

static void MapErrorBars(Graph* graphPtr, BarElement* bePtr, 
			 BarStyle **dataToStyle)
{
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  Region2d reg;
  Blt_GraphExtents(graphPtr, &reg);

  int nPoints = NUMBEROFPOINTS(ops);
  int n =0;
  if (ops->coords.x && ops->coords.y) {
    if (ops->xError && (ops->xError->nValues > 0))
      n = MIN(ops->xError->nValues, nPoints);
    else
      if (ops->xHigh && ops->xLow)
	n = MIN3(ops->xHigh->nValues, ops->xLow->nValues, nPoints);
  }

  if (n > 0) {
    Segment2d*bars = (Segment2d*)malloc(n * 3 * sizeof(Segment2d));
    Segment2d* segPtr = bars;
    int* map = (int*)malloc(n * 3 * sizeof(int));
    int* indexPtr = map;

    for (int ii=0; ii<n; ii++) {
      double x = ops->coords.x->values[ii];
      double y = ops->coords.y->values[ii];
      BarStyle* stylePtr = dataToStyle[ii];

      double high, low;
      if ((isfinite(x)) && (isfinite(y))) {
	if (ops->xError->nValues > 0) {
	  high = x + ops->xError->values[ii];
	  low = x - ops->xError->values[ii];
	}
	else {
	  high = ops->xHigh ? ops->xHigh->values[ii] : 0;
	  low  = ops->xLow  ? ops->xLow->values[ii]  : 0;
	}
	if ((isfinite(high)) && (isfinite(low)))  {
	  Point2d p = Blt_Map2D(graphPtr, high, y, &ops->axes);
	  Point2d q = Blt_Map2D(graphPtr, low, y, &ops->axes);
	  segPtr->p = p;
	  segPtr->q = q;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = ii;
	  }
	  /* Left cap */
	  segPtr->p.x = segPtr->q.x = p.x;
	  segPtr->p.y = p.y - stylePtr->errorBarCapWidth;
	  segPtr->q.y = p.y + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = ii;
	  }
	  /* Right cap */
	  segPtr->p.x = segPtr->q.x = q.x;
	  segPtr->p.y = q.y - stylePtr->errorBarCapWidth;
	  segPtr->q.y = q.y + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = ii;
	  }
	}
      }
    }
    bePtr->xeb.segments = bars;
    bePtr->xeb.length = segPtr - bars;
    bePtr->xeb.map = map;
  }

  n =0;
  if (ops->coords.x && ops->coords.y) {
    if (ops->yError && (ops->yError->nValues > 0))
      n = MIN(ops->yError->nValues, nPoints);
    else
      if (ops->yHigh && ops->yLow)
	n = MIN3(ops->yHigh->nValues, ops->yLow->nValues, nPoints);
  }

  if (n > 0) {
    Segment2d* bars = (Segment2d*)malloc(n * 3 * sizeof(Segment2d));
    Segment2d* segPtr = bars;
    int* map = (int*)malloc(n * 3 * sizeof(int));
    int* indexPtr = map;

    for (int ii=0; ii<n; ii++) {
      double x = ops->coords.x->values[ii];
      double y = ops->coords.y->values[ii];
      BarStyle *stylePtr = dataToStyle[ii];

      double high, low;
      if ((isfinite(x)) && (isfinite(y))) {
	if (ops->yError->nValues > 0) {
	  high = y + ops->yError->values[ii];
	  low = y - ops->yError->values[ii];
	} else {
	  high = ops->yHigh->values[ii];
	  low = ops->yLow->values[ii];
	}
	if ((isfinite(high)) && (isfinite(low)))  {
	  Point2d p = Blt_Map2D(graphPtr, x, high, &ops->axes);
	  Point2d q = Blt_Map2D(graphPtr, x, low, &ops->axes);
	  segPtr->p = p;
	  segPtr->q = q;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = ii;
	  }
	  /* Top cap. */
	  segPtr->p.y = segPtr->q.y = p.y;
	  segPtr->p.x = p.x - stylePtr->errorBarCapWidth;
	  segPtr->q.x = p.x + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = ii;
	  }
	  /* Bottom cap. */
	  segPtr->p.y = segPtr->q.y = q.y;
	  segPtr->p.x = q.x - stylePtr->errorBarCapWidth;
	  segPtr->q.x = q.x + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&reg, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = ii;
	  }
	}
      }
    }
    bePtr->yeb.segments = bars;
    bePtr->yeb.length = segPtr - bars;
    bePtr->yeb.map = map;
  }
}

static void MapBarProc(Graph* graphPtr, Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  ResetBar(bePtr);
  if (!ops->coords.x || !ops->coords.y ||
      !ops->coords.x->nValues || !ops->coords.y->nValues)
    return;
  int nPoints = NUMBEROFPOINTS(ops);

  //  double barWidth = graphPtr->barWidth;
  double barWidth = (ops->barWidth > 0.0f) ? 
    ops->barWidth : graphPtr->barWidth;
  double baseline = (ops->axes.y->logScale) ? 0.0 : graphPtr->baseline;
  double barOffset = barWidth * 0.5;

  // Create an array of bars representing the screen coordinates of all the
  // segments in the bar.
  XRectangle* bars = (XRectangle*)calloc(nPoints, sizeof(XRectangle));
  int* barToData = (int*)calloc(nPoints, sizeof(int));

  double* x = ops->coords.x->values;
  double* y = ops->coords.y->values;
  int count = 0;

  int i;
  XRectangle *rp;
  for (rp = bars, i = 0; i < nPoints; i++) {
    Point2d c1, c2;			/* Two opposite corners of the rectangle
					 * in graph coordinates. */
    double dx, dy;
    int height;
    double right, left, top, bottom;

    if (((x[i] - barWidth) > ops->axes.x->axisRange.max) ||
	((x[i] + barWidth) < ops->axes.x->axisRange.min)) {
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
      key.axes = ops->axes;
      key.axes.y = NULL;
      hPtr = Tcl_FindHashEntry(&graphPtr->setTable, (char *)&key);
      if (hPtr) {

	Tcl_HashTable *tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);
	const char *name = (ops->groupName) ? 
	  ops->groupName : ops->axes.y->obj.name;
	hPtr = Tcl_FindHashEntry(tablePtr, name);
	if (hPtr) {
	  double slice, width, offset;
		    
	  BarGroup *groupPtr = (BarGroup*)Tcl_GetHashValue(hPtr);
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
    int invertBar = 0;
    if (c1.y < c2.y) {
      double temp;

      /* Handle negative bar values by swapping ordinates */
      temp = c1.y, c1.y = c2.y, c2.y = temp;
      invertBar = 1;
    }
    /*
     * Get the two corners of the bar segment and compute the rectangle
     */
    double ybot = c2.y;
    c1 = Blt_Map2D(graphPtr, c1.x, c1.y, &ops->axes);
    c2 = Blt_Map2D(graphPtr, c2.x, c2.y, &ops->axes);
    if ((ybot == 0.0) && (ops->axes.y->logScale)) {
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
      top = ops->axes.y->screenMin;
      bottom = ops->axes.y->screenMin + ops->axes.y->screenRange;
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
  bePtr->nBars = count;
  bePtr->bars = bars;
  bePtr->barToData = barToData;
  if (bePtr->nActiveIndices > 0) {
    MapActiveBars(bePtr);
  }
	
  int size = 20;
  if (count > 0)
    size = bars->width;

  // Set the symbol size of all the pen styles
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    BarPen* penPtr = stylePtr->penPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;
    stylePtr->symbolSize = size;
    stylePtr->errorBarCapWidth = 
      (penOps->errorBarCapWidth > 0) 
      ? penOps->errorBarCapWidth : (size * 66666) / 100000;
    stylePtr->errorBarCapWidth /= 2;
  }

  BarStyle** dataToStyle = (BarStyle**)Blt_StyleMap((Element*)bePtr);
  if (((ops->yHigh && ops->yHigh->nValues > 0) && 
       (ops->yLow && ops->yLow->nValues > 0)) ||
      ((ops->xHigh && ops->xHigh->nValues > 0) &&
       (ops->xLow && ops->xLow->nValues > 0)) ||
      (ops->xError && ops->xError->nValues > 0) || 
      (ops->yError && ops->yError->nValues > 0)) {
    MapErrorBars(graphPtr, bePtr, dataToStyle);
  }

  MergePens(bePtr, dataToStyle);
  free(dataToStyle);
}

static void DrawSymbolProc(Graph* graphPtr, Drawable drawable, 
			   Element* elemPtr, int x, int y, int size)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  BarPen* penPtr = NORMALPEN(ops);
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;

  if (!penOps->fill && !penOps->outlineColor)
    return;

  int radius = (size / 2);
  size--;

  x -= radius;
  y -= radius;
  if (penPtr->fillGC)
    XSetTSOrigin(graphPtr->display, penPtr->fillGC, x, y);

  if (penOps->stipple != None)
    XFillRectangle(graphPtr->display, drawable, penPtr->fillGC, x, y, 
		   size, size);
  else
    Tk_Fill3DRectangle(graphPtr->tkwin, drawable, penOps->fill, 
		       x, y, size, size, penOps->borderWidth, penOps->relief);

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
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;
  TkRegion rgn;

  XRectangle clip;
  clip.x = graphPtr->left;
  clip.y = graphPtr->top;
  clip.width  = graphPtr->right - graphPtr->left + 1;
  clip.height = graphPtr->bottom - graphPtr->top + 1;
  rgn = TkCreateRegion();
  TkUnionRectWithRegion(&clip, rgn, rgn);

  if (penOps->fill) {
    int relief = (penOps->relief == TK_RELIEF_SOLID) ? 
      TK_RELIEF_FLAT: penOps->relief;

    int hasOutline = ((relief == TK_RELIEF_FLAT) && penOps->outlineColor);
    if (penOps->stipple != None)
      TkSetRegion(graphPtr->display, penPtr->fillGC, rgn);

    SetBackgroundClipRegion(graphPtr->tkwin, penOps->fill, rgn);

    if (hasOutline)
      TkSetRegion(graphPtr->display, penPtr->outlineGC, rgn);

    XRectangle *rp, *rend;
    for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
      if (penOps->stipple != None)
	XFillRectangle(graphPtr->display, drawable, penPtr->fillGC, 
		       rp->x, rp->y, rp->width, rp->height);
      else
	Tk_Fill3DRectangle(graphPtr->tkwin, drawable, 
			   penOps->fill, rp->x, rp->y, rp->width, rp->height, 
			   penOps->borderWidth, relief);

      if (hasOutline)
	XDrawRectangle(graphPtr->display, drawable, penPtr->outlineGC, 
		       rp->x, rp->y, rp->width, rp->height);
    }

    UnsetBackgroundClipRegion(graphPtr->tkwin, penOps->fill);

    if (hasOutline)
      XSetClipMask(graphPtr->display, penPtr->outlineGC, None);

    if (penOps->stipple != None)
      XSetClipMask(graphPtr->display, penPtr->fillGC, None);

  }
  else if (penOps->outlineColor) {
    TkSetRegion(graphPtr->display, penPtr->outlineGC, rgn);
    XDrawRectangles(graphPtr->display, drawable, penPtr->outlineGC, bars, 
		    nBars);
    XSetClipMask(graphPtr->display, penPtr->outlineGC, None);
  }

  TkDestroyRegion(rgn);
}

static void DrawBarValues(Graph* graphPtr, Drawable drawable, 
			  BarElement* bePtr,
			  BarPen* penPtr, XRectangle *bars, int nBars, 
			  int *barToData)
{
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;

  const char *fmt = penOps->valueFormat;
  if (!fmt)
    fmt = "%g";

  int count = 0;
  XRectangle *rp, *rend;
  for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
    Point2d anchorPos;
    char string[TCL_DOUBLE_SPACE * 2 + 2];

    double x = ops->coords.x->values[barToData[count]];
    double y = ops->coords.y->values[barToData[count]];

    count++;
    if (penOps->valueShow == SHOW_X)
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x); 
    else if (penOps->valueShow == SHOW_Y)
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, y); 
    else if (penOps->valueShow == SHOW_BOTH) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x);
      strcat(string, ",");
      sprintf_s(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
    }

    if (graphPtr->inverted) {
      anchorPos.y = rp->y + rp->height * 0.5;
      anchorPos.x = rp->x + rp->width;
      if (x < graphPtr->baseline)
	anchorPos.x -= rp->width;
    }
    else {
      anchorPos.x = rp->x + rp->width * 0.5;
      anchorPos.y = rp->y;
      if (y < graphPtr->baseline)
	anchorPos.y += rp->height;
    }
    Blt_DrawText(graphPtr->tkwin, drawable, string, &penOps->valueStyle, 
		 (int)anchorPos.x, (int)anchorPos.y);
  }
}

static void DrawNormalBarProc(Graph* graphPtr, Drawable drawable, 
			      Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  int count = 0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {

    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    BarPen* penPtr = (BarPen*)stylePtr->penPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;

    if (stylePtr->nBars > 0)
      DrawBarSegments(graphPtr, drawable, penPtr, stylePtr->bars,
		      stylePtr->nBars);

    if ((stylePtr->xeb.length > 0) && (penOps->errorBarShow & SHOW_X))
      Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->errorBarGC, 
			 stylePtr->xeb.segments, stylePtr->xeb.length);

    if ((stylePtr->yeb.length > 0) && (penOps->errorBarShow & SHOW_Y))
      Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->errorBarGC, 
			 stylePtr->yeb.segments, stylePtr->yeb.length);

    if (penOps->valueShow != SHOW_NONE)
      DrawBarValues(graphPtr, drawable, bePtr, penPtr, 
		    stylePtr->bars, stylePtr->nBars, 
		    bePtr->barToData + count);

    count += stylePtr->nBars;
  }
}

static void DrawActiveBarProc(Graph* graphPtr, Drawable drawable, 
			      Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  if (ops->activePenPtr) {
    BarPen* penPtr = ops->activePenPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;

    if (bePtr->nActiveIndices > 0) {
      if (bePtr->flags & ACTIVE_PENDING) {
	MapActiveBars(bePtr);
      }
      DrawBarSegments(graphPtr, drawable, penPtr, bePtr->activeRects, 
		      bePtr->nActive);
      if (penOps->valueShow != SHOW_NONE) {
	DrawBarValues(graphPtr, drawable, bePtr, penPtr, 
		      bePtr->activeRects, bePtr->nActive, 
		      bePtr->activeToData);
      }
    }
    else if (bePtr->nActiveIndices < 0) {
      DrawBarSegments(graphPtr, drawable, penPtr, bePtr->bars, 
		      bePtr->nBars);
      if (penOps->valueShow != SHOW_NONE) {
	DrawBarValues(graphPtr, drawable, bePtr, penPtr, 
		      bePtr->bars, bePtr->nBars, bePtr->barToData);
      }
    }
  }
}

static void SymbolToPostScriptProc(Graph* graphPtr, Blt_Ps ps, Element* elemPtr,
				   double x, double y, int size)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  BarPen* penPtr = NORMALPEN(ops);
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;

  if (!penOps->fill && !penOps->outlineColor)
    return;

  /*
   * Build a PostScript procedure to draw the fill and outline of the symbol
   * after the path of the symbol shape has been formed
   */
  Blt_Ps_Append(ps, "\n"
		"/DrawSymbolProc {\n"
		"gsave\n    ");
  if (penOps->stipple != None) {
    if (penOps->fill) {
      Blt_Ps_XSetBackground(ps, Tk_3DBorderColor(penOps->fill));
      Blt_Ps_Append(ps, "    gsave fill grestore\n    ");
    }
    if (penOps->outlineColor) {
      Blt_Ps_XSetForeground(ps, penOps->outlineColor);
    } else {
      Blt_Ps_XSetForeground(ps, Tk_3DBorderColor(penOps->fill));
    }
    Blt_Ps_XSetStipple(ps, graphPtr->display, penOps->stipple);
  } else if (penOps->outlineColor) {
    Blt_Ps_XSetForeground(ps, penOps->outlineColor);
    Blt_Ps_Append(ps, "    fill\n");
  }
  Blt_Ps_Append(ps, "  grestore\n");
  Blt_Ps_Append(ps, "} def\n\n");
  Blt_Ps_Format(ps, "%g %g %d Sq\n", x, y, size);
}

static void SegmentsToPostScript(Graph* graphPtr, Blt_Ps ps, BarPen* penPtr, 
				 XRectangle *bars, int nBars)
{
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;
  XRectangle *rp, *rend;

  if (!penOps->fill && !penOps->outlineColor)
    return;

  for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
    if ((rp->width < 1) || (rp->height < 1)) {
      continue;
    }
    if (penOps->stipple != None) {
      Blt_Ps_Rectangle(ps, rp->x, rp->y, rp->width - 1, rp->height - 1);
      if (penOps->fill) {
	Blt_Ps_XSetBackground(ps, Tk_3DBorderColor(penOps->fill));
	Blt_Ps_Append(ps, "gsave fill grestore\n");
      }
      if (penOps->outlineColor) {
	Blt_Ps_XSetForeground(ps, penOps->outlineColor);
      } else {
	Blt_Ps_XSetForeground(ps, Tk_3DBorderColor(penOps->fill));
      }
      Blt_Ps_XSetStipple(ps, graphPtr->display, penOps->stipple);
    } else if (penOps->outlineColor) {
      Blt_Ps_XSetForeground(ps, penOps->outlineColor);
      Blt_Ps_XFillRectangle(ps, (double)rp->x, (double)rp->y, 
			    (int)rp->width - 1, (int)rp->height - 1);
    }
    if ((penOps->fill) && (penOps->borderWidth > 0) && 
	(penOps->relief != TK_RELIEF_FLAT)) {
      Blt_Ps_Draw3DRectangle(ps, penOps->fill, (double)rp->x, (double)rp->y,
			     (int)rp->width, (int)rp->height,
			     penOps->borderWidth, penOps->relief);
    }
  }
}

static void BarValuesToPostScript(Graph* graphPtr, Blt_Ps ps, 
				  BarElement* bePtr,
				  BarPen* penPtr, XRectangle *bars, int nBars, 
				  int *barToData)
{
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  XRectangle *rp, *rend;
  char string[TCL_DOUBLE_SPACE * 2 + 2];
  double x, y;
  Point2d anchorPos;
    
  int count = 0;
  const char* fmt = penOps->valueFormat;
  if (!fmt)
    fmt = "%g";

  for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
    x = ops->coords.x->values[barToData[count]];
    y = ops->coords.y->values[barToData[count]];
    count++;
    if (penOps->valueShow == SHOW_X) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x); 
    } else if (penOps->valueShow == SHOW_Y) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, y); 
    } else if (penOps->valueShow == SHOW_BOTH) {
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
    Blt_Ps_DrawText(ps, string, &penOps->valueStyle, anchorPos.x, 
		    anchorPos.y);
  }
}

static void ActiveBarToPostScriptProc(Graph* graphPtr, Blt_Ps ps, 
				      Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;

  if (ops->activePenPtr) {
    BarPen* penPtr = ops->activePenPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;
	
    if (bePtr->nActiveIndices > 0) {
      if (bePtr->flags & ACTIVE_PENDING) {
	MapActiveBars(bePtr);
      }
      SegmentsToPostScript(graphPtr, ps, penPtr, bePtr->activeRects,
			   bePtr->nActive);
      if (penOps->valueShow != SHOW_NONE) {
	BarValuesToPostScript(graphPtr, ps, bePtr, penPtr, 
			      bePtr->activeRects, bePtr->nActive, bePtr->activeToData);
      }
    } else if (bePtr->nActiveIndices < 0) {
      SegmentsToPostScript(graphPtr, ps, penPtr, bePtr->bars, 
			   bePtr->nBars);
      if (penOps->valueShow != SHOW_NONE) {
	BarValuesToPostScript(graphPtr, ps, bePtr, penPtr, 
			      bePtr->bars, bePtr->nBars, bePtr->barToData);
      }
    }
  }
}

static void NormalBarToPostScriptProc(Graph* graphPtr, Blt_Ps ps, 
				      Element* elemPtr)
{
  BarElement* bePtr = (BarElement*)elemPtr;
  BarElementOptions* ops = (BarElementOptions*)bePtr->ops;
  
  int count = 0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {

    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    BarPen* penPtr = (BarPen*)stylePtr->penPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops;
    if (stylePtr->nBars > 0)
      SegmentsToPostScript(graphPtr, ps, penPtr, stylePtr->bars, 
			   stylePtr->nBars);

    XColor* colorPtr = penOps->errorBarColor;
    if (!colorPtr)
      colorPtr = penOps->outlineColor;

    if ((stylePtr->xeb.length > 0) && (penOps->errorBarShow & SHOW_X)) {
      Blt_Ps_XSetLineAttributes(ps, colorPtr, penOps->errorBarLineWidth, 
				NULL, CapButt, JoinMiter);
      Blt_Ps_Draw2DSegments(ps, stylePtr->xeb.segments,
			    stylePtr->xeb.length);
    }

    if ((stylePtr->yeb.length > 0) && (penOps->errorBarShow & SHOW_Y)) {
      Blt_Ps_XSetLineAttributes(ps, colorPtr, penOps->errorBarLineWidth, 
				NULL, CapButt, JoinMiter);
      Blt_Ps_Draw2DSegments(ps, stylePtr->yeb.segments, 
			    stylePtr->yeb.length);
    }

    if (penOps->valueShow != SHOW_NONE)
      BarValuesToPostScript(graphPtr, ps, bePtr, penPtr, 
			    stylePtr->bars, stylePtr->nBars, 
			    bePtr->barToData + count);

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
  if (graphPtr->barMode == BARS_INFRONT)
    return;
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
    double *x, *xend;
    int nPoints;

    BarElement* bePtr = (BarElement*)Blt_Chain_GetValue(link);
    BarElementOptions* ops = (BarElementOptions*)bePtr->ops;
    if ((bePtr->hide) || (bePtr->obj.classId != CID_ELEM_BAR))
      continue;

    nSegs++;
    
    if (ops->coords.x) {
      nPoints = ops->coords.x->nValues;
      for (x = ops->coords.x->values, xend = x + nPoints; x < xend; x++) {
	Tcl_HashEntry *hPtr;
	BarSetKey key;
	int isNew;
	size_t count;
	const char *name;

	key.value = *x;
	key.axes = ops->axes;
	key.axes.y = NULL;
	hPtr = Tcl_CreateHashEntry(&setTable, (char *)&key, &isNew);
	Tcl_HashTable *tablePtr;
	if (isNew) {
	  tablePtr = (Tcl_HashTable*)malloc(sizeof(Tcl_HashTable));
	  Tcl_InitHashTable(tablePtr, TCL_STRING_KEYS);
	  Tcl_SetHashValue(hPtr, tablePtr);
	}
	else
	  tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);

	name = (ops->groupName) ? ops->groupName : 
	  ops->axes.y->obj.name;
	hPtr = Tcl_CreateHashEntry(tablePtr, name, &isNew);
	if (isNew)
	  count = 1;
	else {
	  count = (size_t)Tcl_GetHashValue(hPtr);
	  count++;
	}		
	Tcl_SetHashValue(hPtr, (ClientData)count);
      }
    }
  }

  if (setTable.numEntries == 0)
    return;

  sum = max = 0;
  for (hPtr = Tcl_FirstHashEntry(&setTable, &iter); hPtr;
       hPtr = Tcl_NextHashEntry(&iter)) {
    Tcl_HashEntry *hPtr2;
    BarSetKey *keyPtr;
    int isNew;

    keyPtr = (BarSetKey *)Tcl_GetHashKey(&setTable, hPtr);
    hPtr2 = Tcl_CreateHashEntry(&graphPtr->setTable, (char *)keyPtr,&isNew);
    Tcl_HashTable *tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);
    Tcl_SetHashValue(hPtr2, tablePtr);
    if (max < tablePtr->numEntries) {
      max = tablePtr->numEntries;	/* # of stacks in group. */
    }
    sum += tablePtr->numEntries;
  }
  Tcl_DeleteHashTable(&setTable);
  if (sum > 0) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    graphPtr->barGroups = (BarGroup*)calloc(sum, sizeof(BarGroup));
    BarGroup *groupPtr = graphPtr->barGroups;
    for (hPtr = Tcl_FirstHashEntry(&graphPtr->setTable, &iter); 
	 hPtr; hPtr = Tcl_NextHashEntry(&iter)) {
      BarSetKey *keyPtr;
      Tcl_HashEntry *hPtr2;
      Tcl_HashSearch iter2;
      size_t xcount;

      Tcl_HashTable *tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);
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
  if ((graphPtr->barMode != BARS_STACKED) || (graphPtr->nBarGroups == 0))
    return;

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
    double *x, *y, *xend;

    BarElement* bePtr = (BarElement*)Blt_Chain_GetValue(link);
    BarElementOptions* ops = (BarElementOptions*)bePtr->ops;
    if ((bePtr->hide) || (bePtr->obj.classId != CID_ELEM_BAR))
      continue;

    if (ops->coords.x && ops->coords.y) {
      for (x = ops->coords.x->values, y = ops->coords.y->values, 
	     xend = x + ops->coords.x->nValues; x < xend; x++, y++) {
	BarSetKey key;
	Tcl_HashEntry *hPtr;
	const char *name;

	key.value = *x;
	key.axes = ops->axes;
	key.axes.y = NULL;
	hPtr = Tcl_FindHashEntry(&graphPtr->setTable, (char *)&key);
	if (!hPtr)
	  continue;

	Tcl_HashTable *tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);
	name = (ops->groupName) ? ops->groupName : 
	  ops->axes.y->obj.name;
	hPtr = Tcl_FindHashEntry(tablePtr, name);
	if (!hPtr)
	  continue;

	BarGroup *groupPtr = (BarGroup*)Tcl_GetHashValue(hPtr);
	groupPtr->sum += *y;
      }
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
    Tcl_HashTable* tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);
    Tcl_DeleteHashTable(tablePtr);
    free(tablePtr);
  }
  Tcl_DeleteHashTable(&graphPtr->setTable);
  Tcl_InitHashTable(&graphPtr->setTable, sizeof(BarSetKey) / sizeof(int));
}
