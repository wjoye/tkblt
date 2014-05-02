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

#include <float.h>

#include "bltGraphBar.h"
#include "bltGrElemBar.h"
#include "bltGrElemOption.h"
#include "bltGrAxis.h"
#include "bltGrMisc.h"
#include "bltGrDef.h"
#include "bltConfig.h"
#include "bltPs.h"

using namespace Blt;

#define CLAMP(x,l,h)	((x) = (((x)<(l))? (l) : ((x)>(h)) ? (h) : (x)))
#define MIN3(a,b,c)	(((a)<(b))?(((a)<(c))?(a):(c)):(((b)<(c))?(b):(c)))

#define PointInRectangle(r,x0,y0)					\
  (((x0) <= (int)((r)->x + (r)->width - 1)) && ((x0) >= (int)(r)->x) && \
   ((y0) <= (int)((r)->y + (r)->height - 1)) && ((y0) >= (int)(r)->y))

// OptionSpecs

static Tk_ObjCustomOption styleObjOption =
  {
    "style", StyleSetProc, StyleGetProc, StyleRestoreProc, StyleFreeProc, 
    (ClientData)sizeof(BarStyle)
  };

extern Tk_ObjCustomOption penObjOption;
extern Tk_ObjCustomOption pairsObjOption;
extern Tk_ObjCustomOption valuesObjOption;
extern Tk_ObjCustomOption xAxisObjOption;
extern Tk_ObjCustomOption yAxisObjOption;

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_CUSTOM, "-activepen", "activePen", "ActivePen",
   "active", -1, Tk_Offset(BarElementOptions, activePenPtr), 
   TK_OPTION_NULL_OK, &penObjOption, 0},
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
   "6", -1, Tk_Offset(BarElementOptions, builtinPen.errorBarCapWidth),
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
   "x", -1, Tk_Offset(BarElementOptions, xAxis), 0, &xAxisObjOption, MAP_ITEM},
  {TK_OPTION_CUSTOM, "-mapy", "mapY", "MapY",
   "y", -1, Tk_Offset(BarElementOptions, yAxis), 0, &yAxisObjOption, MAP_ITEM},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_CUSTOM, "-pen", "pen", "Pen", 
   NULL, -1, Tk_Offset(BarElementOptions, normalPenPtr), 
   TK_OPTION_NULL_OK, &penObjOption},
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

BarElement::BarElement(Graph* graphPtr, const char* name, Tcl_HashEntry* hPtr)
  : Element(graphPtr, name, hPtr)
{
  barToData_ =NULL;
  bars_ =NULL;
  activeToData_ =NULL;
  activeRects_ =NULL;
  nBars_ =0;
  nActive_ =0;
  xPad_ =0;

  xeb_.segments =NULL;
  xeb_.length =0;
  xeb_.map =NULL;
  yeb_.segments =NULL;
  yeb_.length =0;
  yeb_.map =NULL;

  ops_ = (BarElementOptions*)calloc(1, sizeof(BarElementOptions));
  BarElementOptions* ops = (BarElementOptions*)ops_;
  ops->elemPtr = (Element*)this;

  builtinPenPtr = new BarPen(graphPtr_, "builtin", &ops->builtinPen);
  ops->builtinPenPtr = builtinPenPtr;

  optionTable_ = Tk_CreateOptionTable(graphPtr->interp_, optionSpecs);

  ops->stylePalette = Blt_Chain_Create();
  // this is an option and will be freed via Tk_FreeConfigOptions
  // By default an element's name and label are the same
  ops->label = Tcl_Alloc(strlen(name)+1);
  if (name)
    strcpy((char*)ops->label,(char*)name);

  Tk_InitOptions(graphPtr_->interp_, (char*)&(ops->builtinPen),
		 builtinPenPtr->optionTable(), graphPtr->tkwin_);
}

