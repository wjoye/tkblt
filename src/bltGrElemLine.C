/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright (c) 1993 George A Howlett.
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

extern "C" {
#include "bltSpline.h"
}

#include "bltGrElemLine.h"
#include "bltGrElemOption.h"
#include "bltGrPenOp.h"

#define SEARCH_X	0
#define SEARCH_Y	1
#define SEARCH_BOTH	2

#define SEARCH_POINTS	0	/* Search for closest data point. */
#define SEARCH_TRACES	1	/* Search for closest point on trace.
				 * Interpolate the connecting line segments if
				 * necessary. */
#define SEARCH_AUTO	2	/* Automatically determine whether to search
				 * for data points or traces.  Look for traces
				 * if the linewidth is > 0 and if there is
				 * more than one data point. */

#define SCALE_SYMBOL	(1<<10)

#define PointInRegion(e,x,y) (((x) <= (e)->right) && ((x) >= (e)->left) && ((y) <= (e)->bottom) && ((y) >= (e)->top))

#define BROKEN_TRACE(dir,last,next)			\
  (((dir == PEN_INCREASING) && (next < last)) ||	\
   ((dir == PEN_DECREASING) && (next > last)))

#define DRAW_SYMBOL(linePtr)					\
  (((linePtr)->symbolCounter % (linePtr)->symbolInterval) == 0)

typedef enum {
  PEN_INCREASING, PEN_DECREASING, PEN_BOTH_DIRECTIONS
} PenDirection;

typedef struct {
  Point2d *screenPts;
  int nScreenPts;
  int *styleMap;
  int *map;
} MapInfo;

typedef struct {
  int start;
  GraphPoints screenPts;
} bltTrace;

typedef struct {
  Weight weight;
  LinePen* penPtr;
  GraphPoints symbolPts;
  GraphSegments lines;
  GraphSegments xeb;
  GraphSegments yeb;
  int symbolSize;
  int errorBarCapWidth;
} LineStyle;

// Defs

typedef double (DistanceProc)(int x, int y, Point2d *p, Point2d *q, Point2d *t);
static void ResetLine(LineElement* elemPtr);

static ElementClosestProc ClosestLineProc;
static ElementConfigProc ConfigureLineProc;
static ElementDestroyProc DestroyLineProc;
static ElementDrawProc DrawActiveLineProc;
static ElementDrawProc DrawNormalLineProc;
static ElementDrawSymbolProc DrawSymbolProc;
static ElementExtentsProc GetLineExtentsProc;
static ElementToPostScriptProc ActiveLineToPostScriptProc;
static ElementToPostScriptProc NormalLineToPostScriptProc;
static ElementSymbolToPostScriptProc SymbolToPostScriptProc;
static ElementMapProc MapLineProc;
static DistanceProc DistanceToYProc;
static DistanceProc DistanceToXProc;
static DistanceProc DistanceToLineProc;

static ElementProcs lineProcs =
  {
    ClosestLineProc,			/* Finds the closest element/data
					 * point */
    ConfigureLineProc,			/* Configures the element. */
    DestroyLineProc,			/* Destroys the element. */
    DrawActiveLineProc,			/* Draws active element */
    DrawNormalLineProc,			/* Draws normal element */
    DrawSymbolProc,			/* Draws the element symbol. */
    GetLineExtentsProc,			/* Find the extents of the element's
					 * data. */
    ActiveLineToPostScriptProc,		/* Prints active element. */
    NormalLineToPostScriptProc,		/* Prints normal element. */
    SymbolToPostScriptProc,		/* Prints the line's symbol. */
    MapLineProc				/* Compute element's screen
					 * coordinates. */
  };

INLINE static int Round(double x)
{
  return (int) (x + ((x < 0.0) ? -0.5 : 0.5));
}

// OptionSpecs

static const char* smoothObjOption[] = 
  {"linear", "step", "natural", "quadratic", "catrom", NULL};

static const char* penDirObjOption[] = 
  {"increasing", "decreasing", "both", NULL};

static Tk_ObjCustomOption styleObjOption =
  {
    "styles", StyleSetProc, StyleGetProc, StyleRestoreProc, StyleFreeProc, 
    (ClientData)sizeof(LineStyle)

  };

extern Tk_ObjCustomOption linePenObjOption;
extern Tk_ObjCustomOption pairsObjOption;
extern Tk_ObjCustomOption valuesObjOption;
extern Tk_ObjCustomOption xAxisObjOption;
extern Tk_ObjCustomOption yAxisObjOption;

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-activepen", "activePen", "ActivePen",
   "activeLine", -1, Tk_Offset(LineElementOptions, activePenPtr), 
   TK_OPTION_NULL_OK, &linePenObjOption, 0},
  {TK_OPTION_COLOR, "-areaforeground", "areaForeground", "AreaForeground",
   NULL, -1, Tk_Offset(LineElementOptions, fillFgColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BORDER, "-areabackground", "areaBackground", "AreaBackground",
   NULL, -1, Tk_Offset(LineElementOptions, fillBg), 
   TK_OPTION_NULL_OK, NULL, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags", 
   "all", -1, Tk_Offset(LineElementOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_COLOR, "-color", "color", "Color", 
   STD_NORMAL_FOREGROUND, -1, 
   Tk_Offset(LineElementOptions, builtinPen.traceColor),
   0, NULL, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes", 
   NULL, -1, Tk_Offset(LineElementOptions, builtinPen.traceDashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_CUSTOM, "-data", "data", "Data", 
   NULL, -1, Tk_Offset(LineElementOptions, coords),
   TK_OPTION_NULL_OK, &pairsObjOption, MAP_ITEM},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(LineElementOptions, builtinPen.errorBarColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS,"-errorbarwidth", "errorBarWidth", "ErrorBarWidth",
   "1", -1, Tk_Offset(LineElementOptions, builtinPen.errorBarLineWidth),
   0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(LineElementOptions, builtinPen.errorBarCapWidth),
   0, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill", 
   NULL, -1, Tk_Offset(LineElementOptions, builtinPen.symbol.fillColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(LineElementOptions, hide), 0, NULL, MAP_ITEM},
  {TK_OPTION_STRING, "-label", "label", "Label", 
   NULL, -1, Tk_Offset(LineElementOptions, label), 
   TK_OPTION_NULL_OK | TK_OPTION_DONT_SET_DEFAULT, NULL, MAP_ITEM},
  {TK_OPTION_RELIEF, "-legendrelief", "legendRelief", "LegendRelief",
   "flat", -1, Tk_Offset(LineElementOptions, legendRelief), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(LineElementOptions, builtinPen.traceWidth),
   0, NULL, 0},
  {TK_OPTION_CUSTOM, "-mapx", "mapX", "MapX",
   "x", -1, Tk_Offset(LineElementOptions, axes.x),
   0, &xAxisObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY",
   "y", -1, Tk_Offset(LineElementOptions, axes.y),
   0, &yAxisObjOption, MAP_ITEM},
  {TK_OPTION_INT, "-maxsymbols", "maxSymbols", "MaxSymbols",
   "0", -1, Tk_Offset(LineElementOptions, reqMaxSymbols), 0, NULL, 0},
  {TK_OPTION_COLOR, "-offdash", "offDash", "OffDash", 
   NULL, -1, Tk_Offset(LineElementOptions, builtinPen.traceOffColor),
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline", 
   NULL, -1, Tk_Offset(LineElementOptions, builtinPen.symbol.outlineColor), 
   TK_OPTION_NULL_OK, NULL,0},
  {TK_OPTION_PIXELS, "-outlinewidth", "outlineWidth", "OutlineWidth",
   "1", -1, Tk_Offset(LineElementOptions, builtinPen.symbol.outlineWidth),
   0, NULL, 0},
  {TK_OPTION_CUSTOM, "-pen", "pen", "Pen",
   NULL, -1, Tk_Offset(LineElementOptions, normalPenPtr), 
   TK_OPTION_NULL_OK, &linePenObjOption, 0},
  {TK_OPTION_PIXELS, "-pixels", "pixels", "Pixels", 
   "0.1i", -1, Tk_Offset(LineElementOptions, builtinPen.symbol.size), 
   0, NULL, MAP_ITEM},
  {TK_OPTION_DOUBLE, "-reduce", "reduce", "Reduce",
   "0", -1, Tk_Offset(LineElementOptions, rTolerance), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-scalesymbols", "scaleSymbols", "ScaleSymbols",
   "yes", -1, Tk_Offset(LineElementOptions, scaleSymbols), 
   0, NULL, (MAP_ITEM | SCALE_SYMBOL)},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(LineElementOptions, builtinPen.errorBarShow), 
   0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(LineElementOptions, builtinPen.valueShow), 
   0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-smooth", "smooth", "Smooth", 
   "linear", -1, Tk_Offset(LineElementOptions, reqSmooth), 
   0, &smoothObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-styles", "styles", "Styles",
   "", -1, Tk_Offset(LineElementOptions, stylePalette), 0, &styleObjOption, 0},
  {TK_OPTION_CUSTOM, "-symbol", "symbol", "Symbol",
   "none", -1, Tk_Offset(LineElementOptions, builtinPen.symbol), 
   0, &symbolObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-trace", "trace", "Trace",
   "both", -1, Tk_Offset(LineElementOptions, penDir), 
   0, &penDirObjOption, MAP_ITEM},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(LineElementOptions, builtinPen.valueStyle.anchor),
   0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND,-1,
   Tk_Offset(LineElementOptions, builtinPen.valueStyle.color),
   0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, 
   Tk_Offset(LineElementOptions, builtinPen.valueStyle.font),
   0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(LineElementOptions, builtinPen.valueFormat), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(LineElementOptions, builtinPen.valueStyle.angle),
   0, NULL, 0},
  {TK_OPTION_CUSTOM, "-weights", "weights", "Weights",
   NULL, -1, Tk_Offset(LineElementOptions, w), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-x", NULL, NULL, NULL, -1, 0, 0, "-xdata", 0},
  {TK_OPTION_CUSTOM, "-xdata", "xData", "XData", 
   NULL, -1, Tk_Offset(LineElementOptions, coords.x), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xerror", "xError", "XError", 
   NULL, -1, Tk_Offset(LineElementOptions, xError), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xhigh", "xHigh", "XHigh", 
   NULL, -1, Tk_Offset(LineElementOptions, xHigh), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-xlow", "xLow", "XLow", 
   NULL, -1, Tk_Offset(LineElementOptions, xLow), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-y", NULL, NULL, NULL, -1, 0, 0, "-ydata", 0},
  {TK_OPTION_CUSTOM, "-ydata", "yData", "YData", 
   NULL, -1, Tk_Offset(LineElementOptions, coords.y), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-yerror", "yError", "YError", 
   NULL, -1, Tk_Offset(LineElementOptions, yError), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-yhigh", "yHigh", "YHigh",
   NULL, -1, Tk_Offset(LineElementOptions, yHigh), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-ylow", "yLow", "YLow", 
   NULL, -1, Tk_Offset(LineElementOptions, yLow), 
   TK_OPTION_NULL_OK, &valuesObjOption, MAP_ITEM},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

Element* Blt_LineElement(Graph* graphPtr)
{
  LineElement* lePtr = (LineElement*)calloc(1, sizeof(LineElement));
  lePtr->ops = (LineElementOptions*)calloc(1, sizeof(LineElementOptions));
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;
  ops->elemPtr = (Element*)lePtr;

  lePtr->procsPtr = &lineProcs;
  ops->builtinPenPtr = &lePtr->builtinPen;
  lePtr->builtinPen.ops =&ops->builtinPen;
  lePtr->builtinPen.manageOptions =0;

  InitLinePen(graphPtr, &lePtr->builtinPen, "builtin");
  Tk_InitOptions(graphPtr->interp, (char*)&(ops->builtinPen),
		 lePtr->builtinPen.optionTable, graphPtr->tkwin);

  lePtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);

  lePtr->flags = SCALE_SYMBOL;
  return (Element*)lePtr;
}

static void DestroyLineProc(Graph* graphPtr, Element* elemPtr)
{
  LineElement* lePtr = (LineElement*)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  DestroyLinePenProc(graphPtr, (Pen *)&lePtr->builtinPen);
  if (ops->activePenPtr)
    Blt_FreePen((Pen *)ops->activePenPtr);
  if (ops->normalPenPtr)
    Blt_FreePen((Pen *)ops->normalPenPtr);

  ResetLine(lePtr);
  if (ops->stylePalette) {
    Blt_FreeStylePalette(ops->stylePalette);
    Blt_Chain_Destroy(ops->stylePalette);
  }
  if (lePtr->activeIndices)
    free(lePtr->activeIndices);

  if (lePtr->fillPts)
    free(lePtr->fillPts);

  if (lePtr->fillGC)
    Tk_FreeGC(graphPtr->display, lePtr->fillGC);
}

// Configure

static int ConfigureLineProc(Graph* graphPtr, Element *elemPtr)
{
  LineElement* lePtr = (LineElement*)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  if (ConfigureLinePenProc(graphPtr, (Pen *)&lePtr->builtinPen) != TCL_OK)
    return TCL_ERROR;

  // Point to the static normal/active pens if no external pens have been
  // selected.
  Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette);
  if (!link) {
    link = Blt_Chain_AllocLink(sizeof(LineStyle));
    Blt_Chain_LinkAfter(ops->stylePalette, link, NULL);
  } 
  LineStyle* stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
  stylePtr->penPtr = NORMALPEN(ops);

  // Set the outline GC for this pen: GCForeground is outline color.
  // GCBackground is the fill color (only used for bitmap symbols).
  unsigned long gcMask =0;
  XGCValues gcValues;
  if (ops->fillFgColor) {
    gcMask |= GCForeground;
    gcValues.foreground = ops->fillFgColor->pixel;
  }
  if (lePtr->fillBgColor) {
    gcMask |= GCBackground;
    gcValues.background = lePtr->fillBgColor->pixel;
  }
  GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lePtr->fillGC)
    Tk_FreeGC(graphPtr->display, lePtr->fillGC);
  lePtr->fillGC = newGC;

  return TCL_OK;
}

// Support

static void ResetStylePalette(Blt_Chain styles)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(styles); link;
       link = Blt_Chain_NextLink(link)) {
    LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
    stylePtr->lines.length = stylePtr->symbolPts.length = 0;
    stylePtr->xeb.length = stylePtr->yeb.length = 0;
  }
}