BarElement::~BarElement()
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  if (builtinPenPtr)
    delete builtinPenPtr;

  ResetBar();

  if (ops->stylePalette) {
    Blt_FreeStylePalette(ops->stylePalette);
    Blt_Chain_Destroy(ops->stylePalette);
  }

}

int BarElement::configure()
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  if (builtinPenPtr->configure() != TCL_OK)
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

void BarElement::map()
{
  BarGraph* barGraphPtr_ = (BarGraph*)graphPtr_;
  BarElementOptions* ops = (BarElementOptions*)ops_;
  BarGraphOptions* gops = (BarGraphOptions*)graphPtr_->ops_;

  if (!link)
    return;

  ResetBar();
  if (!ops->coords.x || !ops->coords.y ||
      !ops->coords.x->nValues || !ops->coords.y->nValues)
    return;
  int nPoints = NUMBEROFPOINTS(ops);

  //  double barWidth = graphPtr->barWidth;
  double barWidth = (ops->barWidth > 0.0f) ? ops->barWidth : gops->barWidth;
  AxisOptions* axisyops = (AxisOptions*)ops->yAxis->ops();
  double baseline = (axisyops->logScale) ? 0.0 : gops->baseline;
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

    if (((x[i] - barWidth) > ops->xAxis->axisRange_.max) ||
	((x[i] + barWidth) < ops->xAxis->axisRange_.min)) {
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

    if ((barGraphPtr_->nBarGroups_ > 0) && (gops->barMode != BARS_INFRONT) && 
	(!gops->stackAxes)) {
      Tcl_HashEntry *hPtr;
      BarSetKey key;

      key.value = (float)x[i];
      key.xAxis = ops->xAxis;
      key.yAxis = NULL;
      hPtr = Tcl_FindHashEntry(&barGraphPtr_->setTable_, (char *)&key);
      if (hPtr) {

	Tcl_HashTable *tablePtr = (Tcl_HashTable*)Tcl_GetHashValue(hPtr);
	const char *name = (ops->groupName) ? 
	  ops->groupName : ops->yAxis->name_;
	hPtr = Tcl_FindHashEntry(tablePtr, name);
	if (hPtr) {
	  double slice, width, offset;
		    
	  BarGroup *groupPtr = (BarGroup*)Tcl_GetHashValue(hPtr);
	  slice = barWidth / (double)barGraphPtr_->maxBarSetSize_;
	  offset = (slice * groupPtr->index);
	  if (barGraphPtr_->maxBarSetSize_ > 1) {
	    offset += slice * 0.05;
	    slice *= 0.90;
	  }
	  switch (gops->barMode) {
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
    c1 = graphPtr_->map2D(c1.x, c1.y, ops->xAxis, ops->yAxis);
    c2 = graphPtr_->map2D(c2.x, c2.y, ops->xAxis, ops->yAxis);
    if ((ybot == 0.0) && (axisyops->logScale))
      c2.y = graphPtr_->bottom_;
	    
    if (c2.y < c1.y) {
      double t;
      t = c1.y, c1.y = c2.y, c2.y = t;
    }
    if (c2.x < c1.x) {
      double t;
      t = c1.x, c1.x = c2.x, c2.x = t;
    }
    if ((c1.x > graphPtr_->right_) || (c2.x < graphPtr_->left_) || 
	(c1.y > graphPtr_->bottom_) || (c2.y < graphPtr_->top_)) {
      continue;
    }
    /* Bound the bars horizontally by the width of the graph window */
    /* Bound the bars vertically by the position of the axis. */
    if (gops->stackAxes) {
      top = ops->yAxis->screenMin_;
      bottom = ops->yAxis->screenMin_ + ops->yAxis->screenRange_;
      left = graphPtr_->left_;
      right = graphPtr_->right_;
    } else {
      left = top = 0;
      bottom = right = 10000;
      /* Shouldn't really have a call to Tk_Width or Tk_Height in
       * mapping routine.  We only want to clamp the bar segment to the
       * size of the window if we're actually mapped onscreen. */
      if (Tk_Height(graphPtr_->tkwin_) > 1)
	bottom = Tk_Height(graphPtr_->tkwin_);

      if (Tk_Width(graphPtr_->tkwin_) > 1)
	right = Tk_Width(graphPtr_->tkwin_);
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
  nBars_ = count;
  bars_ = bars;
  barToData_ = barToData;
  if (nActiveIndices_ > 0) {
    MapActiveBars();
  }
	
  int size = 20;
  if (count > 0)
    size = bars->width;

  // Set the symbol size of all the pen styles
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    BarPen* penPtr = stylePtr->penPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
    stylePtr->symbolSize = size;
    stylePtr->errorBarCapWidth = 
      (penOps->errorBarCapWidth > 0) 
      ? penOps->errorBarCapWidth : (size * 66666) / 100000;
    stylePtr->errorBarCapWidth /= 2;
  }

  BarStyle** dataToStyle = (BarStyle**)StyleMap();
  if (((ops->yHigh && ops->yHigh->nValues > 0) && 
       (ops->yLow && ops->yLow->nValues > 0)) ||
      ((ops->xHigh && ops->xHigh->nValues > 0) &&
       (ops->xLow && ops->xLow->nValues > 0)) ||
      (ops->xError && ops->xError->nValues > 0) || 
      (ops->yError && ops->yError->nValues > 0)) {
    MapErrorBars(dataToStyle);
  }

  MergePens(dataToStyle);
  free(dataToStyle);
}

void BarElement::extents(Region2d *regPtr)
{
  BarGraph* barGraphPtr_ = (BarGraph*)graphPtr_;
  BarElementOptions* ops = (BarElementOptions*)ops_;
  BarGraphOptions* gops = (BarGraphOptions*)graphPtr_->ops_;

  regPtr->top = regPtr->left = DBL_MAX;
  regPtr->bottom = regPtr->right = -DBL_MAX;

  if (!ops->coords.x || !ops->coords.y ||
      !ops->coords.x->nValues || !ops->coords.y->nValues)
    return;

  int nPoints = NUMBEROFPOINTS(ops);
  double barWidth = gops->barWidth;
  if (ops->barWidth > 0.0f)
    barWidth = ops->barWidth;

  double middle = 0.5;
  regPtr->left = ops->coords.x->min - middle;
  regPtr->right = ops->coords.x->max + middle;

  regPtr->top = ops->coords.y->min;
  regPtr->bottom = ops->coords.y->max;
  if (regPtr->bottom < gops->baseline)
    regPtr->bottom = gops->baseline;

  // Handle stacked bar elements specially.
  // If element is stacked, the sum of its ordinates may be outside the
  // minimum/maximum limits of the element's data points.
  if ((gops->barMode == BARS_STACKED) && (barGraphPtr_->nBarGroups_ > 0))
    CheckBarStacks(ops->xAxis, ops->yAxis, &regPtr->top, &regPtr->bottom);

  // Warning: You get what you deserve if the x-axis is logScale
  AxisOptions* axisxops = (AxisOptions*)ops->xAxis->ops();
  AxisOptions* axisyops = (AxisOptions*)ops->yAxis->ops();
  if (axisxops->logScale)
    regPtr->left = FindElemValuesMinimum(ops->coords.x, DBL_MIN) + middle;

  // Fix y-min limits for barchart
  if (axisyops->logScale) {
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
      if (axisxops->logScale) {
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
      if ((ops->xLow->min <= 0.0) && (axisxops->logScale))
	left = FindElemValuesMinimum(ops->xLow, DBL_MIN);
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
      if (axisyops->logScale) {
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
	  (axisyops->logScale))
	top = FindElemValuesMinimum(ops->yLow, DBL_MIN);
      else
	top = ops->yLow->min;

      if (top < regPtr->top)
	regPtr->top = top;
    }
  }
}

void BarElement::closest()
{
  BarElementOptions* ops = (BarElementOptions*)ops_;
  BarGraphOptions* gops = (BarGraphOptions*)graphPtr_->ops_;

  ClosestSearch* searchPtr = &gops->search;
  double minDist = searchPtr->dist;
  int imin = 0;
    
  int i;
  XRectangle *bp;
  for (bp = bars_, i = 0; i < nBars_; i++, bp++) {
    if (PointInRectangle(bp, searchPtr->x, searchPtr->y)) {
      imin = barToData_[i];
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
	imin = barToData_[i];
      }
    }
  }
  if (minDist < searchPtr->dist) {
    searchPtr->elemPtr = (Element*)this;
    searchPtr->dist = minDist;
    searchPtr->index = imin;
    searchPtr->point.x = 
      ops->coords.x ? (double)ops->coords.x->values[imin] : 0;
    searchPtr->point.y = 
      ops->coords.y ? (double)ops->coords.y->values[imin] : 0;
  }
}

void BarElement::draw(Drawable drawable)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  if (ops->hide)
    return;

  int count = 0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {

    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    BarPen* penPtr = (BarPen*)stylePtr->penPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();

    if (stylePtr->nBars > 0)
      DrawBarSegments(drawable, penPtr, stylePtr->bars, stylePtr->nBars);

    if ((stylePtr->xeb.length > 0) && (penOps->errorBarShow & SHOW_X))
      Blt_Draw2DSegments(graphPtr_->display_, drawable, penPtr->errorBarGC_, 
			 stylePtr->xeb.segments, stylePtr->xeb.length);

    if ((stylePtr->yeb.length > 0) && (penOps->errorBarShow & SHOW_Y))
      Blt_Draw2DSegments(graphPtr_->display_, drawable, penPtr->errorBarGC_, 
			 stylePtr->yeb.segments, stylePtr->yeb.length);

    if (penOps->valueShow != SHOW_NONE)
      DrawBarValues(drawable, penPtr, stylePtr->bars, stylePtr->nBars, 
		    barToData_ + count);

    count += stylePtr->nBars;
  }
}

void BarElement::drawActive(Drawable drawable)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  if (ops->hide || !(flags & ACTIVE))
    return;

  BarPen* penPtr = (BarPen*)ops->activePenPtr;
  if (!penPtr)
    return;
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();

  if (nActiveIndices_ > 0) {
    if (flags & ACTIVE_PENDING)
      MapActiveBars();

    DrawBarSegments(drawable, penPtr, activeRects_, nActive_);
    if (penOps->valueShow != SHOW_NONE)
      DrawBarValues(drawable, penPtr, activeRects_, nActive_, activeToData_);
  }
  else if (nActiveIndices_ < 0) {
    DrawBarSegments(drawable, penPtr, bars_, nBars_);
    if (penOps->valueShow != SHOW_NONE)
      DrawBarValues(drawable, penPtr, bars_, nBars_, barToData_);
  }
}

void BarElement::drawSymbol(Drawable drawable, int x, int y, int size)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  BarPen* penPtr = NORMALPEN(ops);
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();

  if (!penOps->fill && !penOps->outlineColor)
    return;

  int radius = (size / 2);
  size--;

  x -= radius;
  y -= radius;
  if (penPtr->fillGC_)
    XSetTSOrigin(graphPtr_->display_, penPtr->fillGC_, x, y);

  if (penOps->stipple != None)
    XFillRectangle(graphPtr_->display_, drawable, penPtr->fillGC_, x, y, 
		   size, size);
  else
    Tk_Fill3DRectangle(graphPtr_->tkwin_, drawable, penOps->fill, 
		       x, y, size, size, penOps->borderWidth, penOps->relief);

  XDrawRectangle(graphPtr_->display_, drawable, penPtr->outlineGC_, x, y, 
		 size, size);
  if (penPtr->fillGC_)
    XSetTSOrigin(graphPtr_->display_, penPtr->fillGC_, 0, 0);
}

void BarElement::print(Blt_Ps ps)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;
  
  if (ops->hide)
    return;

  // Comment the PostScript to indicate the start of the element
  Blt_Ps_Format(ps, "\n%% Element \"%s\"\n\n", name_);

  int count = 0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link;
       link = Blt_Chain_NextLink(link)) {

    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    BarPen* penPtr = (BarPen*)stylePtr->penPtr;
    BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
    if (stylePtr->nBars > 0)
      SegmentsToPostScript(ps, penPtr, stylePtr->bars, stylePtr->nBars);

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
      BarValuesToPostScript(ps, penPtr, stylePtr->bars, stylePtr->nBars, 
			    barToData_ + count);

    count += stylePtr->nBars;
  }
}

void BarElement::printActive(Blt_Ps ps)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  if (ops->hide || !(flags & ACTIVE))
    return;

  BarPen* penPtr = (BarPen*)ops->activePenPtr;
  if (!penPtr)
    return;
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
	
  Blt_Ps_Format(ps, "\n%% Active Element \"%s\"\n\n", name_);

  if (nActiveIndices_ > 0) {
    if (flags & ACTIVE_PENDING)
      MapActiveBars();
    SegmentsToPostScript(ps, penPtr, activeRects_, nActive_);
    if (penOps->valueShow != SHOW_NONE)
      BarValuesToPostScript(ps, penPtr, activeRects_, nActive_,activeToData_);
  }
  else if (nActiveIndices_ < 0) {
    SegmentsToPostScript(ps, penPtr, bars_, nBars_);
    if (penOps->valueShow != SHOW_NONE)
      BarValuesToPostScript(ps, penPtr, bars_, nBars_, barToData_);
  }
}

void BarElement::printSymbol(Blt_Ps ps, double x, double y, int size)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  BarPen* penPtr = NORMALPEN(ops);
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();

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
    Blt_Ps_XSetStipple(ps, graphPtr_->display_, penOps->stipple);
  } else if (penOps->outlineColor) {
    Blt_Ps_XSetForeground(ps, penOps->outlineColor);
    Blt_Ps_Append(ps, "    fill\n");
  }
  Blt_Ps_Append(ps, "  grestore\n");
  Blt_Ps_Append(ps, "} def\n\n");
  Blt_Ps_Format(ps, "%g %g %d Sq\n", x, y, size);
}