static int ScaleSymbol(LineElement* lePtr, int normalSize)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  double scale = 1.0;
  if (ops->scaleSymbols) {
    double xRange = (ops->axes.x->max - ops->axes.x->min);
    double yRange = (ops->axes.y->max - ops->axes.y->min);
    // Save the ranges as a baseline for future scaling
    if (lePtr->flags & SCALE_SYMBOL) {
      lePtr->xRange = xRange;
      lePtr->yRange = yRange;
      lePtr->flags &= ~SCALE_SYMBOL;
    }
    else {
      // Scale the symbol by the smallest change in the X or Y axes
      double xScale = lePtr->xRange / xRange;
      double yScale = lePtr->yRange / yRange;
      scale = MIN(xScale, yScale);
    }
  }
  int newSize = Round(normalSize * scale);

  // Don't let the size of symbols go unbounded. Both X and Win32 drawing
  // routines assume coordinates to be a signed short int.
  int maxSize = (int)MIN(lePtr->obj.graphPtr->hRange, 
		     lePtr->obj.graphPtr->vRange);
  if (newSize > maxSize)
    newSize = maxSize;

  // Make the symbol size odd so that its center is a single pixel.
  newSize |= 0x01;

  return newSize;
}

static void GetScreenPoints(Graph* graphPtr, LineElement* lePtr, 
			    MapInfo *mapPtr)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  if (!ops->coords.x || !ops->coords.y) {
    mapPtr->screenPts = NULL;
    mapPtr->nScreenPts = 0;
    mapPtr->map = NULL;
  }

  int np = NUMBEROFPOINTS(ops);
  double* x = ops->coords.x->values;
  double* y = ops->coords.y->values;
  Point2d *points = (Point2d*)malloc(sizeof(Point2d) * np);
  int* map = (int*)malloc(sizeof(int) * np);

  int count = 0;
  if (graphPtr->inverted) {
    for (int i = 0; i < np; i++) {
      if ((isfinite(x[i])) && (isfinite(y[i]))) {
	points[count].x = Blt_HMap(ops->axes.y, y[i]);
	points[count].y = Blt_VMap(ops->axes.x, x[i]);
	map[count] = i;
	count++;
      }
    }
  }
  else {
    for (int i = 0; i < np; i++) {
      if ((isfinite(x[i])) && (isfinite(y[i]))) {
	points[count].x = Blt_HMap(ops->axes.x, x[i]);
	points[count].y = Blt_VMap(ops->axes.y, y[i]);
	map[count] = i;
	count++;
      }
    }
  }
  mapPtr->screenPts = points;
  mapPtr->nScreenPts = count;
  mapPtr->map = map;
}

static void ReducePoints(MapInfo *mapPtr, double tolerance)
{
  int* simple = (int*)malloc(mapPtr->nScreenPts * sizeof(int));
  int* map = (int*)malloc(mapPtr->nScreenPts * sizeof(int));
  Point2d *screenPts = (Point2d*)malloc(mapPtr->nScreenPts * sizeof(Point2d));
  int np = Blt_SimplifyLine(mapPtr->screenPts, 0, mapPtr->nScreenPts - 1, 
			    tolerance, simple);
  for (int i = 0; i < np; i++) {
    int k;

    k = simple[i];
    screenPts[i] = mapPtr->screenPts[k];
    map[i] = mapPtr->map[k];
  }
  free(mapPtr->screenPts);
  free(mapPtr->map);
  free(simple);
  mapPtr->screenPts = screenPts;
  mapPtr->map = map;
  mapPtr->nScreenPts = np;
}

static void GenerateSteps(MapInfo *mapPtr)
{
  int newSize = ((mapPtr->nScreenPts - 1) * 2) + 1;
  Point2d *screenPts = (Point2d*)malloc(newSize * sizeof(Point2d));
  int* map = (int*)malloc(sizeof(int) * newSize);
  screenPts[0] = mapPtr->screenPts[0];
  map[0] = 0;

  int count = 1;
  for (int i = 1; i < mapPtr->nScreenPts; i++) {
    screenPts[count + 1] = mapPtr->screenPts[i];

    /* Hold last y-coordinate, use new x-coordinate */
    screenPts[count].x = screenPts[count + 1].x;
    screenPts[count].y = screenPts[count - 1].y;

    /* Use the same style for both the hold and the step points */
    map[count] = map[count + 1] = mapPtr->map[i];
    count += 2;
  }
  free(mapPtr->screenPts);
  free(mapPtr->map);
  mapPtr->map = map;
  mapPtr->screenPts = screenPts;
  mapPtr->nScreenPts = newSize;
}

static void GenerateSpline(Graph* graphPtr, LineElement* lePtr, 
			   MapInfo *mapPtr)
{
  Point2d *origPts, *iPts;
  int *map;
  int extra;
  int niPts, nOrigPts;
  int result;
  int i, j, count;

  nOrigPts = mapPtr->nScreenPts;
  origPts = mapPtr->screenPts;
  for (i = 0, j = 1; j < nOrigPts; i++, j++) {
    if (origPts[j].x <= origPts[i].x) {
      return;			/* Points are not monotonically
				 * increasing */
    }
  }
  if (((origPts[0].x > (double)graphPtr->right)) ||
      ((origPts[mapPtr->nScreenPts - 1].x < (double)graphPtr->left))) {
    return;				/* All points are clipped */
  }

  /*
   * The spline is computed in screen coordinates instead of data points so
   * that we can select the abscissas of the interpolated points from each
   * pixel horizontally across the plotting area.
   */
  extra = (graphPtr->right - graphPtr->left) + 1;
  if (extra < 1) {
    return;
  }
  niPts = nOrigPts + extra + 1;
  iPts = (Point2d*)malloc(niPts * sizeof(Point2d));
  map = (int*)malloc(sizeof(int) * niPts);
  /* Populate the x2 array with both the original X-coordinates and extra
   * X-coordinates for each horizontal pixel that the line segment
   * contains. */
  count = 0;
  for (i = 0, j = 1; j < nOrigPts; i++, j++) {

    /* Add the original x-coordinate */
    iPts[count].x = origPts[i].x;

    /* Include the starting offset of the point in the offset array */
    map[count] = mapPtr->map[i];
    count++;

    /* Is any part of the interval (line segment) in the plotting area?  */
    if ((origPts[j].x >= (double)graphPtr->left) || 
	(origPts[i].x <= (double)graphPtr->right)) {
      double x, last;

      x = origPts[i].x + 1.0;

      /*
       * Since the line segment may be partially clipped on the left or
       * right side, the points to interpolate are always interior to
       * the plotting area.
       *
       *           left			    right
       *      x1----|---------------------------|---x2
       *
       * Pick the max of the starting X-coordinate and the left edge and
       * the min of the last X-coordinate and the right edge.
       */
      x = MAX(x, (double)graphPtr->left);
      last = MIN(origPts[j].x, (double)graphPtr->right);

      /* Add the extra x-coordinates to the interval. */
      while (x < last) {
	map[count] = mapPtr->map[i];
	iPts[count++].x = x;
	x++;
      }
    }
  }
  niPts = count;
  result = 0;
  if (lePtr->smooth == PEN_SMOOTH_NATURAL) {
    result = Blt_NaturalSpline(origPts, nOrigPts, iPts, niPts);
  } else if (lePtr->smooth == PEN_SMOOTH_QUADRATIC) {
    result = Blt_QuadraticSpline(origPts, nOrigPts, iPts, niPts);
  }
  if (!result) {
    /* The spline interpolation failed.  We'll fallback to the current
     * coordinates and do no smoothing (standard line segments).  */
    lePtr->smooth = PEN_SMOOTH_LINEAR;
    free(iPts);
    free(map);
  } else {
    free(mapPtr->screenPts);
    free(mapPtr->map);
    mapPtr->map = map;
    mapPtr->screenPts = iPts;
    mapPtr->nScreenPts = niPts;
  }
}

static void GenerateParametricSpline(Graph* graphPtr, LineElement* lePtr, 
				     MapInfo *mapPtr)
{
  Region2d exts;
  Point2d *origPts, *iPts;
  int *map;
  int niPts, nOrigPts;
  int result;
  int i, j, count;

  nOrigPts = mapPtr->nScreenPts;
  origPts = mapPtr->screenPts;
  Blt_GraphExtents(graphPtr, &exts);

  /* 
   * Populate the x2 array with both the original X-coordinates and extra
   * X-coordinates for each horizontal pixel that the line segment contains.
   */
  count = 1;
  for (i = 0, j = 1; j < nOrigPts; i++, j++) {
    Point2d p, q;

    p = origPts[i];
    q = origPts[j];
    count++;
    if (Blt_LineRectClip(&exts, &p, &q)) {
      count += (int)(hypot(q.x - p.x, q.y - p.y) * 0.5);
    }
  }
  niPts = count;
  iPts = (Point2d*)malloc(niPts * sizeof(Point2d));
  map = (int*)malloc(sizeof(int) * niPts);

  /* 
   * FIXME: This is just plain wrong.  The spline should be computed
   *        and evaluated in separate steps.  This will mean breaking
   *	      up this routine since the catrom coefficients can be
   *	      independently computed for original data point.  This 
   *	      also handles the problem of allocating enough points 
   *	      since evaluation is independent of the number of points 
   *		to be evalualted.  The interpolated 
   *	      line segments should be clipped, not the original segments.
   */
  count = 0;
  for (i = 0, j = 1; j < nOrigPts; i++, j++) {
    Point2d p, q;
    double d;

    p = origPts[i];
    q = origPts[j];

    d = hypot(q.x - p.x, q.y - p.y);
    /* Add the original x-coordinate */
    iPts[count].x = (double)i;
    iPts[count].y = 0.0;

    /* Include the starting offset of the point in the offset array */
    map[count] = mapPtr->map[i];
    count++;

    /* Is any part of the interval (line segment) in the plotting
     * area?  */

    if (Blt_LineRectClip(&exts, &p, &q)) {
      double dp, dq;

      /* Distance of original point to p. */
      dp = hypot(p.x - origPts[i].x, p.y - origPts[i].y);
      /* Distance of original point to q. */
      dq = hypot(q.x - origPts[i].x, q.y - origPts[i].y);
      dp += 2.0;
      while(dp <= dq) {
	/* Point is indicated by its interval and parameter t. */
	iPts[count].x = (double)i;
	iPts[count].y =  dp / d;
	map[count] = mapPtr->map[i];
	count++;
	dp += 2.0;
      }
    }
  }
  iPts[count].x = (double)i;
  iPts[count].y = 0.0;
  map[count] = mapPtr->map[i];
  count++;
  niPts = count;
  result = 0;
  if (lePtr->smooth == PEN_SMOOTH_NATURAL) {
    result = Blt_NaturalParametricSpline(origPts, nOrigPts, &exts, 0,
					 iPts, niPts);
  } else if (lePtr->smooth == PEN_SMOOTH_CATROM) {
    result = Blt_CatromParametricSpline(origPts, nOrigPts, iPts, niPts);
  }
  if (!result) {
    /* The spline interpolation failed.  We will fall back to the current
     * coordinates and do no smoothing (standard line segments).  */
    lePtr->smooth = PEN_SMOOTH_LINEAR;
    free(iPts);
    free(map);
  } else {
    free(mapPtr->screenPts);
    free(mapPtr->map);
    mapPtr->map = map;
    mapPtr->screenPts = iPts;
    mapPtr->nScreenPts = niPts;
  }
}

static void MapSymbols(Graph* graphPtr, LineElement* lePtr, MapInfo *mapPtr)
{
  Region2d exts;
  Point2d *pp;
  int i;

  Point2d* points = (Point2d*)malloc(sizeof(Point2d) * mapPtr->nScreenPts);
  int *map = (int*)malloc(sizeof(int) * mapPtr->nScreenPts);

  Blt_GraphExtents(graphPtr, &exts);
  int count = 0;

  for (pp = mapPtr->screenPts, i = 0; i < mapPtr->nScreenPts; i++, pp++) {
    if (PointInRegion(&exts, pp->x, pp->y)) {
      points[count].x = pp->x;
      points[count].y = pp->y;
      map[count] = mapPtr->map[i];
      count++;
    }
  }
  lePtr->symbolPts.points = points;
  lePtr->symbolPts.length = count;
  lePtr->symbolPts.map = map;
}

static void MapActiveSymbols(Graph* graphPtr, LineElement* lePtr)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  if (lePtr->activePts.points) {
    free(lePtr->activePts.points);
    lePtr->activePts.points = NULL;
  }
  if (lePtr->activePts.map) {
    free(lePtr->activePts.map);
    lePtr->activePts.map = NULL;
  }

  Region2d exts;
  Blt_GraphExtents(graphPtr, &exts);

  Point2d *points = (Point2d*)malloc(sizeof(Point2d) * lePtr->nActiveIndices);
  int* map = (int*)malloc(sizeof(int) * lePtr->nActiveIndices);
  int np = NUMBEROFPOINTS(ops);
  int count = 0;
  if (ops->coords.x && ops->coords.y) {
    for (int ii=0; ii<lePtr->nActiveIndices; ii++) {
      int iPoint = lePtr->activeIndices[ii];
      if (iPoint >= np)
	continue;

      double x = ops->coords.x->values[iPoint];
      double y = ops->coords.y->values[iPoint];
      points[count] = Blt_Map2D(graphPtr, x, y, &ops->axes);
      map[count] = iPoint;
      if (PointInRegion(&exts, points[count].x, points[count].y)) {
	count++;
      }
    }
  }

  if (count > 0) {
    lePtr->activePts.points = points;
    lePtr->activePts.map = map;
  }
  else {
    free(points);
    free(map);	
  }
  lePtr->activePts.length = count;
  lePtr->flags &= ~ACTIVE_PENDING;
}

static void MergePens(LineElement* lePtr, LineStyle **styleMap)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  if (Blt_Chain_GetLength(ops->stylePalette) < 2) {
    Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette);
    LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
    stylePtr->lines.length = lePtr->lines.length;
    stylePtr->lines.segments = lePtr->lines.segments;
    stylePtr->symbolPts.length = lePtr->symbolPts.length;
    stylePtr->symbolPts.points = lePtr->symbolPts.points;
    stylePtr->xeb.length = lePtr->xeb.length;
    stylePtr->xeb.segments = lePtr->xeb.segments;
    stylePtr->yeb.length = lePtr->yeb.length;
    stylePtr->yeb.segments = lePtr->yeb.segments;
    return;
  }

  /* We have more than one style. Group line segments and points of like pen
   * styles.  */
  if (lePtr->lines.length > 0) {
    Blt_ChainLink link;
    Segment2d *sp;
    int *ip;

    Segment2d* segments = 
      (Segment2d*)malloc(lePtr->lines.length * sizeof(Segment2d));
    int* map = (int*)malloc(lePtr->lines.length * sizeof(int));
    sp = segments, ip = map;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      stylePtr->lines.segments = sp;
      for (int i = 0; i < lePtr->lines.length; i++) {
	int iData;

	iData = lePtr->lines.map[i];
	if (styleMap[iData] == stylePtr) {
	  *sp++ = lePtr->lines.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->lines.length = sp - stylePtr->lines.segments;
    }
    free(lePtr->lines.segments);
    lePtr->lines.segments = segments;
    free(lePtr->lines.map);
    lePtr->lines.map = map;
  }
  if (lePtr->symbolPts.length > 0) {
    Blt_ChainLink link;
    int *ip;
    Point2d *pp;

    Point2d* points = 
      (Point2d*)malloc(lePtr->symbolPts.length * sizeof(Point2d));
    int* map = (int*)malloc(lePtr->symbolPts.length * sizeof(int));
    pp = points, ip = map;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      stylePtr->symbolPts.points = pp;
      for (int i = 0; i < lePtr->symbolPts.length; i++) {
	int iData;

	iData = lePtr->symbolPts.map[i];
	if (styleMap[iData] == stylePtr) {
	  *pp++ = lePtr->symbolPts.points[i];
	  *ip++ = iData;
	}
      }
      stylePtr->symbolPts.length = pp - stylePtr->symbolPts.points;
    }
    free(lePtr->symbolPts.points);
    free(lePtr->symbolPts.map);
    lePtr->symbolPts.points = points;
    lePtr->symbolPts.map = map;
  }
  if (lePtr->xeb.length > 0) {
    Segment2d *sp;
    int *ip;
    Blt_ChainLink link;

    Segment2d *segments = 
      (Segment2d*)malloc(lePtr->xeb.length * sizeof(Segment2d));
    int* map = (int*)malloc(lePtr->xeb.length * sizeof(int));
    sp = segments, ip = map;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      stylePtr->xeb.segments = sp;
      for (int i = 0; i < lePtr->xeb.length; i++) {
	int iData;

	iData = lePtr->xeb.map[i];
	if (styleMap[iData] == stylePtr) {
	  *sp++ = lePtr->xeb.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->xeb.length = sp - stylePtr->xeb.segments;
    }
    free(lePtr->xeb.segments);
    free(lePtr->xeb.map);
    lePtr->xeb.segments = segments;
    lePtr->xeb.map = map;
  }
  if (lePtr->yeb.length > 0) {
    Segment2d *sp;
    int *ip;
    Blt_ChainLink link;

    Segment2d *segments = 
      (Segment2d*)malloc(lePtr->yeb.length * sizeof(Segment2d));
    int* map = (int*)malloc(lePtr->yeb.length * sizeof(int));
    sp = segments, ip = map;
    for (link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      stylePtr->yeb.segments = sp;
      for (int i = 0; i < lePtr->yeb.length; i++) {
	int iData;

	iData = lePtr->yeb.map[i];
	if (styleMap[iData] == stylePtr) {
	  *sp++ = lePtr->yeb.segments[i];
	  *ip++ = iData;
	}
      }
      stylePtr->yeb.length = sp - stylePtr->yeb.segments;
    }
    free(lePtr->yeb.segments);
    lePtr->yeb.segments = segments;
    free(lePtr->yeb.map);
    lePtr->yeb.map = map;
  }
}

#define CLIP_TOP	(1<<0)
#define CLIP_BOTTOM	(1<<1)
#define CLIP_RIGHT	(1<<2)
#define CLIP_LEFT	(1<<3)

INLINE static int OutCode(Region2d *extsPtr, Point2d *p)
{
  int code;

  code = 0;
  if (p->x > extsPtr->right) {
    code |= CLIP_RIGHT;
  } else if (p->x < extsPtr->left) {
    code |= CLIP_LEFT;
  }
  if (p->y > extsPtr->bottom) {
    code |= CLIP_BOTTOM;
  } else if (p->y < extsPtr->top) {
    code |= CLIP_TOP;
  }
  return code;
}

static int ClipSegment(Region2d *extsPtr, int code1, int code2,
		       Point2d *p, Point2d *q)
{
  int inside, outside;

  inside = ((code1 | code2) == 0);
  outside = ((code1 & code2) != 0);

  /*
   * In the worst case, we'll clip the line segment against each of the four
   * sides of the bounding rectangle.
   */
  while ((!outside) && (!inside)) {
    if (code1 == 0) {
      Point2d *tmp;
      int code;

      /* Swap pointers and out codes */
      tmp = p, p = q, q = tmp;
      code = code1, code1 = code2, code2 = code;
    }
    if (code1 & CLIP_LEFT) {
      p->y += (q->y - p->y) *
	(extsPtr->left - p->x) / (q->x - p->x);
      p->x = extsPtr->left;
    } else if (code1 & CLIP_RIGHT) {
      p->y += (q->y - p->y) *
	(extsPtr->right - p->x) / (q->x - p->x);
      p->x = extsPtr->right;
    } else if (code1 & CLIP_BOTTOM) {
      p->x += (q->x - p->x) *
	(extsPtr->bottom - p->y) / (q->y - p->y);
      p->y = extsPtr->bottom;
    } else if (code1 & CLIP_TOP) {
      p->x += (q->x - p->x) *
	(extsPtr->top - p->y) / (q->y - p->y);
      p->y = extsPtr->top;
    }
    code1 = OutCode(extsPtr, p);

    inside = ((code1 | code2) == 0);
    outside = ((code1 & code2) != 0);
  }
  return (!inside);
}

static void SaveTrace(LineElement* lePtr, int start, int length,
		      MapInfo *mapPtr)
{
  int i, j;

  bltTrace *tracePtr  = (bltTrace*)malloc(sizeof(bltTrace));
  Point2d *screenPts = (Point2d*)malloc(sizeof(Point2d) * length);
  int* map = (int*)malloc(sizeof(int) * length);

  /* Copy the screen coordinates of the trace into the point array */

  if (mapPtr->map) {
    for (i = 0, j = start; i < length; i++, j++) {
      screenPts[i].x = mapPtr->screenPts[j].x;
      screenPts[i].y = mapPtr->screenPts[j].y;
      map[i] = mapPtr->map[j];
    }
  } else {
    for (i = 0, j = start; i < length; i++, j++) {
      screenPts[i].x = mapPtr->screenPts[j].x;
      screenPts[i].y = mapPtr->screenPts[j].y;
      map[i] = j;
    }
  }
  tracePtr->screenPts.length = length;
  tracePtr->screenPts.points = screenPts;
  tracePtr->screenPts.map = map;
  tracePtr->start = start;
  if (lePtr->traces == NULL) {
    lePtr->traces = Blt_Chain_Create();
  }
  Blt_Chain_Append(lePtr->traces, tracePtr);
}

static void FreeTraces(LineElement* lePtr)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(lePtr->traces); link;
       link = Blt_Chain_NextLink(link)) {
    bltTrace *tracePtr = (bltTrace*)Blt_Chain_GetValue(link);
    free(tracePtr->screenPts.map);
    free(tracePtr->screenPts.points);
    free(tracePtr);
  }
  Blt_Chain_Destroy(lePtr->traces);
  lePtr->traces = NULL;
}

static void MapTraces(Graph* graphPtr, LineElement* lePtr, MapInfo *mapPtr)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  Region2d exts;
  Blt_GraphExtents(graphPtr, &exts);
  int count = 1;
  int code1 = OutCode(&exts, mapPtr->screenPts);
  Point2d* p = mapPtr->screenPts;
  Point2d* q = p + 1;

  int start;
  int ii;
  for (ii=1; ii<mapPtr->nScreenPts; ii++, p++, q++) {
    Point2d s;
    s.x = 0;
    s.y = 0;
    int code2 = OutCode(&exts, q);
    if (code2 != 0) {
      /* Save the coordinates of the last point, before clipping */
      s = *q;
    }
    int broken = BROKEN_TRACE(ops->penDir, p->x, q->x);
    int offscreen = ClipSegment(&exts, code1, code2, p, q);
    if (broken || offscreen) {

      /*
       * The last line segment is either totally clipped by the plotting
       * area or the x-direction is wrong, breaking the trace.  Either
       * way, save information about the last trace (if one exists),
       * discarding the current line segment
       */

      if (count > 1) {
	start = ii - count;
	SaveTrace(lePtr, start, count, mapPtr);
	count = 1;
      }
    }
    else {
      count++;		/* Add the point to the trace. */
      if (code2 != 0) {

	/*
	 * If the last point is clipped, this means that the trace is
	 * broken after this point.  Restore the original coordinate
	 * (before clipping) after saving the trace.
	 */

	start = ii - (count - 1);
	SaveTrace(lePtr, start, count, mapPtr);
	mapPtr->screenPts[ii] = s;
	count = 1;
      }
    }
    code1 = code2;
  }
  if (count > 1) {
    start = ii - count;
    SaveTrace(lePtr, start, count, mapPtr);
  }
}