// Support

void BarElement::ResetStylePalette(Blt_Chain stylePalette)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette); link; 
       link = Blt_Chain_NextLink(link)) {
    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    stylePtr->xeb.length = 0;
    stylePtr->yeb.length = 0;
    stylePtr->nBars = 0;
  }
}

void BarElement::CheckBarStacks(Axis* xAxis, Axis* yAxis, 
				double *minPtr, double *maxPtr)
{
  BarGraph* barGraphPtr_ = (BarGraph*)graphPtr_;
  BarGraphOptions* gops = (BarGraphOptions*)graphPtr_->ops_;
  if ((gops->barMode != BARS_STACKED) || barGraphPtr_->nBarGroups_ == 0)
    return;

  BarGroup *gp, *gend;
  for (gp = barGraphPtr_->barGroups_, gend = gp + barGraphPtr_->nBarGroups_; gp < gend;
       gp++) {
    if ((gp->xAxis == xAxis) && (gp->yAxis == yAxis)) {

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

void BarElement::MergePens(BarStyle** dataToStyle)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  if (Blt_Chain_GetLength(ops->stylePalette) < 2) {
    Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette);
    BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
    stylePtr->nBars = nBars_;
    stylePtr->bars = bars_;
    stylePtr->symbolSize = bars_->width / 2;
    stylePtr->xeb.length = xeb_.length;
    stylePtr->xeb.segments = xeb_.segments;
    stylePtr->yeb.length = yeb_.length;
    stylePtr->yeb.segments = yeb_.segments;
    return;
  }

  // We have more than one style. Group bar segments of like pen styles
  // together
  if (nBars_ > 0) {
    XRectangle* bars = (XRectangle*)malloc(nBars_ * sizeof(XRectangle));
    int* barToData = (int*)malloc(nBars_ * sizeof(int));
    XRectangle* bp = bars;
    int* ip = barToData;
    for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
      stylePtr->symbolSize = bp->width / 2;
      stylePtr->bars = bp;
      for (int ii=0; ii<nBars_; ii++) {
	int iData = barToData[ii];
	if (dataToStyle[iData] == stylePtr) {
	  *bp++ = bars[ii];
	  *ip++ = iData;
	}
      }
      stylePtr->nBars = bp - stylePtr->bars;
    }
    free(bars_);
    free(barToData_);
    bars_ = bars;
    barToData_ = barToData;
  }

  if (xeb_.length > 0) {
    Segment2d* bars = (Segment2d*)malloc(xeb_.length * sizeof(Segment2d));
    int* map = (int*)malloc(xeb_.length * sizeof(int));
    Segment2d *sp = bars;
    int* ip = map;
    for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); 
	 link; link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
      stylePtr->xeb.segments = sp;
      for (int ii=0; ii<xeb_.length; ii++) {
	int iData = xeb_.map[ii];
	if (dataToStyle[iData] == stylePtr) {
	  *sp++ = xeb_.segments[ii];
	  *ip++ = iData;
	}
      }
      stylePtr->xeb.length = sp - stylePtr->xeb.segments;
    }
    free(xeb_.segments);
    xeb_.segments = bars;
    free(xeb_.map);
    xeb_.map = map;
  }

  if (yeb_.length > 0) {
    Segment2d* bars = (Segment2d*)malloc(yeb_.length * sizeof(Segment2d));
    int* map = (int*)malloc(yeb_.length * sizeof(int));
    Segment2d* sp = bars;
    int* ip = map;
    for (Blt_ChainLink link = Blt_Chain_FirstLink(ops->stylePalette); link; 
	 link = Blt_Chain_NextLink(link)) {
      BarStyle *stylePtr = (BarStyle*)Blt_Chain_GetValue(link);
      stylePtr->yeb.segments = sp;
      for (int ii=0; ii<yeb_.length; ii++) {
	int iData = yeb_.map[ii];
	if (dataToStyle[iData] == stylePtr) {
	  *sp++ = yeb_.segments[ii];
	  *ip++ = iData;
	}
      }
      stylePtr->yeb.length = sp - stylePtr->yeb.segments;
    }
    free(yeb_.segments);
    yeb_.segments = bars;
    free(yeb_.map);
    yeb_.map = map;
  }
}