static void MapFillArea(Graph* graphPtr, LineElement* lePtr, MapInfo *mapPtr)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  if (lePtr->fillPts) {
    free(lePtr->fillPts);
    lePtr->fillPts = NULL;
    lePtr->nFillPts = 0;
  }
  if (mapPtr->nScreenPts < 3)
    return;

  int np = mapPtr->nScreenPts + 3;
  Region2d exts;
  Blt_GraphExtents(graphPtr, &exts);

  Point2d* origPts = (Point2d*)malloc(sizeof(Point2d) * np);
  if (graphPtr->inverted) {
    double minX;
    int i;

    minX = (double)ops->axes.y->screenMin;
    for (i = 0; i < mapPtr->nScreenPts; i++) {
      origPts[i].x = mapPtr->screenPts[i].x + 1;
      origPts[i].y = mapPtr->screenPts[i].y;
      if (origPts[i].x < minX) {
	minX = origPts[i].x;
      }
    }	
    /* Add edges to make (if necessary) the polygon fill to the bottom of
     * plotting window */
    origPts[i].x = minX;
    origPts[i].y = origPts[i - 1].y;
    i++;
    origPts[i].x = minX;
    origPts[i].y = origPts[0].y; 
    i++;
    origPts[i] = origPts[0];
  } else {
    double maxY;
    int i;

    maxY = (double)ops->axes.y->bottom;
    for (i = 0; i < mapPtr->nScreenPts; i++) {
      origPts[i].x = mapPtr->screenPts[i].x + 1;
      origPts[i].y = mapPtr->screenPts[i].y;
      if (origPts[i].y > maxY) {
	maxY = origPts[i].y;
      }
    }	
    /* Add edges to extend the fill polygon to the bottom of plotting
     * window */
    origPts[i].x = origPts[i - 1].x;
    origPts[i].y = maxY;
    i++;
    origPts[i].x = origPts[0].x; 
    origPts[i].y = maxY;
    i++;
    origPts[i] = origPts[0];
  }

  Point2d *clipPts = (Point2d*)malloc(sizeof(Point2d) * np * 3);
  np = Blt_PolyRectClip(&exts, origPts, np - 1, clipPts);

  free(origPts);
  if (np < 3) {
    free(clipPts);
  } else {
    lePtr->fillPts = clipPts;
    lePtr->nFillPts = np;
  }
}

static void ResetLine(LineElement* lePtr)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  FreeTraces(lePtr);
  ResetStylePalette(ops->stylePalette);
  if (lePtr->symbolPts.points) {
    free(lePtr->symbolPts.points);
    lePtr->symbolPts.points = NULL;
  }

  if (lePtr->symbolPts.map)
    free(lePtr->symbolPts.map);
  lePtr->symbolPts.map = NULL;
  lePtr->symbolPts.length = 0;

  if (lePtr->lines.segments)
    free(lePtr->lines.segments);
  lePtr->lines.segments = NULL;
  lePtr->lines.length = 0;

  if (lePtr->lines.map)
    free(lePtr->lines.map);
  lePtr->lines.map = NULL;

  if (lePtr->activePts.points)
    free(lePtr->activePts.points);
  lePtr->activePts.points = NULL;
  lePtr->activePts.length = 0;

  if (lePtr->activePts.map)
    free(lePtr->activePts.map);
  lePtr->activePts.map = NULL;

  if (lePtr->xeb.segments)
    free(lePtr->xeb.segments);
  lePtr->xeb.segments = NULL;

  if (lePtr->xeb.map)
    free(lePtr->xeb.map);
  lePtr->xeb.map = NULL;
  lePtr->xeb.length = 0;

  if (lePtr->yeb.segments)
    free(lePtr->yeb.segments);
  lePtr->yeb.segments = NULL;
  lePtr->yeb.length = 0;

  if (lePtr->yeb.map)
    free(lePtr->yeb.map);
  lePtr->yeb.map = NULL;
}

static void MapErrorBars(Graph* graphPtr, LineElement* lePtr, 
			 LineStyle **styleMap)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  Region2d exts;
  Blt_GraphExtents(graphPtr, &exts);

  int n =0;
  int np = NUMBEROFPOINTS(ops);
  if (ops->coords.x && ops->coords.y) {
    if (ops->xError && (ops->xError->nValues > 0))
      n = MIN(ops->xError->nValues, np);
    else
      if (ops->xHigh && ops->xLow)
	n = MIN3(ops->xHigh->nValues, ops->xLow->nValues, np);
  }

  if (n > 0) {
    Segment2d *segPtr;
    Segment2d *errorBars;
    int *errorToData;
    int *indexPtr;
    int i;
		
    segPtr = errorBars = (Segment2d*)malloc(n * 3 * sizeof(Segment2d));
    indexPtr = errorToData = (int*)malloc(n * 3 * sizeof(int));
    for (i = 0; i < n; i++) {
      double x, y;
      double high, low;
      LineStyle *stylePtr;

      x = ops->coords.x->values[i];
      y = ops->coords.y->values[i];
      stylePtr = styleMap[i];
      if ((isfinite(x)) && (isfinite(y))) {
	if (ops->xError->nValues > 0) {
	  high = x + ops->xError->values[i];
	  low = x - ops->xError->values[i];
	} else {
	  high = ops->xHigh ? ops->xHigh->values[i] : 0;
	  low  = ops->xLow ? ops->xLow->values[i] : 0;
	}
	if ((isfinite(high)) && (isfinite(low)))  {
	  Point2d p, q;

	  p = Blt_Map2D(graphPtr, high, y, &ops->axes);
	  q = Blt_Map2D(graphPtr, low, y, &ops->axes);
	  segPtr->p = p;
	  segPtr->q = q;
	  if (Blt_LineRectClip(&exts, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Left cap */
	  segPtr->p.x = segPtr->q.x = p.x;
	  segPtr->p.y = p.y - stylePtr->errorBarCapWidth;
	  segPtr->q.y = p.y + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&exts, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Right cap */
	  segPtr->p.x = segPtr->q.x = q.x;
	  segPtr->p.y = q.y - stylePtr->errorBarCapWidth;
	  segPtr->q.y = q.y + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&exts, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	}
      }
    }
    lePtr->xeb.segments = errorBars;
    lePtr->xeb.length = segPtr - errorBars;
    lePtr->xeb.map = errorToData;
  }

  n =0;
  if (ops->coords.x && ops->coords.y) {
    if (ops->yError && (ops->yError->nValues > 0))
      n = MIN(ops->yError->nValues, np);
    else
      if (ops->yHigh && ops->yLow)
	n = MIN3(ops->yHigh->nValues, ops->yLow->nValues, np);
  }

  if (n > 0) {
    Segment2d *errorBars;
    Segment2d *segPtr;
    int *errorToData;
    int *indexPtr;
    int i;
		
    segPtr = errorBars = (Segment2d*)malloc(n * 3 * sizeof(Segment2d));
    indexPtr = errorToData = (int*)malloc(n * 3 * sizeof(int));
    for (i = 0; i < n; i++) {
      double x, y;
      double high, low;
      LineStyle *stylePtr;

      x = ops->coords.x->values[i];
      y = ops->coords.y->values[i];
      stylePtr = styleMap[i];
      if ((isfinite(x)) && (isfinite(y))) {
	if (ops->yError->nValues > 0) {
	  high = y + ops->yError->values[i];
	  low = y - ops->yError->values[i];
	} else {
	  high = ops->yHigh->values[i];
	  low = ops->yLow->values[i];
	}
	if ((isfinite(high)) && (isfinite(low)))  {
	  Point2d p, q;
		    
	  p = Blt_Map2D(graphPtr, x, high, &ops->axes);
	  q = Blt_Map2D(graphPtr, x, low, &ops->axes);
	  segPtr->p = p;
	  segPtr->q = q;
	  if (Blt_LineRectClip(&exts, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Top cap. */
	  segPtr->p.y = segPtr->q.y = p.y;
	  segPtr->p.x = p.x - stylePtr->errorBarCapWidth;
	  segPtr->q.x = p.x + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&exts, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	  /* Bottom cap. */
	  segPtr->p.y = segPtr->q.y = q.y;
	  segPtr->p.x = q.x - stylePtr->errorBarCapWidth;
	  segPtr->q.x = q.x + stylePtr->errorBarCapWidth;
	  if (Blt_LineRectClip(&exts, &segPtr->p, &segPtr->q)) {
	    segPtr++;
	    *indexPtr++ = i;
	  }
	}
      }
    }
    lePtr->yeb.segments = errorBars;
    lePtr->yeb.length = segPtr - errorBars;
    lePtr->yeb.map = errorToData;
  }
}

static void MapLineProc(Graph* graphPtr, Element *elemPtr)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;
    
  ResetLine(lePtr);
  if (!ops->coords.x || !ops->coords.y ||
      !ops->coords.x->nValues || !ops->coords.y->nValues)
    return;
  int np = NUMBEROFPOINTS(ops);

  MapInfo mi;
  GetScreenPoints(graphPtr, lePtr, &mi);
  MapSymbols(graphPtr, lePtr, &mi);

  if ((lePtr->flags & ACTIVE_PENDING) && (lePtr->nActiveIndices > 0))
    MapActiveSymbols(graphPtr, lePtr);

  // Map connecting line segments if they are to be displayed.
  lePtr->smooth = ops->reqSmooth;
  if ((np > 1) && (ops->builtinPen.traceWidth > 0)) {
    // Do smoothing if necessary.  This can extend the coordinate array,
    // so both mi.points and mi.nPoints may change.
    switch (lePtr->smooth) {
    case PEN_SMOOTH_STEP:
      GenerateSteps(&mi);
      break;

    case PEN_SMOOTH_NATURAL:
    case PEN_SMOOTH_QUADRATIC:
      // Can't interpolate with less than three points
      if (mi.nScreenPts < 3)
	lePtr->smooth = PEN_SMOOTH_LINEAR;
      else
	GenerateSpline(graphPtr, lePtr, &mi);
      break;

    case PEN_SMOOTH_CATROM:
      // Can't interpolate with less than three points
      if (mi.nScreenPts < 3)
	lePtr->smooth = PEN_SMOOTH_LINEAR;
      else
	GenerateParametricSpline(graphPtr, lePtr, &mi);
      break;

    default:
      break;
    }
    if (ops->rTolerance > 0.0) {
      ReducePoints(&mi, ops->rTolerance);
    }
    if (ops->fillBg) {
      MapFillArea(graphPtr, lePtr, &mi);
    }
    MapTraces(graphPtr, lePtr, &mi);
  }
  free(mi.screenPts);
  free(mi.map);

  // Set the symbol size of all the pen styles
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {
    LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
    LinePen* penPtr = (LinePen *)stylePtr->penPtr;
    LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;
    int size = ScaleSymbol(lePtr, penOps->symbol.size);
    stylePtr->symbolSize = size;
    stylePtr->errorBarCapWidth = (penOps->errorBarCapWidth > 0) 
      ? penOps->errorBarCapWidth : Round(size * 0.6666666);
    stylePtr->errorBarCapWidth /= 2;
  }

  LineStyle **styleMap = (LineStyle **)Blt_StyleMap((Element *)lePtr);
  if (((ops->yHigh && ops->yHigh->nValues > 0) &&
       (ops->yLow && ops->yLow->nValues > 0)) ||
      ((ops->xHigh && ops->xHigh->nValues > 0) &&
       (ops->xLow && ops->xLow->nValues > 0)) ||
      (ops->xError && ops->xError->nValues > 0) ||
      (ops->yError && ops->yError->nValues > 0)) {
    MapErrorBars(graphPtr, lePtr, styleMap);
  }

  MergePens(lePtr, styleMap);
  free(styleMap);
}

static double DistanceToLineProc(int x, int y, Point2d *p, Point2d *q,
				 Point2d *t)
{
  double right, left, top, bottom;

  *t = Blt_GetProjection(x, y, p, q);
  if (p->x > q->x) {
    right = p->x, left = q->x;
  } else {
    left = p->x, right = q->x;
  }
  if (p->y > q->y) {
    bottom = p->y, top = q->y;
  } else {
    top = p->y, bottom = q->y;
  }
  if (t->x > right) {
    t->x = right;
  } else if (t->x < left) {
    t->x = left;
  }
  if (t->y > bottom) {
    t->y = bottom;
  } else if (t->y < top) {
    t->y = top;
  }
  return hypot((t->x - x), (t->y - y));
}

static double DistanceToXProc(int x, int y, Point2d *p,	Point2d *q,
			      Point2d *t)
{
  double dx, dy;
  double d;

  if (p->x > q->x) {
    if ((x > p->x) || (x < q->x)) {
      return DBL_MAX;		/* X-coordinate outside line segment. */
    }
  } else {
    if ((x > q->x) || (x < p->x)) {
      return DBL_MAX;		/* X-coordinate outside line segment. */
    }
  }
  dx = p->x - q->x;
  dy = p->y - q->y;
  t->x = (double)x;
  if (fabs(dx) < DBL_EPSILON) {
    double d1, d2;
    /* 
     * Same X-coordinate indicates a vertical line.  Pick the closest end
     * point.
     */
    d1 = p->y - y;
    d2 = q->y - y;
    if (fabs(d1) < fabs(d2)) {
      t->y = p->y, d = d1;
    } else {
      t->y = q->y, d = d2;
    }
  } else if (fabs(dy) < DBL_EPSILON) {
    /* Horizontal line. */
    t->y = p->y, d = p->y - y;
  } else {
    double m, b;
		
    m = dy / dx;
    b = p->y - (m * p->x);
    t->y = (x * m) + b;
    d = y - t->y;
  }
  return fabs(d);
}

static double DistanceToYProc(int x, int y, Point2d *p, Point2d *q, Point2d *t)
{
  double dx, dy;
  double d;

  if (p->y > q->y) {
    if ((y > p->y) || (y < q->y)) {
      return DBL_MAX;
    }
  } else {
    if ((y > q->y) || (y < p->y)) {
      return DBL_MAX;
    }
  }
  dx = p->x - q->x;
  dy = p->y - q->y;
  t->y = y;
  if (fabs(dy) < DBL_EPSILON) {
    double d1, d2;

    /* Save Y-coordinate indicates an horizontal line. Pick the closest end
     * point. */
    d1 = p->x - x;
    d2 = q->x - x;
    if (fabs(d1) < fabs(d2)) {
      t->x = p->x, d = d1;
    } else {
      t->x = q->x, d = d2;
    }
  } else if (fabs(dx) < DBL_EPSILON) {
    /* Vertical line. */
    t->x = p->x, d = p->x - x;
  } else {
    double m, b;
	
    m = dy / dx;
    b = p->y - (m * p->x);
    t->x = (y - b) / m;
    d = x - t->x;
  }
  return fabs(d);
}

static int ClosestTrace(Graph* graphPtr, LineElement* lePtr,
			DistanceProc *distProc)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;
  ClosestSearch* searchPtr = &graphPtr->search;

  Blt_ChainLink link;
  Point2d closest;
  double dMin;
  int iClose;

  iClose = -1;			/* Suppress compiler warning. */
  dMin = searchPtr->dist;
  closest.x = closest.y = 0;		/* Suppress compiler warning. */
  for (link = Blt_Chain_FirstLink(lePtr->traces); link;
       link = Blt_Chain_NextLink(link)) {
    Point2d *p, *pend;

    bltTrace *tracePtr = (bltTrace*)Blt_Chain_GetValue(link);
    for (p = tracePtr->screenPts.points, 
	   pend = p + (tracePtr->screenPts.length - 1); p < pend; p++) {
      Point2d b;
      double d;

      d = (*distProc)(searchPtr->x, searchPtr->y, p, p + 1, &b);
      if (d < dMin) {
	closest = b;
	iClose = tracePtr->screenPts.map[p-tracePtr->screenPts.points];
	dMin = d;
      }
    }
  }
  if (dMin < searchPtr->dist) {
    searchPtr->dist = dMin;
    searchPtr->elemPtr = (Element*)lePtr;
    searchPtr->index = iClose;
    searchPtr->point = Blt_InvMap2D(graphPtr, closest.x, closest.y, &ops->axes);
    return 1;
  }
  return 0;
}

static void ClosestPoint(LineElement* lePtr, ClosestSearch *searchPtr)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  double dMin = searchPtr->dist;
  int iClose = 0;

  // Instead of testing each data point in graph coordinates, look at the
  // array of mapped screen coordinates. The advantages are
  //  1) only examine points that are visible (unclipped), and
  //  2) the computed distance is already in screen coordinates.
  int count;
  Point2d *pp;
  for (pp = lePtr->symbolPts.points, count = 0; 
       count < lePtr->symbolPts.length; count++, pp++) {
    double dx = (double)(searchPtr->x - pp->x);
    double dy = (double)(searchPtr->y - pp->y);
    double d;
    if (searchPtr->along == SEARCH_BOTH)
      d = hypot(dx, dy);
    else if (searchPtr->along == SEARCH_X)
      d = dx;
    else if (searchPtr->along == SEARCH_Y)
      d = dy;
    else
      continue;

    if (d < dMin) {
      iClose = lePtr->symbolPts.map[count];
      dMin = d;
    }
  }
  if (dMin < searchPtr->dist) {
    searchPtr->elemPtr = (Element*)lePtr;
    searchPtr->dist = dMin;
    searchPtr->index = iClose;
    searchPtr->point.x = ops->coords.x->values[iClose];
    searchPtr->point.y = ops->coords.y->values[iClose];
  }
}