void BarElement::MapActiveBars()
{
  if (activeRects_) {
    free(activeRects_);
    activeRects_ = NULL;
  }
  if (activeToData_) {
    free(activeToData_);
    activeToData_ = NULL;
  }
  nActive_ = 0;

  if (nActiveIndices_ > 0) {
    XRectangle *activeRects = 
      (XRectangle*)malloc(sizeof(XRectangle) * nActiveIndices_);
    int* activeToData = (int*)malloc(sizeof(int) * nActiveIndices_);
    int count = 0;
    for (int ii=0; ii<nBars_; ii++) {
      int *ip, *iend;
      for (ip = activeIndices_, iend = ip + nActiveIndices_; ip < iend; ip++) {
	if (barToData_[ii] == *ip) {
	  activeRects[count] = bars_[ii];
	  activeToData[count] = ii;
	  count++;
	}
      }
    }
    nActive_ = count;
    activeRects_ = activeRects;
    activeToData_ = activeToData;
  }
  flags &= ~ACTIVE_PENDING;
}

void BarElement::ResetBar()
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  ResetStylePalette(ops->stylePalette);

  if (activeRects_)
    free(activeRects_);
  activeRects_ = NULL;

  if (activeToData_)
    free(activeToData_);
  activeToData_ = NULL;

  if (xeb_.segments)
    free(xeb_.segments);
  xeb_.segments = NULL;

  if (xeb_.map)
    free(xeb_.map);
  xeb_.map = NULL;

  if (yeb_.segments)
    free(yeb_.segments);
  yeb_.segments = NULL;

  if (yeb_.map)
    free(yeb_.map);
  yeb_.map = NULL;

  if (bars_)
    free(bars_);
  bars_ = NULL;

  if (barToData_)
    free(barToData_);
  barToData_ = NULL;

  nActive_ = 0;
  xeb_.length = 0;
  yeb_.length = 0;
  nBars_ = 0;
}

void BarElement::MapErrorBars(BarStyle **dataToStyle)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;

  Region2d reg;
  graphPtr_->extents(&reg);

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
	  Point2d p = graphPtr_->map2D(high, y, ops->xAxis, ops->yAxis);
	  Point2d q = graphPtr_->map2D(low, y, ops->xAxis, ops->yAxis);
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
    xeb_.segments = bars;
    xeb_.length = segPtr - bars;
    xeb_.map = map;
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
	  Point2d p = graphPtr_->map2D(x, high, ops->xAxis, ops->yAxis);
	  Point2d q = graphPtr_->map2D(x, low, ops->xAxis, ops->yAxis);
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
    yeb_.segments = bars;
    yeb_.length = segPtr - bars;
    yeb_.map = map;
  }
}

void BarElement::SetBackgroundClipRegion(Tk_Window tkwin, Tk_3DBorder border, 
					 TkRegion rgn)
{
  Display* display = Tk_Display(tkwin);
  GC gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
  TkSetRegion(display, gc, rgn);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
  TkSetRegion(display, gc, rgn);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
  TkSetRegion(display, gc, rgn);
}

void BarElement::UnsetBackgroundClipRegion(Tk_Window tkwin, Tk_3DBorder border)
{
  Display* display = Tk_Display(tkwin);
  GC gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
  XSetClipMask(display, gc, None);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
  XSetClipMask(display, gc, None);
  gc = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
  XSetClipMask(display, gc, None);
}