static void GetLineExtentsProc(Element *elemPtr, Region2d *extsPtr)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  extsPtr->top = extsPtr->left = DBL_MAX;
  extsPtr->bottom = extsPtr->right = -DBL_MAX;

  if (!ops->coords.x || !ops->coords.y ||
      !ops->coords.x->nValues || !ops->coords.y->nValues)
    return;
  int np = NUMBEROFPOINTS(ops);

  extsPtr->right = ops->coords.x->max;
  if ((ops->coords.x->min <= 0.0) && (ops->axes.x->logScale))
    extsPtr->left = Blt_FindElemValuesMinimum(ops->coords.x, DBL_MIN);
  else
    extsPtr->left = ops->coords.x->min;

  extsPtr->bottom = ops->coords.y->max;
  if ((ops->coords.y->min <= 0.0) && (ops->axes.y->logScale))
    extsPtr->top = Blt_FindElemValuesMinimum(ops->coords.y, DBL_MIN);
  else
    extsPtr->top = ops->coords.y->min;

  // Correct the data limits for error bars
  if (ops->xError && ops->xError->nValues > 0) {
    int i;
	
    np = MIN(ops->xError->nValues, np);
    for (i = 0; i < np; i++) {
      double x;

      x = ops->coords.x->values[i] + ops->xError->values[i];
      if (x > extsPtr->right) {
	extsPtr->right = x;
      }
      x = ops->coords.x->values[i] - ops->xError->values[i];
      if (ops->axes.x->logScale) {
	// Mirror negative values, instead of ignoring them
	if (x < 0.0)
	  x = -x;
	if ((x > DBL_MIN) && (x < extsPtr->left))
	  extsPtr->left = x;
      } 
      else if (x < extsPtr->left)
	extsPtr->left = x;
    }		     
  }
  else {
    if (ops->xHigh && 
	(ops->xHigh->nValues > 0) && 
	(ops->xHigh->max > extsPtr->right)) {
      extsPtr->right = ops->xHigh->max;
    }
    if (ops->xLow && ops->xLow->nValues > 0) {
      double left;
	    
      if ((ops->xLow->min <= 0.0) && 
	  (ops->axes.x->logScale))
	left = Blt_FindElemValuesMinimum(ops->xLow, DBL_MIN);
      else
	left = ops->xLow->min;

      if (left < extsPtr->left)
	extsPtr->left = left;
    }
  }
    
  if (ops->yError && ops->yError->nValues > 0) {
    int i;
	
    np = MIN(ops->yError->nValues, np);
    for (i = 0; i < np; i++) {
      double y;

      y = ops->coords.y->values[i] + ops->yError->values[i];
      if (y > extsPtr->bottom) {
	extsPtr->bottom = y;
      }
      y = ops->coords.y->values[i] - ops->yError->values[i];
      if (ops->axes.y->logScale) {
	if (y < 0.0) {
	  y = -y;		/* Mirror negative values, instead of
				 * ignoring them. */
	}
	if ((y > DBL_MIN) && (y < extsPtr->left)) {
	  extsPtr->top = y;
	}
      } else if (y < extsPtr->top) {
	extsPtr->top = y;
      }
    }		     
  } else {
    if (ops->yHigh && 
	(ops->yHigh->nValues > 0) && 
	(ops->yHigh->max > extsPtr->bottom)) {
      extsPtr->bottom = ops->yHigh->max;
    }
    if (ops->yLow && ops->yLow->nValues > 0) {
      double top;
	    
      if ((ops->yLow->min <= 0.0) && 
	  (ops->axes.y->logScale))
	top = Blt_FindElemValuesMinimum(ops->yLow, DBL_MIN);
      else
	top = ops->yLow->min;

      if (top < extsPtr->top)
	extsPtr->top = top;
    }
  }
}

static void ClosestLineProc(Graph* graphPtr, Element *elemPtr)
{
  LineElement* lePtr = (LineElement*)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  ClosestSearch* searchPtr = &graphPtr->search;
  int mode = searchPtr->mode;
  if (mode == SEARCH_AUTO) {
    LinePen* penPtr = NORMALPEN(ops);
    LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;
    mode = SEARCH_POINTS;
    if ((NUMBEROFPOINTS(ops) > 1) && (penOps->traceWidth > 0))
      mode = SEARCH_TRACES;
  }
  if (mode == SEARCH_POINTS)
    ClosestPoint(lePtr, searchPtr);
  else {
    DistanceProc *distProc;
    if (searchPtr->along == SEARCH_X)
      distProc = DistanceToXProc;
    else if (searchPtr->along == SEARCH_Y)
      distProc = DistanceToYProc;
    else
      distProc = DistanceToLineProc;

    int found = ClosestTrace(graphPtr, lePtr, distProc);
    if ((!found) && (searchPtr->along != SEARCH_BOTH)) {
      ClosestPoint(lePtr, searchPtr);
    }
  }
}

/*
 * XDrawLines() points: XMaxRequestSize(dpy) - 3
 * XFillPolygon() points:  XMaxRequestSize(dpy) - 4
 * XDrawSegments() segments:  (XMaxRequestSize(dpy) - 3) / 2
 * XDrawRectangles() rectangles:  (XMaxRequestSize(dpy) - 3) / 2
 * XFillRectangles() rectangles:  (XMaxRequestSize(dpy) - 3) / 2
 * XDrawArcs() or XFillArcs() arcs:  (XMaxRequestSize(dpy) - 3) / 3
 */

#define MAX_DRAWLINES(d)	Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWPOLYGON(d)	Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWSEGMENTS(d)	Blt_MaxRequestSize(d, sizeof(XSegment))
#define MAX_DRAWRECTANGLES(d)	Blt_MaxRequestSize(d, sizeof(XRectangle))
#define MAX_DRAWARCS(d)		Blt_MaxRequestSize(d, sizeof(XArc))

static void DrawCircles(Display *display, Drawable drawable, 
			LineElement* lePtr, LinePen* penPtr, 
			int nSymbolPts, Point2d *symbolPts, int radius)
{
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  int count = 0;
  int s = radius + radius;
  XArc *arcs = (XArc*)malloc(nSymbolPts * sizeof(XArc));

  if (lePtr->symbolInterval > 0) {
    XArc* ap = arcs;
    Point2d *pp, *pend;
    for (pp = symbolPts, pend = pp + nSymbolPts; pp < pend; pp++) {
      if (DRAW_SYMBOL(lePtr)) {
	ap->x = Round(pp->x) - radius;
	ap->y = Round(pp->y) - radius;
	ap->width = ap->height = (unsigned short)s;
	ap->angle1 = 0;
	ap->angle2 = 23040;
	ap++, count++;
      }
      lePtr->symbolCounter++;
    }
  }
  else {
    XArc *ap = arcs;
    Point2d *pp, *pend;
    for (pp = symbolPts, pend = pp + nSymbolPts; pp < pend; pp++) {
      ap->x = Round(pp->x) - radius;
      ap->y = Round(pp->y) - radius;
      ap->width = ap->height = (unsigned short)s;
      ap->angle1 = 0;
      ap->angle2 = 23040;
      ap++;
    }
    count = nSymbolPts;
  }

  int reqSize = MAX_DRAWARCS(display);
  for (int ii=0; ii<count; ii+= reqSize) {
    int n = ((ii + reqSize) > count) ? (count - ii) : reqSize;
    if (penOps->symbol.fillGC)
      XFillArcs(display, drawable, penOps->symbol.fillGC, arcs + ii, n);

    if (penOps->symbol.outlineWidth > 0)
      XDrawArcs(display, drawable, penOps->symbol.outlineGC, arcs + ii, n);
  }

  free(arcs);
}