void BarElement::DrawBarSegments(Drawable drawable, BarPen* penPtr,
				 XRectangle *bars, int nBars)
{
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
  TkRegion rgn;

  XRectangle clip;
  clip.x = graphPtr_->left_;
  clip.y = graphPtr_->top_;
  clip.width  = graphPtr_->right_ - graphPtr_->left_ + 1;
  clip.height = graphPtr_->bottom_ - graphPtr_->top_ + 1;
  rgn = TkCreateRegion();
  TkUnionRectWithRegion(&clip, rgn, rgn);

  if (penOps->fill) {
    int relief = (penOps->relief == TK_RELIEF_SOLID) ? 
      TK_RELIEF_FLAT: penOps->relief;

    int hasOutline = ((relief == TK_RELIEF_FLAT) && penOps->outlineColor);
    if (penOps->stipple != None)
      TkSetRegion(graphPtr_->display_, penPtr->fillGC_, rgn);

    SetBackgroundClipRegion(graphPtr_->tkwin_, penOps->fill, rgn);

    if (hasOutline)
      TkSetRegion(graphPtr_->display_, penPtr->outlineGC_, rgn);

    XRectangle *rp, *rend;
    for (rp = bars, rend = rp + nBars; rp < rend; rp++) {
      if (penOps->stipple != None)
	XFillRectangle(graphPtr_->display_, drawable, penPtr->fillGC_, 
		       rp->x, rp->y, rp->width, rp->height);
      else
	Tk_Fill3DRectangle(graphPtr_->tkwin_, drawable, 
			   penOps->fill, rp->x, rp->y, rp->width, rp->height, 
			   penOps->borderWidth, relief);

      if (hasOutline)
	XDrawRectangle(graphPtr_->display_, drawable, penPtr->outlineGC_, 
		       rp->x, rp->y, rp->width, rp->height);
    }

    UnsetBackgroundClipRegion(graphPtr_->tkwin_, penOps->fill);

    if (hasOutline)
      XSetClipMask(graphPtr_->display_, penPtr->outlineGC_, None);

    if (penOps->stipple != None)
      XSetClipMask(graphPtr_->display_, penPtr->fillGC_, None);

  }
  else if (penOps->outlineColor) {
    TkSetRegion(graphPtr_->display_, penPtr->outlineGC_, rgn);
    XDrawRectangles(graphPtr_->display_, drawable, penPtr->outlineGC_, bars, 
		    nBars);
    XSetClipMask(graphPtr_->display_, penPtr->outlineGC_, None);
  }

  TkDestroyRegion(rgn);
}