static void DrawSquares(Display *display, Drawable drawable, 
			LineElement* lePtr, LinePen* penPtr, 
			int nSymbolPts, Point2d *symbolPts, int r)
{
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  int count =0;
  int s = r + r;
  XRectangle *rectangles = (XRectangle*)malloc(nSymbolPts * sizeof(XRectangle));

  if (lePtr->symbolInterval > 0) {
    XRectangle* rp = rectangles;
    Point2d *pp, *pend;
    for (pp = symbolPts, pend = pp + nSymbolPts; pp < pend; pp++) {
      if (DRAW_SYMBOL(lePtr)) {
	rp->x = Round(pp->x) - r;
	rp->y = Round(pp->y) - r;
	rp->width = rp->height = (unsigned short)s;
	rp++, count++;
      }
      lePtr->symbolCounter++;
    }
  }
  else {
    XRectangle* rp = rectangles;
    Point2d *pp, *pend;
    for (pp = symbolPts, pend = pp + nSymbolPts; pp < pend; pp++) {
      rp->x = Round(pp->x) - r;
      rp->y = Round(pp->y) - r;
      rp->width = rp->height = (unsigned short)s;
      rp++;
    }
    count = nSymbolPts;
  }

  int reqSize = MAX_DRAWRECTANGLES(display) - 3;
  XRectangle *rp, *rend;
  for (rp = rectangles, rend = rp + count; rp < rend; rp += reqSize) {
    int n = rend - rp;
    if (n > reqSize)
      n = reqSize;

    if (penOps->symbol.fillGC)
      XFillRectangles(display, drawable, penOps->symbol.fillGC, rp, n);

    if (penOps->symbol.outlineWidth > 0)
      XDrawRectangles(display, drawable, penOps->symbol.outlineGC, rp, n);
  }

  free(rectangles);
}

#define SQRT_PI		1.77245385090552
#define S_RATIO		0.886226925452758
static void DrawSymbols(Graph* graphPtr, Drawable drawable,
			LineElement* lePtr, LinePen* penPtr,
			int size, int nSymbolPts, Point2d *symbolPts)
{
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  XPoint pattern[13];
  int count;

  if (size < 3) {
    if (penOps->symbol.fillGC) {
      XPoint* points = (XPoint*)malloc(nSymbolPts * sizeof(XPoint));
      XPoint* xpp = points;
      Point2d *pp, *endp;
      for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	xpp->x = Round(pp->x);
	xpp->y = Round(pp->y);
	xpp++;
      }
      XDrawPoints(graphPtr->display, drawable, penOps->symbol.fillGC, 
		  points, nSymbolPts, CoordModeOrigin);
      free(points);
    }
    return;
  }

  int r1 = (int)ceil(size * 0.5);
  int r2 = (int)ceil(size * S_RATIO * 0.5);

  switch (penOps->symbol.type) {
  case SYMBOL_NONE:
    break;

  case SYMBOL_SQUARE:
    DrawSquares(graphPtr->display, drawable, lePtr, penPtr, nSymbolPts,
		symbolPts, r2);
    break;

  case SYMBOL_CIRCLE:
    DrawCircles(graphPtr->display, drawable, lePtr, penPtr, nSymbolPts,
		symbolPts, r1);
    break;

  case SYMBOL_SPLUS:
  case SYMBOL_SCROSS:
    {
      if (penOps->symbol.type == SYMBOL_SCROSS) {
	r2 = Round((double)r2 * M_SQRT1_2);
	pattern[3].y = pattern[2].x = pattern[0].x = pattern[0].y = -r2;
	pattern[3].x = pattern[2].y = pattern[1].y = pattern[1].x = r2;
      }
      else {
	pattern[0].y = pattern[1].y = pattern[2].x = pattern[3].x = 0;
	pattern[0].x = pattern[2].y = -r2;
	pattern[1].x = pattern[3].y = r2;
      }

      XSegment *segments = (XSegment*)malloc(nSymbolPts * 2 * sizeof(XSegment));
      if (lePtr->symbolInterval > 0) {
	XSegment* sp = segments;
	count = 0;
	Point2d *pp, *endp;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  if (DRAW_SYMBOL(lePtr)) {
	    int rndx, rndy;
	    rndx = Round(pp->x), rndy = Round(pp->y);
	    sp->x1 = pattern[0].x + rndx;
	    sp->y1 = pattern[0].y + rndy;
	    sp->x2 = pattern[1].x + rndx;
	    sp->y2 = pattern[1].y + rndy;
	    sp++;
	    sp->x1 = pattern[2].x + rndx;
	    sp->y1 = pattern[2].y + rndy;
	    sp->x2 = pattern[3].x + rndx;
	    sp->y2 = pattern[3].y + rndy;
	    sp++;
	    count++;
	  }
	  lePtr->symbolCounter++;
	}
      }
      else {
	XSegment* sp = segments;
	count = nSymbolPts;
	Point2d *pp, *endp;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int rndx = Round(pp->x);
	  int rndy = Round(pp->y);
	  sp->x1 = pattern[0].x + rndx;
	  sp->y1 = pattern[0].y + rndy;
	  sp->x2 = pattern[1].x + rndx;
	  sp->y2 = pattern[1].y + rndy;
	  sp++;
	  sp->x1 = pattern[2].x + rndx;
	  sp->y1 = pattern[2].y + rndy;
	  sp->x2 = pattern[3].x + rndx;
	  sp->y2 = pattern[3].y + rndy;
	  sp++;
	}
      }
      int nSegs = count * 2;
      // Always draw skinny symbols regardless of the outline width
      int reqSize = MAX_DRAWSEGMENTS(graphPtr->display);
      for (int ii=0; ii<nSegs; ii+=reqSize) {
	int chunk = ((ii + reqSize) > nSegs) ? (nSegs - ii) : reqSize;
	XDrawSegments(graphPtr->display, drawable, 
		      penOps->symbol.outlineGC, segments + ii, chunk);
      }
      free(segments);
    }
    break;

  case SYMBOL_PLUS:
  case SYMBOL_CROSS:
    {
      XPoint *polygon;
      int d;			/* Small delta for cross/plus
				 * thickness */

      d = (r2 / 3);

      /*
       *
       *          2   3       The plus/cross symbol is a closed polygon
       *                      of 12 points. The diagram to the left
       *    0,12  1   4    5  represents the positions of the points
       *           x,y        which are computed below. The extra
       *     11  10   7    6  (thirteenth) point connects the first and
       *                      last points.
       *          9   8
       */

      pattern[0].x = pattern[11].x = pattern[12].x = -r2;
      pattern[2].x = pattern[1].x = pattern[10].x = pattern[9].x = -d;
      pattern[3].x = pattern[4].x = pattern[7].x = pattern[8].x = d;
      pattern[5].x = pattern[6].x = r2;
      pattern[2].y = pattern[3].y = -r2;
      pattern[0].y = pattern[1].y = pattern[4].y = pattern[5].y =
	pattern[12].y = -d;
      pattern[11].y = pattern[10].y = pattern[7].y = pattern[6].y = d;
      pattern[9].y = pattern[8].y = r2;

      if (penOps->symbol.type == SYMBOL_CROSS) {
	/* For the cross symbol, rotate the points by 45 degrees. */
	for (int ii=0; ii<12; ii++) {
	  double dx = (double)pattern[ii].x * M_SQRT1_2;
	  double dy = (double)pattern[ii].y * M_SQRT1_2;
	  pattern[ii].x = Round(dx - dy);
	  pattern[ii].y = Round(dx + dy);
	}
	pattern[12] = pattern[0];
      }
      polygon = (XPoint*)malloc(nSymbolPts * 13 * sizeof(XPoint));
      if (lePtr->symbolInterval > 0) {
	count = 0;
	XPoint* xpp = polygon;
	Point2d *pp, *endp;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  if (DRAW_SYMBOL(lePtr)) {
	    int rndx = Round(pp->x);
	    int rndy = Round(pp->y);
	    for (int ii=0; ii<13; ii++) {
	      xpp->x = pattern[ii].x + rndx;
	      xpp->y = pattern[ii].y + rndy;
	      xpp++;
	    }
	    count++;
	  }
	  lePtr->symbolCounter++;
	}
      }
      else {
	XPoint* xpp = polygon;
	Point2d *pp, *endp;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int rndx = Round(pp->x);
	  int rndy = Round(pp->y);
	  for (int ii=0; ii<13; ii++) {
	    xpp->x = pattern[ii].x + rndx;
	    xpp->y = pattern[ii].y + rndy;
	    xpp++;
	  }
	}
	count = nSymbolPts;
      }
      if (penOps->symbol.fillGC) {
	int ii;
	XPoint *xpp;
	for (xpp = polygon, ii = 0; ii<count; ii++, xpp += 13)
	  XFillPolygon(graphPtr->display, drawable, 
		       penOps->symbol.fillGC, xpp, 13, Complex, 
		       CoordModeOrigin);
      }
      if (penOps->symbol.outlineWidth > 0) {
	int ii;
	XPoint *xpp;
	for (xpp = polygon, ii=0; ii<count; ii++, xpp += 13)
	  XDrawLines(graphPtr->display, drawable, 
		     penOps->symbol.outlineGC, xpp, 13, CoordModeOrigin);
      }
      free(polygon);
    }
    break;

  case SYMBOL_DIAMOND:
    {
      XPoint *polygon;

      /*
       *
       *                      The plus symbol is a closed polygon
       *            1         of 4 points. The diagram to the left
       *                      represents the positions of the points
       *       0,4 x,y  2     which are computed below. The extra
       *                      (fifth) point connects the first and
       *            3         last points.
       *
       */
      pattern[1].y = pattern[0].x = -r1;
      pattern[2].y = pattern[3].x = pattern[0].y = pattern[1].x = 0;
      pattern[3].y = pattern[2].x = r1;
      pattern[4] = pattern[0];

      polygon = (XPoint*)malloc(nSymbolPts * 5 * sizeof(XPoint));
      if (lePtr->symbolInterval > 0) {
	Point2d *pp, *endp;
	XPoint *xpp;

	xpp = polygon;
	count = 0;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int i;

	  if (DRAW_SYMBOL(lePtr)) {
	    int rndx, rndy;
			
	    rndx = Round(pp->x), rndy = Round(pp->y);
	    for (i = 0; i < 5; i++) {
	      xpp->x = pattern[i].x + rndx;
	      xpp->y = pattern[i].y + rndy;
	      xpp++;
	    }
	    count++;
	  }
	  lePtr->symbolCounter++;
	}
      } else {
	Point2d *pp, *endp;
	XPoint *xpp;

	xpp = polygon;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int i;
	  int rndx, rndy;
			
	  rndx = Round(pp->x), rndy = Round(pp->y);
	  for (i = 0; i < 5; i++) {
	    xpp->x = pattern[i].x + rndx;
	    xpp->y = pattern[i].y + rndy;
	    xpp++;
	  }
	}
	count = nSymbolPts;
      }
      if (penOps->symbol.fillGC) {
	XPoint *xpp;
	int i;

	for (xpp = polygon, i = 0; i < count; i++, xpp += 5)
	  XFillPolygon(graphPtr->display, drawable, 
		       penOps->symbol.fillGC, xpp, 5, Convex, CoordModeOrigin);
      }
      if (penOps->symbol.outlineWidth > 0) {
	XPoint *xpp;
	int i;

	for (xpp = polygon, i = 0; i < count; i++, xpp += 5) {
	  XDrawLines(graphPtr->display, drawable, 
		     penOps->symbol.outlineGC, xpp, 5, CoordModeOrigin);
	}
      }
      free(polygon);
    }
    break;

  case SYMBOL_TRIANGLE:
  case SYMBOL_ARROW:
    {
      XPoint *polygon;
      double b;
      int b2, h1, h2;
#define H_RATIO		1.1663402261671607
#define B_RATIO		1.3467736870885982
#define TAN30		0.57735026918962573
#define COS30		0.86602540378443871

      b = Round(size * B_RATIO * 0.7);
      b2 = Round(b * 0.5);
      h2 = Round(TAN30 * b2);
      h1 = Round(b2 / COS30);
      /*
       *
       *                      The triangle symbol is a closed polygon
       *           0,3         of 3 points. The diagram to the left
       *                      represents the positions of the points
       *           x,y        which are computed below. The extra
       *                      (fourth) point connects the first and
       *      2           1   last points.
       *
       */

      if (penOps->symbol.type == SYMBOL_ARROW) {
	pattern[3].x = pattern[0].x = 0;
	pattern[3].y = pattern[0].y = h1;
	pattern[1].x = b2;
	pattern[2].y = pattern[1].y = -h2;
	pattern[2].x = -b2;
      } else {
	pattern[3].x = pattern[0].x = 0;
	pattern[3].y = pattern[0].y = -h1;
	pattern[1].x = b2;
	pattern[2].y = pattern[1].y = h2;
	pattern[2].x = -b2;
      }
      polygon = (XPoint*)malloc(nSymbolPts * 4 * sizeof(XPoint));
      if (lePtr->symbolInterval > 0) {
	Point2d *pp, *endp;
	XPoint *xpp;

	xpp = polygon;
	count = 0;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int i;

	  if (DRAW_SYMBOL(lePtr)) {
	    int rndx, rndy;

	    rndx = Round(pp->x), rndy = Round(pp->y);
	    for (i = 0; i < 4; i++) {
	      xpp->x = pattern[i].x + rndx;
	      xpp->y = pattern[i].y + rndy;
	      xpp++;
	    }
	    count++;
	  }
	  lePtr->symbolCounter++;
	}
      } else {
	Point2d *pp, *endp;
	XPoint *xpp;

	xpp = polygon;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int i;
	  int rndx, rndy;

	  rndx = Round(pp->x), rndy = Round(pp->y);
	  for (i = 0; i < 4; i++) {
	    xpp->x = pattern[i].x + rndx;
	    xpp->y = pattern[i].y + rndy;
	    xpp++;
	  }
	}
	count = nSymbolPts;
      }
      if (penOps->symbol.fillGC) {
	XPoint *xpp;
	int i;

	xpp = polygon;
	for (xpp = polygon, i = 0; i < count; i++, xpp += 4)
	  XFillPolygon(graphPtr->display, drawable, 
		       penOps->symbol.fillGC, xpp, 4, Convex, CoordModeOrigin);
      }
      if (penOps->symbol.outlineWidth > 0) {
	XPoint *xpp;
	int i;

	xpp = polygon;
	for (xpp = polygon, i = 0; i < count; i++, xpp += 4) {
	  XDrawLines(graphPtr->display, drawable, 
		     penOps->symbol.outlineGC, xpp, 4, CoordModeOrigin);
	}
      }
      free(polygon);
    }
    break;

  case SYMBOL_BITMAP:
    {
      int w, h, bw, bh;
      double scale, sx, sy;
      int dx, dy;

      Tk_SizeOfBitmap(graphPtr->display, penOps->symbol.bitmap, &w, &h);

      // Compute the size of the scaled bitmap.  Stretch the bitmap to fit
      // a nxn bounding box.
      sx = (double)size / (double)w;
      sy = (double)size / (double)h;
      scale = MIN(sx, sy);
      bw = (int)(w * scale);
      bh = (int)(h * scale);

      XSetClipMask(graphPtr->display, penOps->symbol.outlineGC, None);
      if (penOps->symbol.mask != None)
	XSetClipMask(graphPtr->display, penOps->symbol.outlineGC,
		     penOps->symbol.mask);

      if (penOps->symbol.fillGC == NULL)
	XSetClipMask(graphPtr->display, penOps->symbol.outlineGC, 
		     penOps->symbol.bitmap);

      dx = bw / 2;
      dy = bh / 2;

      if (lePtr->symbolInterval > 0) {
	Point2d *pp, *endp;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  if (DRAW_SYMBOL(lePtr)) {
	    int x = Round(pp->x) - dx;
	    int y = Round(pp->y) - dy;
	    if ((penOps->symbol.fillGC == NULL) || 
		(penOps->symbol.mask != None))
	      XSetClipOrigin(graphPtr->display,
			     penOps->symbol.outlineGC, x, y);
	    XCopyPlane(graphPtr->display, penOps->symbol.bitmap, drawable,
		       penOps->symbol.outlineGC, 0, 0, bw, bh, x, y, 1);
	  }
	  lePtr->symbolCounter++;
	}
      }
      else {
	Point2d *pp, *endp;
	for (pp = symbolPts, endp = pp + nSymbolPts; pp < endp; pp++) {
	  int x = Round(pp->x) - dx;
	  int y = Round(pp->y) - dy;
	  if ((penOps->symbol.fillGC == NULL) || 
	      (penOps->symbol.mask != None))
	    XSetClipOrigin(graphPtr->display, 
			   penOps->symbol.outlineGC, x, y);
	  XCopyPlane(graphPtr->display, penOps->symbol.bitmap, drawable,
		     penOps->symbol.outlineGC, 0, 0, bw, bh, x, y, 1);
	}
      }
    }
    break;
  }
}

static void DrawSymbolProc(Graph* graphPtr, Drawable drawable,
			   Element *elemPtr, int x, int y, int size)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  LinePen* penPtr = NORMALPEN(ops);
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  if (penOps->traceWidth > 0) {
    /*
     * Draw an extra line offset by one pixel from the previous to give a
     * thicker appearance.  This is only for the legend entry.  This routine
     * is never called for drawing the actual line segments.
     */
    XDrawLine(graphPtr->display, drawable, penPtr->traceGC, x - size, y, 
	      x + size, y);
    XDrawLine(graphPtr->display, drawable, penPtr->traceGC, x - size, y + 1,
	      x + size, y + 1);
  }
  if (penOps->symbol.type != SYMBOL_NONE) {
    Point2d point;

    point.x = x, point.y = y;
    DrawSymbols(graphPtr, drawable, lePtr, penPtr, size, 1, &point);
  }
}

static void DrawTraces(Graph* graphPtr, Drawable drawable, 
		       LineElement* lePtr, LinePen* penPtr)
{
  Blt_ChainLink link;
  int np;

  np = Blt_MaxRequestSize(graphPtr->display, sizeof(XPoint)) - 1;
  XPoint *points = (XPoint*)malloc((np + 1) * sizeof(XPoint));
	    
  for (link = Blt_Chain_FirstLink(lePtr->traces); link;
       link = Blt_Chain_NextLink(link)) {
    XPoint *xpp;
    int remaining, count;
    int n;

    bltTrace *tracePtr = (bltTrace*)Blt_Chain_GetValue(link);

    /*
     * If the trace has to be split into separate XDrawLines calls, then the
     * end point of the current trace is also the starting point of the new
     * split.
     */
    /* Step 1. Convert and draw the first section of the trace.
     *	   It may contain the entire trace. */

    n = MIN(np, tracePtr->screenPts.length); 
    for (xpp = points, count = 0; count < n; count++, xpp++) {
      xpp->x = Round(tracePtr->screenPts.points[count].x);
      xpp->y = Round(tracePtr->screenPts.points[count].y);
    }
    XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points, 
	       count, CoordModeOrigin);

    /* Step 2. Next handle any full-size chunks left. */

    while ((count + np) < tracePtr->screenPts.length) {
      int j;

      /* Start with the last point of the previous trace. */
      points[0].x = points[np - 1].x;
      points[0].y = points[np - 1].y;
	    
      for (xpp = points + 1, j = 0; j < np; j++, count++, xpp++) {
	xpp->x = Round(tracePtr->screenPts.points[count].x);
	xpp->y = Round(tracePtr->screenPts.points[count].y);
      }
      XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points, 
		 np + 1, CoordModeOrigin);
    }
	
    /* Step 3. Convert and draw the remaining points. */

    remaining = tracePtr->screenPts.length - count;
    if (remaining > 0) {
      /* Start with the last point of the previous trace. */
      points[0].x = points[np - 1].x;
      points[0].y = points[np - 1].y;
      for (xpp = points + 1; count < tracePtr->screenPts.length; count++, 
	     xpp++) {
	xpp->x = Round(tracePtr->screenPts.points[count].x);
	xpp->y = Round(tracePtr->screenPts.points[count].y);
      }	    
      XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points, 
		 remaining + 1, CoordModeOrigin);
    }
  }
  free(points);
}

static void DrawValues(Graph* graphPtr, Drawable drawable, 
		       LineElement* lePtr, LinePen* penPtr, 
		       int length, Point2d *points, int *map)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  Point2d *pp, *endp;
  double *xval, *yval;
  const char* fmt;
  char string[TCL_DOUBLE_SPACE * 2 + 2];
  int count;
    
  fmt = penOps->valueFormat;
  if (fmt == NULL)
    fmt = "%g";

  count = 0;
  xval = ops->coords.x->values, yval = ops->coords.y->values;

  // be sure to update style->gc, things might have changed
  penOps->valueStyle.flags |= UPDATE_GC;
  for (pp = points, endp = points + length; pp < endp; pp++) {
    double x, y;

    x = xval[map[count]];
    y = yval[map[count]];
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
    Blt_DrawText(graphPtr->tkwin, drawable, string, &penOps->valueStyle, 
		 Round(pp->x), Round(pp->y));
  } 
}

static void DrawActiveLineProc(Graph* graphPtr, Drawable drawable, 
			       Element *elemPtr)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;
  LinePen* penPtr = (LinePen *)ops->activePenPtr;

  if (!penPtr)
    return;
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  int symbolSize = ScaleSymbol(lePtr, penOps->symbol.size);

  /* 
   * nActiveIndices 
   *	  > 0		Some points are active.  Uses activeArr.
   *	  < 0		All points are active.
   *    == 0		No points are active.
   */
  if (lePtr->nActiveIndices > 0) {
    if (lePtr->flags & ACTIVE_PENDING) {
      MapActiveSymbols(graphPtr, lePtr);
    }
    if (penOps->symbol.type != SYMBOL_NONE) {
      DrawSymbols(graphPtr, drawable, lePtr, penPtr, symbolSize,
		  lePtr->activePts.length, lePtr->activePts.points);
    }
    if (penOps->valueShow != SHOW_NONE) {
      DrawValues(graphPtr, drawable, lePtr, penPtr, 
		 lePtr->activePts.length,
		 lePtr->activePts.points, 
		 lePtr->activePts.map);
    }
  } else if (lePtr->nActiveIndices < 0) { 
    if (penOps->traceWidth > 0) {
      if (lePtr->lines.length > 0) {
	Blt_Draw2DSegments(graphPtr->display, drawable, 
			   penPtr->traceGC, lePtr->lines.segments, 
			   lePtr->lines.length);
      } else if (Blt_Chain_GetLength(lePtr->traces) > 0) {
	DrawTraces(graphPtr, drawable, lePtr, penPtr);
      }
    }
    if (penOps->symbol.type != SYMBOL_NONE) {
      DrawSymbols(graphPtr, drawable, lePtr, penPtr, symbolSize,
		  lePtr->symbolPts.length, lePtr->symbolPts.points);
    }
    if (penOps->valueShow != SHOW_NONE) {
      DrawValues(graphPtr, drawable, lePtr, penPtr, 
		 lePtr->symbolPts.length, lePtr->symbolPts.points, 
		 lePtr->symbolPts.map);
    }
  }
}

static void DrawNormalLineProc(Graph* graphPtr, Drawable drawable, 
			       Element *elemPtr)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  // Fill area under the curve
  if (lePtr->fillPts) {
    XPoint *points = (XPoint*)malloc(sizeof(XPoint) * lePtr->nFillPts);

    unsigned int count =0;
    Point2d *endp, *pp;
    for (pp = lePtr->fillPts, endp = pp + lePtr->nFillPts; 
	 pp < endp; pp++) {
      points[count].x = Round(pp->x);
      points[count].y = Round(pp->y);
      count++;
    }
    if (ops->fillBg)
      Tk_Fill3DPolygon(graphPtr->tkwin, drawable, ops->fillBg, points, 
		       lePtr->nFillPts, 0, TK_RELIEF_FLAT);
    free(points);
  }

  // Lines: stripchart segments or graph traces
  if (lePtr->lines.length > 0) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {

      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      LinePen* penPtr = (LinePen *)stylePtr->penPtr;
      LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;
      if ((stylePtr->lines.length > 0) && 
	  (penOps->errorBarLineWidth > 0)) {
	Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->traceGC,
			   stylePtr->lines.segments, stylePtr->lines.length);
      }
    }
  } 
  else {
    LinePen* penPtr = NORMALPEN(ops);
    LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

    if ((Blt_Chain_GetLength(lePtr->traces) > 0) && 
	(penOps->traceWidth > 0)) {
      DrawTraces(graphPtr, drawable, lePtr, penPtr);
    }
  }

  if (ops->reqMaxSymbols > 0) {
    int total = 0;
    for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      total += stylePtr->symbolPts.length;
    }
    lePtr->symbolInterval = total / ops->reqMaxSymbols;
    lePtr->symbolCounter = 0;
  }

  // Symbols, error bars, values

  unsigned int count =0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {
    LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
    LinePen* penPtr = (LinePen *)stylePtr->penPtr;
    LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

    if ((stylePtr->xeb.length > 0) && (penOps->errorBarShow & SHOW_X))
      Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->errorBarGC, 
			 stylePtr->xeb.segments, stylePtr->xeb.length);

    if ((stylePtr->yeb.length > 0) && (penOps->errorBarShow & SHOW_Y))
      Blt_Draw2DSegments(graphPtr->display, drawable, penPtr->errorBarGC, 
			 stylePtr->yeb.segments, stylePtr->yeb.length);

    if ((stylePtr->symbolPts.length > 0) && 
	(penOps->symbol.type != SYMBOL_NONE))
      DrawSymbols(graphPtr, drawable, lePtr, penPtr, 
		  stylePtr->symbolSize, stylePtr->symbolPts.length, 
		  stylePtr->symbolPts.points);

    if (penOps->valueShow != SHOW_NONE)
      DrawValues(graphPtr, drawable, lePtr, penPtr, 
		 stylePtr->symbolPts.length, stylePtr->symbolPts.points, 
		 lePtr->symbolPts.map + count);

    count += stylePtr->symbolPts.length;
  }
  lePtr->symbolInterval = 0;
}