void BarElement::DrawBarValues(Drawable drawable, BarPen* penPtr, 
			       XRectangle *bars, int nBars, int *barToData)
{
  BarElementOptions* ops = (BarElementOptions*)ops_;
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
  BarGraphOptions* gops = (BarGraphOptions*)graphPtr_->ops_;

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
      snprintf(string, TCL_DOUBLE_SPACE, fmt, x); 
    else if (penOps->valueShow == SHOW_Y)
      snprintf(string, TCL_DOUBLE_SPACE, fmt, y); 
    else if (penOps->valueShow == SHOW_BOTH) {
      snprintf(string, TCL_DOUBLE_SPACE, fmt, x);
      strcat(string, ",");
      snprintf(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
    }

    if (gops->inverted) {
      anchorPos.y = rp->y + rp->height * 0.5;
      anchorPos.x = rp->x + rp->width;
      if (x < gops->baseline)
	anchorPos.x -= rp->width;
    }
    else {
      anchorPos.x = rp->x + rp->width * 0.5;
      anchorPos.y = rp->y;
      if (y < gops->baseline)
	anchorPos.y += rp->height;
    }
    Blt_DrawText(graphPtr_->tkwin_, drawable, string, &penOps->valueStyle, 
		 (int)anchorPos.x, (int)anchorPos.y);
  }
}

void BarElement::SegmentsToPostScript(Blt_Ps ps, BarPen* penPtr, 
				      XRectangle *bars, int nBars)
{
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
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
      Blt_Ps_XSetStipple(ps, graphPtr_->display_, penOps->stipple);
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

void BarElement::BarValuesToPostScript(Blt_Ps ps, BarPen* penPtr, 
				       XRectangle *bars, int nBars, 
				       int *barToData)
{
  BarPenOptions* penOps = (BarPenOptions*)penPtr->ops();
  BarElementOptions* ops = (BarElementOptions*)ops_;
  BarGraphOptions* gops = (BarGraphOptions*)graphPtr_->ops_;

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
    if (penOps->valueShow == SHOW_X)
      snprintf(string, TCL_DOUBLE_SPACE, fmt, x); 
    else if (penOps->valueShow == SHOW_Y)
      snprintf(string, TCL_DOUBLE_SPACE, fmt, y); 
    else if (penOps->valueShow == SHOW_BOTH) {
      snprintf(string, TCL_DOUBLE_SPACE, fmt, x);
      strcat(string, ",");
      snprintf(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
    }
    if (gops->inverted) {
      anchorPos.y = rp->y + rp->height * 0.5;
      anchorPos.x = rp->x + rp->width;
      if (x < gops->baseline) {
	anchorPos.x -= rp->width;
      } 
    } else {
      anchorPos.x = rp->x + rp->width * 0.5;
      anchorPos.y = rp->y;
      if (y < gops->baseline) {			
	anchorPos.y += rp->height;
      }
    }
    Blt_Ps_DrawText(ps, string, &penOps->valueStyle, anchorPos.x, 
		    anchorPos.y);
  }
}