static void GetSymbolPostScriptInfo(Graph* graphPtr, Blt_Ps ps,
				    LinePen* penPtr, int size)
{
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  /* Set line and foreground attributes */
  XColor* fillColor = penOps->symbol.fillColor;
  if (!fillColor)
    fillColor = penOps->traceColor;

  XColor* outlineColor = penOps->symbol.outlineColor;
  if (!outlineColor)
    outlineColor = penOps->traceColor;

  if (penOps->symbol.type == SYMBOL_NONE)
    Blt_Ps_XSetLineAttributes(ps, penOps->traceColor, penOps->traceWidth + 2,
			      &penOps->traceDashes, CapButt, JoinMiter);
  else {
    Blt_Ps_XSetLineWidth(ps, penOps->symbol.outlineWidth);
    Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
  }

  /*
   * Build a PostScript procedure to draw the symbols.  For bitmaps, paint
   * both the bitmap and its mask. Otherwise fill and stroke the path formed
   * already.
   */
  Blt_Ps_Append(ps, "\n/DrawSymbolProc {\n");
  switch (penOps->symbol.type) {
  case SYMBOL_NONE:
    break;				/* Do nothing */
  case SYMBOL_BITMAP:
    {
      int w, h;
      double sx, sy, scale;

      /*
       * Compute how much to scale the bitmap.  Don't let the scaled
       * bitmap exceed the bounding square for the symbol.
       */
      Tk_SizeOfBitmap(graphPtr->display, penOps->symbol.bitmap, &w, &h);
      sx = (double)size / (double)w;
      sy = (double)size / (double)h;
      scale = MIN(sx, sy);

      if (penOps->symbol.mask != None) {
	Blt_Ps_VarAppend(ps, "\n  % Bitmap mask is \"",
			 Tk_NameOfBitmap(graphPtr->display, penOps->symbol.mask),
			 "\"\n\n  ", NULL);
	Blt_Ps_XSetBackground(ps, fillColor);
	Blt_Ps_DrawBitmap(ps, graphPtr->display, penOps->symbol.mask, 
			  scale, scale);
      }
      Blt_Ps_VarAppend(ps, "\n  % Bitmap symbol is \"",
		       Tk_NameOfBitmap(graphPtr->display, penOps->symbol.bitmap),
		       "\"\n\n  ", NULL);
      Blt_Ps_XSetForeground(ps, outlineColor);
      Blt_Ps_DrawBitmap(ps, graphPtr->display, penOps->symbol.bitmap, 
			scale, scale);
    }
    break;
  default:
    Blt_Ps_Append(ps, "  ");
    Blt_Ps_XSetBackground(ps, fillColor);
    Blt_Ps_Append(ps, "  gsave fill grestore\n");

    if (penOps->symbol.outlineWidth > 0) {
      Blt_Ps_Append(ps, "  ");
      Blt_Ps_XSetForeground(ps, outlineColor);
      Blt_Ps_Append(ps, "  stroke\n");
    }
    break;
  }
  Blt_Ps_Append(ps, "} def\n\n");
}

static void SymbolsToPostScript(Graph* graphPtr, Blt_Ps ps, LinePen* penPtr,
				int size, int nSymbolPts, Point2d *symbolPts)
{
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  double symbolSize;
  static const char* symbolMacros[] =
    {
      "Li", "Sq", "Ci", "Di", "Pl", "Cr", "Sp", "Sc", "Tr", "Ar", "Bm", NULL
    };
  GetSymbolPostScriptInfo(graphPtr, ps, penPtr, size);

  symbolSize = (double)size;
  switch (penOps->symbol.type) {
  case SYMBOL_SQUARE:
  case SYMBOL_CROSS:
  case SYMBOL_PLUS:
  case SYMBOL_SCROSS:
  case SYMBOL_SPLUS:
    symbolSize = (double)Round(size * S_RATIO);
    break;
  case SYMBOL_TRIANGLE:
  case SYMBOL_ARROW:
    symbolSize = (double)Round(size * 0.7);
    break;
  case SYMBOL_DIAMOND:
    symbolSize = (double)Round(size * M_SQRT1_2);
    break;

  default:
    break;
  }
  {
    Point2d *pp, *endp;

    for (pp = symbolPts, endp = symbolPts + nSymbolPts; pp < endp; pp++) {
      Blt_Ps_Format(ps, "%g %g %g %s\n", pp->x, pp->y, 
		    symbolSize, symbolMacros[penOps->symbol.type]);
    }
  }
}

static void SymbolToPostScriptProc(Graph* graphPtr, Blt_Ps ps,
				   Element *elemPtr, double x, double y,
				   int size)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  LinePen* penPtr = NORMALPEN(ops);
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  if (penOps->traceWidth > 0) {
    /*
     * Draw an extra line offset by one pixel from the previous to give a
     * thicker appearance.  This is only for the legend entry.  This routine
     * is never called for drawing the actual line segments.
     */
    Blt_Ps_XSetLineAttributes(ps, penOps->traceColor, penOps->traceWidth, 
			      &penOps->traceDashes, CapButt, JoinMiter);
    Blt_Ps_Format(ps, "%g %g %d Li\n", x, y, size + size);
  }
  if (penOps->symbol.type != SYMBOL_NONE) {
    Point2d point;

    point.x = x, point.y = y;
    SymbolsToPostScript(graphPtr, ps, penPtr, size, 1, &point);
  }
}

static void SetLineAttributes(Blt_Ps ps, LinePen* penPtr)
{
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  /* Set the attributes of the line (color, dashes, linewidth) */
  Blt_Ps_XSetLineAttributes(ps, penOps->traceColor, penOps->traceWidth, 
			    &penOps->traceDashes, CapButt, JoinMiter);
  if ((LineIsDashed(penOps->traceDashes)) && 
      (penOps->traceOffColor)) {
    Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
    Blt_Ps_XSetBackground(ps, penOps->traceOffColor);
    Blt_Ps_Append(ps, "    ");
    Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
    Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
  } else {
    Blt_Ps_Append(ps, "/DashesProc {} def\n");
  }
}

static void TracesToPostScript(Blt_Ps ps, LineElement* lePtr, LinePen* penPtr)
{
  Blt_ChainLink link;

  SetLineAttributes(ps, penPtr);
  for (link = Blt_Chain_FirstLink(lePtr->traces); link;
       link = Blt_Chain_NextLink(link)) {
    bltTrace *tracePtr = (bltTrace*)Blt_Chain_GetValue(link);
    if (tracePtr->screenPts.length > 0) {
      Blt_Ps_Append(ps, "% start trace\n");
      Blt_Ps_DrawPolyline(ps, tracePtr->screenPts.points, 
			  tracePtr->screenPts.length);
      Blt_Ps_Append(ps, "% end trace\n");
    }
  }
}

static void ValuesToPostScript(Blt_Ps ps, LineElement* lePtr, LinePen* penPtr,
			       int nSymbolPts, Point2d *symbolPts, 
			       int *pointToData)
{
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  const char* fmt = penOps->valueFormat;
  if (fmt == NULL)
    fmt = "%g";

  int count = 0;
  Point2d *pp, *endp;
  for (pp = symbolPts, endp = symbolPts + nSymbolPts; pp < endp; pp++) {
    double x, y;

    x = ops->coords.x->values[pointToData[count]];
    y = ops->coords.y->values[pointToData[count]];
    count++;

    char string[TCL_DOUBLE_SPACE * 2 + 2];
    if (penOps->valueShow == SHOW_X) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x); 
    } else if (penOps->valueShow == SHOW_Y) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, y); 
    } else if (penOps->valueShow == SHOW_BOTH) {
      sprintf_s(string, TCL_DOUBLE_SPACE, fmt, x);
      strcat(string, ",");
      sprintf_s(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
    }
    Blt_Ps_DrawText(ps, string, &penOps->valueStyle, pp->x, pp->y);
  } 
}

static void ActiveLineToPostScriptProc(Graph* graphPtr, Blt_Ps ps, 
				       Element *elemPtr)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  LinePen* penPtr = (LinePen *)ops->activePenPtr;
  if (!penPtr)
    return;
  LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

  int symbolSize = ScaleSymbol(lePtr, penOps->symbol.size);
  if (lePtr->nActiveIndices > 0) {
    if (lePtr->flags & ACTIVE_PENDING) {
      MapActiveSymbols(graphPtr, lePtr);
    }
    if (penOps->symbol.type != SYMBOL_NONE) {
      SymbolsToPostScript(graphPtr, ps, penPtr, symbolSize,
			  lePtr->activePts.length, lePtr->activePts.points);
    }
    if (penOps->valueShow != SHOW_NONE) {
      ValuesToPostScript(ps, lePtr, penPtr, lePtr->activePts.length,
			 lePtr->activePts.points, lePtr->activePts.map);
    }
  }
  else if (lePtr->nActiveIndices < 0) {
    if (penOps->traceWidth > 0) {
      if (lePtr->lines.length > 0) {
	SetLineAttributes(ps, penPtr);
	Blt_Ps_Draw2DSegments(ps, lePtr->lines.segments, 
			      lePtr->lines.length);
      }
      if (Blt_Chain_GetLength(lePtr->traces) > 0) {
	TracesToPostScript(ps, lePtr, (LinePen *)penPtr);
      }
    }
    if (penOps->symbol.type != SYMBOL_NONE) {
      SymbolsToPostScript(graphPtr, ps, penPtr, symbolSize,
			  lePtr->symbolPts.length, lePtr->symbolPts.points);
    }
    if (penOps->valueShow != SHOW_NONE) {
      ValuesToPostScript(ps, lePtr, penPtr, lePtr->symbolPts.length, 
			 lePtr->symbolPts.points, lePtr->symbolPts.map);
    }
  }
}

static void NormalLineToPostScriptProc(Graph* graphPtr, Blt_Ps ps, 
				       Element *elemPtr)
{
  LineElement* lePtr = (LineElement *)elemPtr;
  LineElementOptions* ops = (LineElementOptions*)lePtr->ops;

  // Draw fill area
  if (lePtr->fillPts) {
    // Create a path to use for both the polygon and its outline
    Blt_Ps_Append(ps, "% start fill area\n");
    Blt_Ps_Polyline(ps, lePtr->fillPts, lePtr->nFillPts);

    // If the background fill color was specified, draw the polygon in a
    // solid fashion with that color
    if (lePtr->fillBgColor) {
      Blt_Ps_XSetBackground(ps, lePtr->fillBgColor);
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    }
    Blt_Ps_XSetForeground(ps, ops->fillFgColor);
    if (ops->fillBg)
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    else
      Blt_Ps_Append(ps, "gsave fill grestore\n");

    Blt_Ps_Append(ps, "% end fill area\n");
  }

  // Draw lines (strip chart) or traces (xy graph)
  if (lePtr->lines.length > 0) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
      LinePen* penPtr = (LinePen *)stylePtr->penPtr;
      LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

      if ((stylePtr->lines.length > 0) && (penOps->traceWidth > 0)) {
	SetLineAttributes(ps, penPtr);
	Blt_Ps_Append(ps, "% start segments\n");
	Blt_Ps_Draw2DSegments(ps, stylePtr->lines.segments, 
			      stylePtr->lines.length);
	Blt_Ps_Append(ps, "% end segments\n");
      }
    }
  }
  else {
    LinePen* penPtr = NORMALPEN(ops);
    LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;

    if ((Blt_Chain_GetLength(lePtr->traces) > 0) && 
	(penOps->traceWidth > 0)) {
      TracesToPostScript(ps, lePtr, penPtr);
    }
  }

  // Draw symbols, error bars, values

  unsigned int count =0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {

    LineStyle *stylePtr = (LineStyle*)Blt_Chain_GetValue(link);
    LinePen* penPtr = (LinePen *)stylePtr->penPtr;
    LinePenOptions* penOps = (LinePenOptions*)penPtr->ops;
    XColor* colorPtr = penOps->errorBarColor;
    if (!colorPtr)
      colorPtr = penOps->traceColor;

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
    if ((stylePtr->symbolPts.length > 0) &&
	(penOps->symbol.type != SYMBOL_NONE)) {
      SymbolsToPostScript(graphPtr, ps, penPtr, stylePtr->symbolSize, 
			  stylePtr->symbolPts.length, 
			  stylePtr->symbolPts.points);
    }
    if (penOps->valueShow != SHOW_NONE) {
      ValuesToPostScript(ps, lePtr, penPtr, stylePtr->symbolPts.length, 
			 stylePtr->symbolPts.points, 
			 lePtr->symbolPts.map + count);
    }
    count += stylePtr->symbolPts.length;
  }
}

