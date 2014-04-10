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

extern "C" {
#include "bltGraph.h"
}

#include "bltConfig.h"
#include "bltGrElem.h"
#include "bltGrElemBar.h"
#include "bltGrElemLine.h"
#include "bltGrAxis.h"
#include "bltGrAxisOption.h"
#include "bltGrLegd.h"

#define AXIS_PAD_TITLE 2
#define NUMDIGITS 15	/* Specifies the number of digits of
			 * accuracy used when outputting axis
			 * tick labels. */

#define UROUND(x,u)		(Round((x)/(u))*(u))
#define UCEIL(x,u)		(ceil((x)/(u))*(u))
#define UFLOOR(x,u)		(floor((x)/(u))*(u))
#define HORIZMARGIN(m)	(!((m)->site & 0x1)) /* Even sites are horizontal */

typedef struct {
  int axis;
  int t1;
  int t2;
  int label;
} AxisInfo;

AxisName axisNames[] = { 
  { "x",  CID_AXIS_X, MARGIN_BOTTOM, MARGIN_LEFT   },
  { "y",  CID_AXIS_Y, MARGIN_LEFT,   MARGIN_BOTTOM },
  { "x2", CID_AXIS_X, MARGIN_TOP,    MARGIN_RIGHT  },
  { "y2", CID_AXIS_Y, MARGIN_RIGHT,  MARGIN_TOP    }
} ;

// Defs

extern int ConfigureAxis(Axis *axisPtr);
extern int AxisObjConfigure(Tcl_Interp* interp, Axis* axisPtr,
			    int objc, Tcl_Obj* const objv[]);

static void FreeTickLabels(Blt_Chain chain);
static int Round(double x)
{
  return (int) (x + ((x < 0.0) ? -0.5 : 0.5));
}

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_COLOR, "-activeforeground", "activeForeground", "ActiveForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(AxisOptions, activeFgColor), 
   0, NULL, 0}, 
  {TK_OPTION_RELIEF, "-activerelief", "activeRelief", "Relief",
   "flat", -1, Tk_Offset(AxisOptions, activeRelief), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-autorange", "autoRange", "AutoRange",
   "0", -1, Tk_Offset(AxisOptions, windowSize), 0, NULL, 0},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   NULL, -1, Tk_Offset(AxisOptions, normalBg), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_CUSTOM, "-bindtags", "bindTags", "BindTags",
   "all", -1, Tk_Offset(AxisOptions, tags), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   "0", -1, Tk_Offset(AxisOptions, borderWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-checklimits", "checkLimits", "CheckLimits", 
   "no", -1, Tk_Offset(AxisOptions, checkLimits), 0, NULL, 0},
  {TK_OPTION_COLOR, "-color", "color", "Color",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(AxisOptions, tickColor), 0, NULL, 0},
  {TK_OPTION_STRING, "-command", "command", "Command",
   NULL, -1, Tk_Offset(AxisOptions, formatCmd), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-descending", "descending", "Descending",
   "no", -1, Tk_Offset(AxisOptions, descending), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-exterior", "exterior", "exterior",
   "yes", -1, Tk_Offset(AxisOptions, exterior), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-color", 0},
  {TK_OPTION_SYNONYM, "-foreground", NULL, NULL, NULL, -1, 0, 0, "-color", 0},
  {TK_OPTION_BOOLEAN, "-grid", "grid", "Grid",
   "yes", -1, Tk_Offset(AxisOptions, showGrid), 0, NULL, 0},
  {TK_OPTION_COLOR, "-gridcolor", "gridColor", "GridColor", 
   "gray64", -1, Tk_Offset(AxisOptions, major.color), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-griddashes", "gridDashes", "GridDashes", 
   "dot", -1, Tk_Offset(AxisOptions, major.dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_PIXELS, "-gridlinewidth", "gridLineWidth", "GridLineWidth",
   "0", -1, Tk_Offset(AxisOptions, major.lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-gridminor", "gridMinor", "GridMinor", 
   "yes", -1, Tk_Offset(AxisOptions, showGridMinor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-gridminorcolor", "gridMinorColor", "GridMinorColor", 
   "gray64", -1, Tk_Offset(AxisOptions, minor.color), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-gridminordashes", "gridMinorDashes", "GridMinorDashes", 
   "dot", -1, Tk_Offset(AxisOptions, minor.dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_PIXELS, "-gridminorlinewidth", "gridMinorLineWidth", 
   "GridMinorLineWidth",
   "0", -1, Tk_Offset(AxisOptions, minor.lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide",
   "no", -1, Tk_Offset(AxisOptions, hide), 0, NULL, 0},
  {TK_OPTION_JUSTIFY, "-justify", "justify", "Justify",
   "c", -1, Tk_Offset(AxisOptions, titleJustify), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-labeloffset", "labelOffset", "LabelOffset",
   "no", -1, Tk_Offset(AxisOptions, labelOffset), 0, NULL, 0},
  {TK_OPTION_COLOR, "-limitscolor", "limitsColor", "LimitsColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(AxisOptions, limitsTextStyle.color), 
   0, NULL, 0},
  {TK_OPTION_FONT, "-limitsfont", "limitsFont", "LimitsFont",
   STD_FONT_SMALL, -1, Tk_Offset(AxisOptions, limitsTextStyle.font), 
   0, NULL, 0},
  {TK_OPTION_STRING, "-limitsformat", "limitsFormat", "LimitsFormat",
   NULL, -1, Tk_Offset(AxisOptions, limitsFormat), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(AxisOptions, lineWidth), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-logscale", "logScale", "LogScale",
   "no", -1, Tk_Offset(AxisOptions, logScale), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-loosemin", "looseMin", "LooseMin", 
   "no", -1, Tk_Offset(AxisOptions, looseMin), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-loosemax", "looseMax", "LooseMax", 
   "no", -1, Tk_Offset(AxisOptions, looseMax), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-majorticks", "majorTicks", "MajorTicks",
   NULL, -1, Tk_Offset(AxisOptions, t1UPtr), 
   TK_OPTION_NULL_OK, &ticksObjOption, 0},
  {TK_OPTION_CUSTOM, "-max", "max", "Max", 
   NULL, -1, Tk_Offset(AxisOptions, reqMax), 
   TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_CUSTOM, "-min", "min", "Min", 
   NULL, -1, Tk_Offset(AxisOptions, reqMin), 
   TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_CUSTOM, "-minorticks", "minorTicks", "MinorTicks",
   NULL, -1, Tk_Offset(AxisOptions, t2UPtr), 
   TK_OPTION_NULL_OK, &ticksObjOption, 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "flat", -1, Tk_Offset(AxisOptions, relief), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-rotate", "rotate", "Rotate", 
   "0", -1, Tk_Offset(AxisOptions, tickAngle), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-scrollcommand", "scrollCommand", "ScrollCommand",
   NULL, -1, Tk_Offset(AxisOptions, scrollCmdObjPtr), 
   TK_OPTION_NULL_OK, &objectObjOption, 0},
  {TK_OPTION_PIXELS, "-scrollincrement", "scrollIncrement", "ScrollIncrement",
   "10", -1, Tk_Offset(AxisOptions, scrollUnits), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-scrollmax", "scrollMax", "ScrollMax", 
   NULL, -1, Tk_Offset(AxisOptions, reqScrollMax),  
   TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_CUSTOM, "-scrollmin", "scrollMin", "ScrollMin", 
   NULL, -1, Tk_Offset(AxisOptions, reqScrollMin), 
   TK_OPTION_NULL_OK, &limitObjOption, 0},
  {TK_OPTION_DOUBLE, "-shiftby", "shiftBy", "ShiftBy",
   "0", -1, Tk_Offset(AxisOptions, shiftBy), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-showticks", "showTicks", "ShowTicks",
   "yes", -1, Tk_Offset(AxisOptions, showTicks), 0, NULL, 0},
  {TK_OPTION_DOUBLE, "-stepsize", "stepSize", "StepSize",
   "0", -1, Tk_Offset(AxisOptions, reqStep), 0, NULL, 0},
  {TK_OPTION_INT, "-subdivisions", "subdivisions", "Subdivisions",
   "2", -1, Tk_Offset(AxisOptions, reqNumMinorTicks), 0, NULL, 0},
  {TK_OPTION_ANCHOR, "-tickanchor", "tickAnchor", "Anchor",
   "c", -1, Tk_Offset(AxisOptions, reqTickAnchor), 0, NULL, 0},
  {TK_OPTION_FONT, "-tickfont", "tickFont", "Font",
   STD_FONT_SMALL, -1, Tk_Offset(AxisOptions, tickFont), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-ticklength", "tickLength", "TickLength",
   "8", -1, Tk_Offset(AxisOptions, tickLength), 0, NULL, 0},
  {TK_OPTION_INT, "-tickdefault", "tickDefault", "TickDefault",
   "4", -1, Tk_Offset(AxisOptions, reqNumMajorTicks), 0, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title",
   NULL, -1, Tk_Offset(AxisOptions, title), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_BOOLEAN, "-titlealternate", "titleAlternate", "TitleAlternate",
   "no", -1, Tk_Offset(AxisOptions, titleAlternate), 0, NULL, 0},
  {TK_OPTION_COLOR, "-titlecolor", "titleColor", "TitleColor", 
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(AxisOptions, titleColor), 0, NULL, 0},
  {TK_OPTION_FONT, "-titlefont", "titleFont", "TitleFont",
   STD_FONT_NORMAL, -1, Tk_Offset(AxisOptions, titleFont), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

Axis::Axis(Graph* graphPtr, const char* name, int margin)
{
  ops_ = (AxisOptions*)calloc(1, sizeof(AxisOptions));
  AxisOptions* ops = (AxisOptions*)ops_;

  graphPtr_ = graphPtr;
  classId_ = CID_NONE;
  name_ = dupstr(name);
  className_ = dupstr("none");

  use_ =0;
  hashPtr_ =NULL;
  flags =0;		

  /* Fields specific to axes. */

  detail_ =NULL;
  refCount_ =0;
  titlePos_.x =0;
  titlePos_.y =0;
  titleWidth_ =0;
  titleHeight_ =0;	
  min =0;
  max =0;
  scrollMin =0;
  scrollMax =0;
  valueRange.min =0;
  valueRange.max =0;
  valueRange.range =0;
  valueRange.scale =0;
  axisRange.min =0;
  axisRange.max =0;
  axisRange.range =0;
  axisRange.scale =0;
  prevMin =0;
  prevMax =0;
  t1Ptr =NULL;
  t2Ptr =NULL;
  minorSweep.initial =0;
  minorSweep.step =0;
  minorSweep.nSteps =0;
  majorSweep.initial =0;
  majorSweep.step =0;
  majorSweep.nSteps =0;

  /* The following fields are specific to logical axes */

  margin_ = margin;
  link =NULL;
  chain =NULL;
  segments =NULL;
  nSegments =0;
  tickLabels = Blt_Chain_Create();
  left =0;
  right =0;
  top =0;
  bottom =0;
  width =0;
  height =0;
  maxTickWidth =0;
  maxTickHeight =0; 
  tickAnchor = TK_ANCHOR_N;
  tickGC =NULL;
  activeTickGC =NULL;
  titleAngle =0;	
  titleAnchor = TK_ANCHOR_N;
  screenScale =0;
  screenMin =0;
  screenRange =0;

  ops->reqMin =NAN;
  ops->reqMax =NAN;
  ops->reqScrollMin =NAN;
  ops->reqScrollMax =NAN;
  Blt_Ts_InitStyle(ops->limitsTextStyle);

  optionTable_ = Tk_CreateOptionTable(graphPtr_->interp, optionSpecs);
}

Axis::~Axis()
{
  AxisOptions* ops = (AxisOptions*)ops_;

  if (graphPtr_->bindTable)
    Blt_DeleteBindings(graphPtr_->bindTable, this);

  if (link)
    Blt_Chain_DeleteLink(chain, link);

  if (name_)
    delete [] name_;
  if (className_)
    delete [] className_;

  if (hashPtr_)
    Tcl_DeleteHashEntry(hashPtr_);

  Blt_Ts_FreeStyle(graphPtr_->display, &ops->limitsTextStyle);

  if (tickGC)
    Tk_FreeGC(graphPtr_->display, tickGC);

  if (activeTickGC)
    Tk_FreeGC(graphPtr_->display, activeTickGC);

  if (ops->major.segments) 
    free(ops->major.segments);
  if (ops->major.gc) 
    Blt_FreePrivateGC(graphPtr_->display, ops->major.gc);

  if (ops->minor.segments) 
    free(ops->minor.segments);
  if (ops->minor.gc)
    Blt_FreePrivateGC(graphPtr_->display, ops->minor.gc);

  if (t1Ptr)
    free(t1Ptr);
  if (t2Ptr)
    free(t2Ptr);

  FreeTickLabels(tickLabels);

  Blt_Chain_Destroy(tickLabels);

  if (segments)
    free(segments);

  Tk_FreeConfigOptions((char*)ops_, optionTable_, graphPtr_->tkwin);

  if (ops_)
    free(ops_);
}

void Axis::setClass(ClassId classId)
{
  if (className_)
    delete [] className_;
  className_ =NULL;

  classId_ = classId;
  switch (classId) {
  case CID_NONE:
    className_ = dupstr("none");
    break;
  case CID_AXIS_X:
    className_ = dupstr("XAxis");
    break;
  case CID_AXIS_Y:
    className_ = dupstr("YAxis");
    break;
  default:
    break;
  }
}

// Support

void Axis::logScale(double min, double max)
{
  AxisOptions* ops = (AxisOptions*)ops_;

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
      range = niceNum(range, 0);
      majorStep = niceNum(range / ops->reqNumMajorTicks, 1);
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
    if (!ops->looseMin || (ops->looseMin && !isnan(ops->reqMin))) {
      tickMin = min;
      nMajor++;
    }
    if (!ops->looseMax || (ops->looseMax && !isnan(ops->reqMax))) {
      tickMax = max;
    }
  }
  majorSweep.step = majorStep;
  majorSweep.initial = floor(tickMin);
  majorSweep.nSteps = nMajor;
  minorSweep.initial = minorSweep.step = minorStep;
  minorSweep.nSteps = nMinor;

  setAxisRange(&axisRange, tickMin, tickMax);
}

void Axis::linearScale(double min, double max)
{
  AxisOptions* ops = (AxisOptions*)ops_;

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
    if (ops->reqStep > 0.0) {
      /* An interval was designated by the user.  Keep scaling it until
       * it fits comfortably within the current range of the axis.  */
      step = ops->reqStep;
      while ((2 * step) >= range) {
	step *= 0.5;
      }
    } 
    else {
      range = niceNum(range, 0);
      step = niceNum(range / ops->reqNumMajorTicks, 1);
    }
	
    /* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
    axisMin = tickMin = floor(min / step) * step + 0.0;
    axisMax = tickMax = ceil(max / step) * step + 0.0;
	
    nTicks = Round((tickMax - tickMin) / step) + 1;
  } 
  majorSweep.step = step;
  majorSweep.initial = tickMin;
  majorSweep.nSteps = nTicks;

  /*
   * The limits of the axis are either the range of the data ("tight") or at
   * the next outer tick interval ("loose").  The looseness or tightness has
   * to do with how the axis fits the range of data values.  This option is
   * overridden when the user sets an axis limit (by either -min or -max
   * option).  The axis limit is always at the selected limit (otherwise we
   * assume that user would have picked a different number).
   */
  if (!ops->looseMin || (ops->looseMin && !isnan(ops->reqMin)))
    axisMin = min;

  if (!ops->looseMax || (ops->looseMax && !isnan(ops->reqMax)))
    axisMax = max;

  setAxisRange(&axisRange, axisMin, axisMax);

  /* Now calculate the minor tick step and number. */

  if (ops->reqNumMinorTicks > 0) {
    nTicks = ops->reqNumMinorTicks - 1;
    step = 1.0 / (nTicks + 1);
  } 
  else {
    nTicks = 0;			/* No minor ticks. */
    step = 0.5;			/* Don't set the minor tick interval to
				 * 0.0. It makes the GenerateTicks
				 * routine * create minor log-scale tick
				 * marks.  */
  }
  minorSweep.initial = minorSweep.step = step;
  minorSweep.nSteps = nTicks;
}

void Axis::setAxisRange(AxisRange *rangePtr, double min, double max)
{
  rangePtr->min = min;
  rangePtr->max = max;
  rangePtr->range = max - min;
  if (fabs(rangePtr->range) < DBL_EPSILON) {
    rangePtr->range = 1.0;
  }
  rangePtr->scale = 1.0 / rangePtr->range;
}

void FixAxisRange(Axis *axisPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  double min, max;

  /*
   * When auto-scaling, the axis limits are the bounds of the element data.
   * If no data exists, set arbitrary limits (wrt to log/linear scale).
   */
  min = axisPtr->valueRange.min;
  max = axisPtr->valueRange.max;

  /* Check the requested axis limits. Can't allow -min to be greater
   * than -max, or have undefined log scale limits.  */
  if (((!isnan(ops->reqMin)) && (!isnan(ops->reqMax))) &&
      (ops->reqMin >= ops->reqMax)) {
    ops->reqMin = ops->reqMax = NAN;
  }
  if (ops->logScale) {
    if ((!isnan(ops->reqMin)) && (ops->reqMin <= 0.0)) {
      ops->reqMin = NAN;
    }
    if ((!isnan(ops->reqMax)) && (ops->reqMax <= 0.0)) {
      ops->reqMax = NAN;
    }
  }

  if (min == DBL_MAX) {
    if (!isnan(ops->reqMin)) {
      min = ops->reqMin;
    } else {
      min = (ops->logScale) ? 0.001 : 0.0;
    }
  }
  if (max == -DBL_MAX) {
    if (!isnan(ops->reqMax)) {
      max = ops->reqMax;
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
  axisPtr->setAxisRange(&axisPtr->valueRange, min, max);

  /*   
   * The axis limits are either the current data range or overridden by the
   * values selected by the user with the -min or -max options.
   */
  axisPtr->min = min;
  axisPtr->max = max;
  if (!isnan(ops->reqMin)) {
    axisPtr->min = ops->reqMin;
  }
  if (!isnan(ops->reqMax)) { 
    axisPtr->max = ops->reqMax;
  }
  if (axisPtr->max < axisPtr->min) {
    /*   
     * If the limits still don't make sense, it's because one limit
     * configuration option (-min or -max) was set and the other default
     * (based upon the data) is too small or large.  Remedy this by making
     * up a new min or max from the user-defined limit.
     */
    if (isnan(ops->reqMin)) {
      axisPtr->min = axisPtr->max - (fabs(axisPtr->max) * 0.1);
    }
    if (isnan(ops->reqMax)) {
      axisPtr->max = axisPtr->min + (fabs(axisPtr->max) * 0.1);
    }
  }
  /* 
   * If a window size is defined, handle auto ranging by shifting the axis
   * limits.
   */
  if ((ops->windowSize > 0.0) && 
      (isnan(ops->reqMin)) && (isnan(ops->reqMax))) {
    if (ops->shiftBy < 0.0) {
      ops->shiftBy = 0.0;
    }
    max = axisPtr->min + ops->windowSize;
    if (axisPtr->max >= max) {
      if (ops->shiftBy > 0.0) {
	max = UCEIL(axisPtr->max, ops->shiftBy);
      }
      axisPtr->min = max - ops->windowSize;
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
double Axis::niceNum(double x, int round)
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

int AxisIsHorizontal(Axis *axisPtr)
{
  Graph* graphPtr = axisPtr->graphPtr_;

  return ((axisPtr->classId() == CID_AXIS_Y) == graphPtr->inverted);
}

static void FreeTickLabels(Blt_Chain chain)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(chain); link != NULL; 
       link = Blt_Chain_NextLink(link)) {
    TickLabel *labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
    free(labelPtr);
  }
  Blt_Chain_Reset(chain);
}

static TickLabel *MakeLabel(Axis *axisPtr, double value)
{
#define TICK_LABEL_SIZE		200

  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  char string[TICK_LABEL_SIZE + 1];
  TickLabel *labelPtr;

  /* Generate a default tick label based upon the tick value.  */
  if (ops->logScale) {
    sprintf_s(string, TICK_LABEL_SIZE, "1E%d", ROUND(value));
  } else {
    sprintf_s(string, TICK_LABEL_SIZE, "%.*G", NUMDIGITS, value);
  }

  if (ops->formatCmd) {
    Graph* graphPtr;
    Tcl_Interp* interp;
    Tk_Window tkwin;
	
    graphPtr = axisPtr->graphPtr_;
    interp = graphPtr->interp;
    tkwin = graphPtr->tkwin;
    /*
     * A TCL proc was designated to format tick labels. Append the path
     * name of the widget and the default tick label as arguments when
     * invoking it. Copy and save the new label from interp->result.
     */
    Tcl_ResetResult(interp);
    if (Tcl_VarEval(interp, ops->formatCmd, " ", Tk_PathName(tkwin),
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
  labelPtr = (TickLabel*)malloc(sizeof(TickLabel) + strlen(string));
  strcpy(labelPtr->string, string);
  labelPtr->anchorPos.x = DBL_MAX;
  labelPtr->anchorPos.y = DBL_MAX;
  return labelPtr;
}

double Blt_InvHMap(Axis *axisPtr, double x)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  double value;

  x = (double)(x - axisPtr->screenMin) * axisPtr->screenScale;
  if (ops->descending) {
    x = 1.0 - x;
  }
  value = (x * axisPtr->axisRange.range) + axisPtr->axisRange.min;
  if (ops->logScale) {
    value = EXP10(value);
  }
  return value;
}

double Blt_InvVMap(Axis *axisPtr, double y) /* Screen coordinate */
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  double value;

  y = (double)(y - axisPtr->screenMin) * axisPtr->screenScale;
  if (ops->descending) {
    y = 1.0 - y;
  }
  value = ((1.0 - y) * axisPtr->axisRange.range) + axisPtr->axisRange.min;
  if (ops->logScale) {
    value = EXP10(value);
  }
  return value;
}

double Blt_HMap(Axis *axisPtr, double x)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  if ((ops->logScale) && (x != 0.0)) {
    x = log10(fabs(x));
  }
  /* Map graph coordinate to normalized coordinates [0..1] */
  x = (x - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
  if (ops->descending) {
    x = 1.0 - x;
  }
  return (x * axisPtr->screenRange + axisPtr->screenMin);
}

double Blt_VMap(Axis *axisPtr, double y)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  if ((ops->logScale) && (y != 0.0)) {
    y = log10(fabs(y));
  }
  /* Map graph coordinate to normalized coordinates [0..1] */
  y = (y - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
  if (ops->descending) {
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

void GetDataLimits(Axis *axisPtr, double min, double max)
{
  if (axisPtr->valueRange.min > min) {
    axisPtr->valueRange.min = min;
  }
  if (axisPtr->valueRange.max < max) {
    axisPtr->valueRange.max = max;
  }
}

static Ticks *GenerateTicks(TickSweep *sweepPtr)
{
  Ticks* ticksPtr = 
    (Ticks*)malloc(sizeof(Ticks) + (sweepPtr->nSteps * sizeof(double)));
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
    for (i = 0; i < sweepPtr->nSteps; i++)
      ticksPtr->values[i] = logTable[i];
  }
  else {
    double value = sweepPtr->initial;	/* Start from smallest axis tick */
    for (int ii=0; ii<sweepPtr->nSteps; ii++) {
      value = UROUND(value, sweepPtr->step);
      ticksPtr->values[ii] = value;
      value += sweepPtr->step;
    }
  }
  ticksPtr->nTicks = sweepPtr->nSteps;
  return ticksPtr;
}

static void ResetTextStyles(Axis *axisPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();

  Graph* graphPtr = axisPtr->graphPtr_;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;

  Blt_Ts_ResetStyle(graphPtr->tkwin, &ops->limitsTextStyle);

  gcMask = (GCForeground | GCLineWidth | GCCapStyle);
  gcValues.foreground = ops->tickColor->pixel;
  gcValues.font = Tk_FontId(ops->tickFont);
  gcValues.line_width = LineWidth(ops->lineWidth);
  gcValues.cap_style = CapProjecting;

  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (axisPtr->tickGC) {
    Tk_FreeGC(graphPtr->display, axisPtr->tickGC);
  }
  axisPtr->tickGC = newGC;

  /* Assuming settings from above GC */
  gcValues.foreground = ops->activeFgColor->pixel;
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (axisPtr->activeTickGC) {
    Tk_FreeGC(graphPtr->display, axisPtr->activeTickGC);
  }
  axisPtr->activeTickGC = newGC;

  gcValues.background = gcValues.foreground = ops->major.color->pixel;
  gcValues.line_width = LineWidth(ops->major.lineWidth);
  gcMask = (GCForeground | GCBackground | GCLineWidth);
  if (LineIsDashed(ops->major.dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(ops->major.dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &ops->major.dashes);
  }
  if (ops->major.gc) {
    Blt_FreePrivateGC(graphPtr->display, ops->major.gc);
  }
  ops->major.gc = newGC;

  gcValues.background = gcValues.foreground = ops->minor.color->pixel;
  gcValues.line_width = LineWidth(ops->minor.lineWidth);
  gcMask = (GCForeground | GCBackground | GCLineWidth);
  if (LineIsDashed(ops->minor.dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(ops->minor.dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &ops->minor.dashes);
  }
  if (ops->minor.gc) {
    Blt_FreePrivateGC(graphPtr->display, ops->minor.gc);
  }
  ops->minor.gc = newGC;
}

static float titleAngle[4] =		/* Rotation for each axis title */
  {
    0.0, 90.0, 0.0, 270.0
  };

static void AxisOffsets(Axis *axisPtr, int margin, int offset,
			AxisInfo *infoPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;
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
  if (ops->lineWidth > 0) {
    if (ops->showTicks) {
      t1 = ops->tickLength;
      t2 = (t1 * 10) / 15;
    }
    labelOffset = t1 + AXIS_PAD_TITLE;
    if (ops->exterior)
      labelOffset += ops->lineWidth;
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
  inset = pad + ops->lineWidth / 2;
  switch (margin) {
  case MARGIN_TOP:
    axisLine = graphPtr->top;
    if (ops->exterior) {
      axisLine -= graphPtr->plotBW + axisPad + ops->lineWidth / 2;
      tickLabel = axisLine - 2;
      if (ops->lineWidth > 0) {
	tickLabel -= ops->tickLength;
      }
    } else {
      if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
	axisLine--;
      } 
      axisLine -= axisPad + ops->lineWidth / 2;
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
    if (ops->titleAlternate) {
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
    axisPtr->titlePos_.x = x;
    axisPtr->titlePos_.y = y;
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
    if (ops->exterior) {
      axisLine += graphPtr->plotBW + axisPad + ops->lineWidth / 2;
      tickLabel = axisLine + 2;
      if (ops->lineWidth > 0) {
	tickLabel += ops->tickLength;
      }
    } else {
      axisLine -= axisPad + ops->lineWidth / 2;
      tickLabel = graphPtr->bottom +  graphPtr->plotBW + 2;
    }
    mark = graphPtr->bottom + offset;
    fangle = fmod(ops->tickAngle, 90.0);
    if (fangle == 0.0) {
      axisPtr->tickAnchor = TK_ANCHOR_N;
    } else {
      int quadrant;

      quadrant = (int)(ops->tickAngle / 90.0);
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
    if (ops->titleAlternate) {
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
    axisPtr->titlePos_.x = x;
    axisPtr->titlePos_.y = y;
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
    if (ops->exterior) {
      axisLine -= graphPtr->plotBW + axisPad + ops->lineWidth / 2;
      tickLabel = axisLine - 2;
      if (ops->lineWidth > 0) {
	tickLabel -= ops->tickLength;
      }
    } else {
      if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
	axisLine--;
      } 
      axisLine += axisPad + ops->lineWidth / 2;
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
    if (ops->titleAlternate) {
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
    axisPtr->titlePos_.x = x;
    axisPtr->titlePos_.y = y;
    break;

  case MARGIN_RIGHT:
    axisLine = graphPtr->right;
    if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
      axisLine++;			/* Draw axis line within solid plot
					 * border. */
    } 
    if (ops->exterior) {
      axisLine += graphPtr->plotBW + axisPad + ops->lineWidth / 2;
      tickLabel = axisLine + 2;
      if (ops->lineWidth > 0) {
	tickLabel += ops->tickLength;
      }
    } else {
      axisLine -= axisPad + ops->lineWidth / 2;
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
    if (ops->titleAlternate) {
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
    axisPtr->titlePos_.x = x;
    axisPtr->titlePos_.y = y;
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
  if (!ops->exterior) {
    /*infoPtr->label = axisLine + labelOffset - t1; */
    infoPtr->t1 = axisLine - t1;
    infoPtr->t2 = axisLine - t2;
  } 
}

static void MakeAxisLine(Axis *axisPtr, int line, Segment2d *sp)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  double min, max;

  min = axisPtr->axisRange.min;
  max = axisPtr->axisRange.max;
  if (ops->logScale) {
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
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();

  if (ops->logScale)
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
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();

  if (axisPtr->segments) {
    free(axisPtr->segments);
    axisPtr->segments = NULL;	
  }

  Ticks* t1Ptr = ops->t1UPtr ? ops->t1UPtr : axisPtr->t1Ptr;
  Ticks* t2Ptr = ops->t2UPtr ? ops->t2UPtr : axisPtr->t2Ptr;

  int nMajorTicks= t1Ptr ? t1Ptr->nTicks : 0;
  int nMinorTicks= t2Ptr ? t2Ptr->nTicks : 0;

  int arraySize = 1 + (nMajorTicks * (nMinorTicks + 1));
  Segment2d* segments = (Segment2d*)malloc(arraySize * sizeof(Segment2d));
  Segment2d* sp = segments;
  if (ops->lineWidth > 0) {
    /* Axis baseline */
    MakeAxisLine(axisPtr, infoPtr->axis, sp);
    sp++;
  }

  if (ops->showTicks) {
    int isHoriz = AxisIsHorizontal(axisPtr);
    for (int ii=0; ii<nMajorTicks; ii++) {
      double t1 = t1Ptr->values[ii];
      /* Minor ticks */
      for (int jj=0; jj<nMinorTicks; jj++) {
	double t2 = t1 + (axisPtr->majorSweep.step*t2Ptr->values[jj]);
	if (InRange(t2, &axisPtr->axisRange)) {
	  MakeTick(axisPtr, t2, infoPtr->t2, infoPtr->axis, sp);
	  sp++;
	}
      }
      if (!InRange(t1, &axisPtr->axisRange))
	continue;

      /* Major tick */
      MakeTick(axisPtr, t1, infoPtr->t1, infoPtr->axis, sp);
      sp++;
    }

    Blt_ChainLink link = Blt_Chain_FirstLink(axisPtr->tickLabels);
    double labelPos = (double)infoPtr->label;

    for (int ii=0; ii< nMajorTicks; ii++) {
      double t1 = t1Ptr->values[ii];
      if (ops->labelOffset)
	t1 += axisPtr->majorSweep.step * 0.5;

      if (!InRange(t1, &axisPtr->axisRange))
	continue;

      TickLabel* labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
      link = Blt_Chain_NextLink(link);
      Segment2d seg;
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
  Graph* graphPtr = axisPtr->graphPtr_;

  if (AxisIsHorizontal(axisPtr)) {
    axisPtr->screenMin = graphPtr->hOffset;
    axisPtr->width = graphPtr->right - graphPtr->left;
    axisPtr->screenRange = graphPtr->hRange;
  }
  else {
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
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();

  AxisInfo info;
  Graph* graphPtr = axisPtr->graphPtr_;
  unsigned int slice, w, h;

  if ((graphPtr->margins[axisPtr->margin_].axes->nLinks > 1) ||
      (ops->reqNumMajorTicks <= 0)) {
    ops->reqNumMajorTicks = 4;
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

  Blt_GetTextExtents(ops->tickFont, 0, "0", 1, &w, &h);
  axisPtr->screenMin += (slice * count) + 2 + h / 2;
  axisPtr->screenRange = slice - 2 * 2 - h;
  axisPtr->screenScale = 1.0f / axisPtr->screenRange;
  AxisOffsets(axisPtr, margin, 0, &info);
  MakeSegments(axisPtr, &info);
}

static double AdjustViewport(double offset, double windowSize)
{
  // Canvas-style scrolling allows the world to be scrolled within the window.
  if (windowSize > 1.0) {
    if (windowSize < (1.0 - offset))
      offset = 1.0 - windowSize;

    if (offset > 0.0)
      offset = 0.0;
  }
  else {
    if ((offset + windowSize) > 1.0)
      offset = 1.0 - windowSize;

    if (offset < 0.0)
      offset = 0.0;
  }
  return offset;
}

int GetAxisScrollInfo(Tcl_Interp* interp, 
			     int objc, Tcl_Obj* const objv[],
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
    if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK)
      return TCL_ERROR;

    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    if ((c == 'u') && (strncmp(string, "units", length) == 0))
      fract = count * scrollUnits;
    else if ((c == 'p') && (strncmp(string, "pages", length) == 0))
      /* A page is 90% of the view-able window. */
      fract = (int)(count * windowSize * 0.9 + 0.5);
    else if ((c == 'p') && (strncmp(string, "pixels", length) == 0))
      fract = count * scale;
    else {
      Tcl_AppendResult(interp, "unknown \"scroll\" units \"", string,
		       "\"", NULL);
      return TCL_ERROR;
    }
    offset += fract;
  } 
  else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
    double fract;

    /* moveto fraction */
    if (Tcl_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
      return TCL_ERROR;
    }
    offset = fract;
  } 
  else {
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
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;

  if (ops->normalBg) {
    Tk_Fill3DRectangle(graphPtr->tkwin, drawable, 
		       ops->normalBg, 
		       axisPtr->left, axisPtr->top, 
		       axisPtr->right - axisPtr->left, 
		       axisPtr->bottom - axisPtr->top,
		       ops->borderWidth, 
		       ops->relief);
  }
  if (ops->title) {
    TextStyle ts;

    Blt_Ts_InitStyle(ts);
    ts.flags |= UPDATE_GC;

    ts.angle = axisPtr->titleAngle;
    ts.font = ops->titleFont;
    ts.xPad = 1;
    ts.yPad = 0;
    ts.anchor = axisPtr->titleAnchor;
    ts.justify = ops->titleJustify;
    if (axisPtr->flags & ACTIVE)
      ts.color = ops->activeFgColor;
    else
      ts.color = ops->titleColor;

    if ((axisPtr->titleAngle == 90.0) || (axisPtr->titleAngle == 270.0))
      ts.maxLength = axisPtr->height;
    else
      ts.maxLength = axisPtr->width;

    Blt_Ts_DrawText(graphPtr->tkwin, drawable, ops->title, -1, &ts, 
		    (int)axisPtr->titlePos_.x, (int)axisPtr->titlePos_.y);
  }
  if (ops->scrollCmdObjPtr) {
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
    if (ops->logScale) {
      worldMin = log10(worldMin);
      worldMax = log10(worldMax);
      viewMin = log10(viewMin);
      viewMax = log10(viewMax);
    }
    worldWidth = worldMax - worldMin;	
    viewWidth = viewMax - viewMin;
    isHoriz = AxisIsHorizontal(axisPtr);

    if (isHoriz != ops->descending) {
      fract = (viewMin - worldMin) / worldWidth;
    } else {
      fract = (worldMax - viewMax) / worldWidth;
    }
    fract = AdjustViewport(fract, viewWidth / worldWidth);

    if (isHoriz != ops->descending) {
      viewMin = (fract * worldWidth);
      axisPtr->min = viewMin + worldMin;
      axisPtr->max = axisPtr->min + viewWidth;
      viewMax = viewMin + viewWidth;
      if (ops->logScale) {
	axisPtr->min = EXP10(axisPtr->min);
	axisPtr->max = EXP10(axisPtr->max);
      }
      Blt_UpdateScrollbar(graphPtr->interp, ops->scrollCmdObjPtr,
			  viewMin, viewMax, worldWidth);
    } else {
      viewMax = (fract * worldWidth);
      axisPtr->max = worldMax - viewMax;
      axisPtr->min = axisPtr->max - viewWidth;
      viewMin = viewMax + viewWidth;
      if (ops->logScale) {
	axisPtr->min = EXP10(axisPtr->min);
	axisPtr->max = EXP10(axisPtr->max);
      }
      Blt_UpdateScrollbar(graphPtr->interp, ops->scrollCmdObjPtr,
			  viewMax, viewMin, worldWidth);
    }
  }
  if (ops->showTicks) {
    Blt_ChainLink link;
    TextStyle ts;

    Blt_Ts_InitStyle(ts);
    ts.flags |= UPDATE_GC;

    ts.angle = ops->tickAngle;
    ts.font = ops->tickFont;
    ts.xPad = 2;
    ts.yPad = 0;
    ts.anchor = axisPtr->tickAnchor;
    if (axisPtr->flags & ACTIVE)
      ts.color = ops->activeFgColor;
    else
      ts.color = ops->tickColor;

    for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL;
	 link = Blt_Chain_NextLink(link)) {	
      TickLabel *labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
      Blt_DrawText(graphPtr->tkwin, drawable, labelPtr->string, &ts, 
		   (int)labelPtr->anchorPos.x, (int)labelPtr->anchorPos.y);
    }
  }

  if ((axisPtr->nSegments > 0) && (ops->lineWidth > 0)) {	
    GC gc = (axisPtr->flags & ACTIVE) ? axisPtr->activeTickGC : axisPtr->tickGC;
    Blt_Draw2DSegments(graphPtr->display, drawable, gc, axisPtr->segments, 
		       axisPtr->nSegments);
  }
}

static void AxisToPostScript(Blt_Ps ps, Axis *axisPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Blt_Ps_Format(ps, "%% Axis \"%s\"\n", axisPtr->name());
  if (ops->normalBg)
    Blt_Ps_Fill3DRectangle(ps, ops->normalBg, 
			   (double)axisPtr->left, (double)axisPtr->top, 
			   axisPtr->right - axisPtr->left, 
			   axisPtr->bottom - axisPtr->top, 
			   ops->borderWidth, ops->relief);

  if (ops->title) {
    TextStyle ts;

    Blt_Ts_InitStyle(ts);

    ts.angle = axisPtr->titleAngle;
    ts.font = ops->titleFont;
    ts.xPad = 1;
    ts.yPad = 0;
    ts.anchor = axisPtr->titleAnchor;
    ts.justify = ops->titleJustify;
    ts.color = ops->titleColor;
    Blt_Ps_DrawText(ps, ops->title, &ts, axisPtr->titlePos_.x, 
		    axisPtr->titlePos_.y);
  }

  if (ops->showTicks) {
    TextStyle ts;

    Blt_Ts_InitStyle(ts);

    ts.angle = ops->tickAngle;
    ts.font = ops->tickFont;
    ts.xPad = 2;
    ts.yPad = 0;
    ts.anchor = axisPtr->tickAnchor;
    ts.color = ops->tickColor;

    for (Blt_ChainLink link=Blt_Chain_FirstLink(axisPtr->tickLabels); link; 
	 link = Blt_Chain_NextLink(link)) {
      TickLabel *labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
      Blt_Ps_DrawText(ps, labelPtr->string, &ts, labelPtr->anchorPos.x, 
		      labelPtr->anchorPos.y);
    }
  }
  if ((axisPtr->nSegments > 0) && (ops->lineWidth > 0)) {
    Blt_Ps_XSetLineAttributes(ps, ops->tickColor, ops->lineWidth, 
			      (Blt_Dashes *)NULL, CapButt, JoinMiter);
    Blt_Ps_Draw2DSegments(ps, axisPtr->segments, axisPtr->nSegments);
  }
}

static void MakeGridLine(Axis *axisPtr, double value, Segment2d *sp)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;

  if (ops->logScale) {
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
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
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
  if (ops->showGridMinor)
    needed += (t1Ptr->nTicks * t2Ptr->nTicks);

  if (needed == 0) {
    // Free generated ticks
    if (t1Ptr != axisPtr->t1Ptr)
      free(t1Ptr);
    if (t2Ptr != axisPtr->t2Ptr)
      free(t2Ptr);
    return;			
  }

  needed = t1Ptr->nTicks;
  if (needed != ops->major.nAllocated) {
    if (ops->major.segments) {
      free(ops->major.segments);
      ops->major.segments = NULL;
    }
    ops->major.segments = (Segment2d*)malloc(sizeof(Segment2d) * needed);
    ops->major.nAllocated = needed;
  }
  needed = (t1Ptr->nTicks * t2Ptr->nTicks);
  if (needed != ops->minor.nAllocated) {
    if (ops->minor.segments) {
      free(ops->minor.segments);
      ops->minor.segments = NULL;
    }
    ops->minor.segments = (Segment2d*)malloc(sizeof(Segment2d) * needed);
    ops->minor.nAllocated = needed;
  }
  s1 = ops->major.segments, s2 = ops->minor.segments;
  for (i = 0; i < t1Ptr->nTicks; i++) {
    double value;

    value = t1Ptr->values[i];
    if (ops->showGridMinor) {
      int j;

      for (j = 0; j < t2Ptr->nTicks; j++) {
	double subValue = value + (axisPtr->majorSweep.step * 
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

  // Free generated ticks
  if (t1Ptr != axisPtr->t1Ptr)
    free(t1Ptr);
  if (t2Ptr != axisPtr->t2Ptr)
    free(t2Ptr);

  ops->major.nUsed = s1 - ops->major.segments;
  ops->minor.nUsed = s2 - ops->minor.segments;
}

static void GetAxisGeometry(Graph* graphPtr, Axis *axisPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  FreeTickLabels(axisPtr->tickLabels);

  // Leave room for axis baseline and padding
  unsigned int y =0;
  if (ops->exterior && (graphPtr->plotRelief != TK_RELIEF_SOLID))
    y += ops->lineWidth + 2;

  axisPtr->maxTickHeight = axisPtr->maxTickWidth = 0;

  if (axisPtr->t1Ptr)
    free(axisPtr->t1Ptr);
  axisPtr->t1Ptr = GenerateTicks(&axisPtr->majorSweep);
  if (axisPtr->t2Ptr)
    free(axisPtr->t2Ptr);
  axisPtr->t2Ptr = GenerateTicks(&axisPtr->minorSweep);

  if (ops->showTicks) {
    Ticks* t1Ptr = ops->t1UPtr ? ops->t1UPtr : axisPtr->t1Ptr;
	
    int nTicks =0;
    if (t1Ptr)
      nTicks = t1Ptr->nTicks;
	
    unsigned int nLabels =0;
    for (int ii=0; ii<nTicks; ii++) {
      double x = t1Ptr->values[ii];
      double x2 = t1Ptr->values[ii];
      if (ops->labelOffset)
	x2 += axisPtr->majorSweep.step * 0.5;

      if (!InRange(x2, &axisPtr->axisRange))
	continue;

      TickLabel* labelPtr = MakeLabel(axisPtr, x);
      Blt_Chain_Append(axisPtr->tickLabels, labelPtr);
      nLabels++;
      /* 
       * Get the dimensions of each tick label.  Remember tick labels
       * can be multi-lined and/or rotated.
       */
      unsigned int lw, lh;	/* Label width and height. */
      Blt_GetTextExtents(ops->tickFont, 0, labelPtr->string, -1, &lw, &lh);
      labelPtr->width  = lw;
      labelPtr->height = lh;

      if (ops->tickAngle != 0.0f) {
	double rlw, rlh;	/* Rotated label width and height. */
	Blt_GetBoundingBox(lw, lh, ops->tickAngle, &rlw, &rlh, NULL);
	lw = ROUND(rlw), lh = ROUND(rlh);
      }
      if (axisPtr->maxTickWidth < int(lw))
	axisPtr->maxTickWidth = lw;

      if (axisPtr->maxTickHeight < int(lh))
	axisPtr->maxTickHeight = lh;
    }
	
    unsigned int pad =0;
    if (ops->exterior) {
      /* Because the axis cap style is "CapProjecting", we need to
       * account for an extra 1.5 linewidth at the end of each line.  */
      pad = ((ops->lineWidth * 12) / 8);
    }
    if (AxisIsHorizontal(axisPtr))
      y += axisPtr->maxTickHeight + pad;
    else {
      y += axisPtr->maxTickWidth + pad;
      if (axisPtr->maxTickWidth > 0)
	// Pad either size of label.
	y += 5;
    }
    y += 2 * AXIS_PAD_TITLE;
    if ((ops->lineWidth > 0) && ops->exterior)
      // Distance from axis line to tick label.
      y += ops->tickLength;

  } // showTicks

  if (ops->title) {
    if (ops->titleAlternate) {
      if (y < axisPtr->titleHeight_)
	y = axisPtr->titleHeight_;
    } 
    else
      y += axisPtr->titleHeight_ + AXIS_PAD_TITLE;
  }

  // Correct for orientation of the axis
  if (AxisIsHorizontal(axisPtr))
    axisPtr->height = y;
  else
    axisPtr->width = y;
}

static int GetMarginGeometry(Graph* graphPtr, Margin *marginPtr)
{
  int isHoriz = HORIZMARGIN(marginPtr);

  // Count the visible axes.
  unsigned int nVisible = 0;
  unsigned int l =0;
  int w =0;
  int h =0;

  marginPtr->maxTickWidth =0;
  marginPtr->maxTickHeight =0;

  if (graphPtr->stackAxes) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(marginPtr->axes);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_) {
	nVisible++;
	if (graphPtr->flags & GET_AXIS_GEOMETRY)
	  GetAxisGeometry(graphPtr, axisPtr);

	if (isHoriz) {
	  if (h < axisPtr->height)
	    h = axisPtr->height;
	}
	else {
	  if (w < axisPtr->width)
	    w = axisPtr->width;
	}
	if (axisPtr->maxTickWidth > marginPtr->maxTickWidth)
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth;

	if (axisPtr->maxTickHeight > marginPtr->maxTickHeight)
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight;
      }
    }
  }
  else {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(marginPtr->axes);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_) {
	nVisible++;
	if (graphPtr->flags & GET_AXIS_GEOMETRY)
	  GetAxisGeometry(graphPtr, axisPtr);

	if ((ops->titleAlternate) && (l < axisPtr->titleWidth_))
	  l = axisPtr->titleWidth_;

	if (isHoriz)
	  h += axisPtr->height;
	else
	  w += axisPtr->width;

	if (axisPtr->maxTickWidth > marginPtr->maxTickWidth)
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth;

	if (axisPtr->maxTickHeight > marginPtr->maxTickHeight)
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight;
      }
    }
  }
  // Enforce a minimum size for margins.
  if (w < 3)
    w = 3;

  if (h < 3)
    h = 3;

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
  int titleY;
  int plotWidth, plotHeight;
  int inset, inset2;

  int width = graphPtr->width;
  int height = graphPtr->height;

  /* 
   * Step 1:  Compute the amount of space needed to display the axes
   *		associated with each margin.  They can be overridden by 
   *		-leftmargin, -rightmargin, -bottommargin, and -topmargin
   *		graph options, respectively.
   */
  int left   = GetMarginGeometry(graphPtr, &graphPtr->leftMargin);
  int right  = GetMarginGeometry(graphPtr, &graphPtr->rightMargin);
  int top    = GetMarginGeometry(graphPtr, &graphPtr->topMargin);
  int bottom = GetMarginGeometry(graphPtr, &graphPtr->bottomMargin);

  int pad = graphPtr->bottomMargin.maxTickWidth;
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
  if (graphPtr->title)
    top += graphPtr->titleHeight + 6;

  inset = (graphPtr->inset + graphPtr->plotBW);
  inset2 = 2 * inset;

  /* 
   * Step 3: Estimate the size of the plot area from the remaining
   *	       space.  This may be overridden by the -plotwidth and
   *	       -plotheight graph options.  We use this to compute the
   *	       size of the legend. 
   */
  if (width == 0)
    width = 400;

  if (height == 0)
    height = 400;

  plotWidth  = (graphPtr->reqPlotWidth > 0) ? graphPtr->reqPlotWidth :
    width - (inset2 + left + right); /* Plot width. */
  plotHeight = (graphPtr->reqPlotHeight > 0) ? graphPtr->reqPlotHeight : 
    height - (inset2 + top + bottom); /* Plot height. */
  graphPtr->legend->map(plotWidth, plotHeight);

  /* 
   * Step 2:  Add the legend to the appropiate margin. 
   */
  if (!graphPtr->legend->isHidden()) {
    switch (graphPtr->legend->position()) {
    case Legend::RIGHT:
      right += graphPtr->legend->width() + 2;
      break;
    case Legend::LEFT:
      left += graphPtr->legend->width() + 2;
      break;
    case Legend::TOP:
      top += graphPtr->legend->height() + 2;
      break;
    case Legend::BOTTOM:
      bottom += graphPtr->legend->height() + 2;
      break;
    case Legend::XY:
    case Legend::PLOT:
      /* Do nothing. */
      break;
    }
  }

  /* 
   * Recompute the plotarea or graph size, now accounting for the legend. 
   */
  if (graphPtr->reqPlotWidth == 0) {
    plotWidth = width  - (inset2 + left + right);
    if (plotWidth < 1)
      plotWidth = 1;
  }
  if (graphPtr->reqPlotHeight == 0) {
    plotHeight = height - (inset2 + top + bottom);
    if (plotHeight < 1)
      plotHeight = 1;
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
      // Shrink the width
      int scaledWidth = (int)(plotHeight * graphPtr->aspect);
      if (scaledWidth < 1)
	scaledWidth = 1;

      // Add the difference to the right margin.
      // CHECK THIS: w = scaledWidth
      right += (plotWidth - scaledWidth);
    }
    else {
      // Shrink the height
      int scaledHeight = (int)(plotWidth / graphPtr->aspect);
      if (scaledHeight < 1)
	scaledHeight = 1;

      // Add the difference to the top margin
      // CHECK THIS: h = scaledHeight;
      top += (plotHeight - scaledHeight); 
    }
  }

  /* 
   * Step 6: If there's multiple axes in a margin, the axis titles will be
   *	       displayed in the adjoining margins.  Make sure there's room 
   *	       for the longest axis titles.
   */

  if (top < graphPtr->leftMargin.axesTitleLength)
    top = graphPtr->leftMargin.axesTitleLength;

  if (right < graphPtr->bottomMargin.axesTitleLength)
    right = graphPtr->bottomMargin.axesTitleLength;

  if (top < graphPtr->rightMargin.axesTitleLength)
    top = graphPtr->rightMargin.axesTitleLength;

  if (right < graphPtr->topMargin.axesTitleLength)
    right = graphPtr->topMargin.axesTitleLength;

  /* 
   * Step 7: Override calculated values with requested margin sizes.
   */
  if (graphPtr->leftMargin.reqSize > 0)
    left = graphPtr->leftMargin.reqSize;

  if (graphPtr->rightMargin.reqSize > 0)
    right = graphPtr->rightMargin.reqSize;

  if (graphPtr->topMargin.reqSize > 0)
    top = graphPtr->topMargin.reqSize;

  if (graphPtr->bottomMargin.reqSize > 0)
    bottom = graphPtr->bottomMargin.reqSize;

  if (graphPtr->reqPlotWidth > 0) {	
    /* 
     * Width of plotarea is constained.  If there's extra space, add it to
     * th left and/or right margins.  If there's too little, grow the
     * graph width to accomodate it.
     */
    int w = plotWidth + inset2 + left + right;
    if (width > w) {		/* Extra space in window. */
      int extra = (width - w) / 2;
      if (graphPtr->leftMargin.reqSize == 0) { 
	left += extra;
	if (graphPtr->rightMargin.reqSize == 0) { 
	  right += extra;
	}
	else {
	  left += extra;
	}
      }
      else if (graphPtr->rightMargin.reqSize == 0) {
	right += extra + extra;
      }
    }
    else if (width < w) {
      width = w;
    }
  } 
  if (graphPtr->reqPlotHeight > 0) {	/* Constrain the plotarea height. */
    /* 
     * Height of plotarea is constained.  If there's extra space, 
     * add it to th top and/or bottom margins.  If there's too little,
     * grow the graph height to accomodate it.
     */
    int h = plotHeight + inset2 + top + bottom;
    if (height > h) {		/* Extra space in window. */
      int extra;

      extra = (height - h) / 2;
      if (graphPtr->topMargin.reqSize == 0) { 
	top += extra;
	if (graphPtr->bottomMargin.reqSize == 0) { 
	  bottom += extra;
	}
	else {
	  top += extra;
	}
      }
      else if (graphPtr->bottomMargin.reqSize == 0) {
	bottom += extra + extra;
      }
    }
    else if (height < h) {
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

  if (graphPtr->vRange < 1)
    graphPtr->vRange = 1;

  if (graphPtr->hRange < 1)
    graphPtr->hRange = 1;

  graphPtr->hScale = 1.0f / (float)graphPtr->hRange;
  graphPtr->vScale = 1.0f / (float)graphPtr->vRange;

  // Calculate the placement of the graph title so it is centered within the
  // space provided for it in the top margin
  titleY = graphPtr->titleHeight;
  graphPtr->titleY = 3 + graphPtr->inset;
  graphPtr->titleX = (graphPtr->right + graphPtr->left) / 2;
}

int ConfigureAxis(Axis *axisPtr)
{
  AxisOptions* ops = (AxisOptions*)axisPtr->ops();
  Graph* graphPtr = axisPtr->graphPtr_;
  float angle;

  /* Check the requested axis limits. Can't allow -min to be greater than
   * -max.  Do this regardless of -checklimits option. We want to always 
   * detect when the user has zoomed in beyond the precision of the data.*/
  if (((!isnan(ops->reqMin)) && (!isnan(ops->reqMax))) &&
      (ops->reqMin >= ops->reqMax)) {
    char msg[200];
    sprintf_s(msg, 200, 
	      "impossible axis limits (-min %g >= -max %g) for \"%s\"",
	      ops->reqMin, ops->reqMax, axisPtr->name());
    Tcl_AppendResult(graphPtr->interp, msg, NULL);
    return TCL_ERROR;
  }
  axisPtr->scrollMin = ops->reqScrollMin;
  axisPtr->scrollMax = ops->reqScrollMax;
  if (ops->logScale) {
    if (ops->checkLimits) {
      /* Check that the logscale limits are positive.  */
      if ((!isnan(ops->reqMin)) && (ops->reqMin <= 0.0)) {
	Tcl_AppendResult(graphPtr->interp,"bad logscale -min limit \"", 
			 Blt_Dtoa(graphPtr->interp, ops->reqMin), 
			 "\" for axis \"", axisPtr->name(), "\"", 
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
  angle = fmod(ops->tickAngle, 360.0);
  if (angle < 0.0f)
    angle += 360.0f;

  ops->tickAngle = angle;
  ResetTextStyles(axisPtr);

  axisPtr->titleWidth_ = axisPtr->titleHeight_ = 0;
  if (ops->title) {
    unsigned int w, h;

    Blt_GetTextExtents(ops->titleFont, 0, ops->title, -1, &w, &h);
    axisPtr->titleWidth_ = (unsigned short int)w;
    axisPtr->titleHeight_ = (unsigned short int)h;
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

void Blt_DestroyAxes(Graph* graphPtr)
{
  Tcl_HashSearch cursor;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    axisPtr->hashPtr_ = NULL;
    delete axisPtr;
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
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
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
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!axisPtr->use_ || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (graphPtr->stackAxes) {
	if (ops->reqNumMajorTicks <= 0)
	  ops->reqNumMajorTicks = 4;

	MapStackedAxis(axisPtr, count, margin);
      } 
      else {
	if (ops->reqNumMajorTicks <= 0)
	  ops->reqNumMajorTicks = 4;

	MapAxis(axisPtr, offset, margin);
      }

      if (ops->showGrid)
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
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_ && !(axisPtr->flags & DELETE_PENDING))
	DrawAxis(axisPtr, drawable);
    }
  }
}

void Blt_DrawGrids(Graph* graphPtr, Drawable drawable) 
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->margins[i].axes); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (ops->hide || (axisPtr->flags & DELETE_PENDING))
	continue;

      if (axisPtr->use_ && ops->showGrid) {
	Blt_Draw2DSegments(graphPtr->display, drawable, 
			   ops->major.gc, ops->major.segments, 
			   ops->major.nUsed);

	if (ops->showGridMinor)
	  Blt_Draw2DSegments(graphPtr->display, drawable, 
			     ops->minor.gc, ops->minor.segments, 
			     ops->minor.nUsed);
      }
    }
  }
}

void Blt_GridsToPostScript(Graph* graphPtr, Blt_Ps ps) 
{
  for (int i = 0; i < 4; i++) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->margins[i].axes); link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (ops->hide || !ops->showGrid || !axisPtr->use_ || 
	  (axisPtr->flags & DELETE_PENDING))
	continue;

      Blt_Ps_Format(ps, "%% Axis %s: grid line attributes\n",
		    axisPtr->name());
      Blt_Ps_XSetLineAttributes(ps, ops->major.color, 
				ops->major.lineWidth, 
				&ops->major.dashes, CapButt, 
				JoinMiter);
      Blt_Ps_Format(ps, "%% Axis %s: major grid line segments\n",
		    axisPtr->name());
      Blt_Ps_Draw2DSegments(ps, ops->major.segments, 
			    ops->major.nUsed);
      if (ops->showGridMinor) {
	Blt_Ps_XSetLineAttributes(ps, ops->minor.color, 
				  ops->minor.lineWidth, 
				  &ops->minor.dashes, CapButt, 
				  JoinMiter);
	Blt_Ps_Format(ps, "%% Axis %s: minor grid line segments\n",
		      axisPtr->name());
	Blt_Ps_Draw2DSegments(ps, ops->minor.segments, 
			      ops->minor.nUsed);
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
      Axis *axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_ && !(axisPtr->flags & DELETE_PENDING))
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
    Dim2D textDim;
    const char *minFmt, *maxFmt;
    char *minPtr, *maxPtr;
    int isHoriz;

    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    ops->limitsTextStyle.flags |= UPDATE_GC;

    if (axisPtr->flags & DELETE_PENDING)
      continue;

    if (!ops->limitsFormat)
      continue;

    isHoriz = AxisIsHorizontal(axisPtr);
    minPtr = maxPtr = NULL;
    minFmt = maxFmt = ops->limitsFormat;
    if (minFmt[0] != '\0') {
      minPtr = minString;
      sprintf_s(minString, 200, minFmt, axisPtr->axisRange.min);
    }
    if (maxFmt[0] != '\0') {
      maxPtr = maxString;
      sprintf_s(maxString, 200, maxFmt, axisPtr->axisRange.max);
    }
    if (ops->descending) {
      char *tmp;

      tmp = minPtr, minPtr = maxPtr, maxPtr = tmp;
    }
    if (maxPtr) {
      if (isHoriz) {
	ops->limitsTextStyle.angle = 90.0;
	ops->limitsTextStyle.anchor = TK_ANCHOR_SE;

	Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		      &ops->limitsTextStyle, graphPtr->right, 
		      hMax, &textDim);
	hMax -= (textDim.height + SPACING);
      } 
      else {
	ops->limitsTextStyle.angle = 0.0;
	ops->limitsTextStyle.anchor = TK_ANCHOR_NW;

	Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		      &ops->limitsTextStyle, vMax, 
		      graphPtr->top, &textDim);
	vMax += (textDim.width + SPACING);
      }
    }
    if (minPtr) {
      ops->limitsTextStyle.anchor = TK_ANCHOR_SW;

      if (isHoriz) {
	ops->limitsTextStyle.angle = 90.0;

	Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		      &ops->limitsTextStyle, graphPtr->left, 
		      hMin, &textDim);
	hMin -= (textDim.height + SPACING);
      } 
      else {
	ops->limitsTextStyle.angle = 0.0;

	Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		      &ops->limitsTextStyle, vMin, 
		      graphPtr->bottom, &textDim);
	vMin += (textDim.width + SPACING);
      }
    }
  }
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
    const char *minFmt, *maxFmt;
    unsigned int textWidth, textHeight;

    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();

    if (axisPtr->flags & DELETE_PENDING)
      continue;

    if (!ops->limitsFormat)
      continue;

    minFmt = maxFmt = ops->limitsFormat;
    if (*maxFmt != '\0') {
      sprintf_s(string, 200, maxFmt, axisPtr->axisRange.max);
      Blt_GetTextExtents(ops->tickFont, 0, string, -1, &textWidth,
			 &textHeight);
      if ((textWidth > 0) && (textHeight > 0)) {
	if (axisPtr->classId() == CID_AXIS_X) {
	  ops->limitsTextStyle.angle = 90.0;
	  ops->limitsTextStyle.anchor = TK_ANCHOR_SE;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle, 
			  (double)graphPtr->right, hMax);
	  hMax -= (textWidth + SPACING);
	} 
	else {
	  ops->limitsTextStyle.angle = 0.0;
	  ops->limitsTextStyle.anchor = TK_ANCHOR_NW;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle,
			  vMax, (double)graphPtr->top);
	  vMax += (textWidth + SPACING);
	}
      }
    }
    if (*minFmt != '\0') {
      sprintf_s(string, 200, minFmt, axisPtr->axisRange.min);
      Blt_GetTextExtents(ops->tickFont, 0, string, -1, &textWidth,
			 &textHeight);
      if ((textWidth > 0) && (textHeight > 0)) {
	ops->limitsTextStyle.anchor = TK_ANCHOR_SW;
	if (axisPtr->classId() == CID_AXIS_X) {
	  ops->limitsTextStyle.angle = 90.0;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle, 
			  (double)graphPtr->left, hMin);
	  hMin -= (textWidth + SPACING);
	}
	else {
	  ops->limitsTextStyle.angle = 0.0;

	  Blt_Ps_DrawText(ps, string, &ops->limitsTextStyle, 
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

  return (Axis*)Blt_Chain_GetValue(link);
}

Axis *Blt_NearestAxis(Graph* graphPtr, int x, int y)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch cursor;
    
  for (hPtr = Tcl_FirstHashEntry(&graphPtr->axes.table, &cursor); 
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&cursor)) {
    Axis *axisPtr = (Axis*)Tcl_GetHashValue(hPtr);
    AxisOptions* ops = (AxisOptions*)axisPtr->ops();
    if (ops->hide || !axisPtr->use_ || (axisPtr->flags & DELETE_PENDING))
      continue;

    if (ops->showTicks) {
      Blt_ChainLink link;

      for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
	   link = Blt_Chain_NextLink(link)) {	
	Point2d t;
	double rw, rh;
	Point2d bbox[5];

	TickLabel *labelPtr = (TickLabel*)Blt_Chain_GetValue(link);
	Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			   ops->tickAngle, &rw, &rh, bbox);
	t = Blt_AnchorPoint(labelPtr->anchorPos.x, 
			    labelPtr->anchorPos.y, rw, rh, axisPtr->tickAnchor);
	t.x = x - t.x - (rw * 0.5);
	t.y = y - t.y - (rh * 0.5);

	bbox[4] = bbox[0];
	if (Blt_PointInPolygon(&t, bbox, 5)) {
	  axisPtr->detail_ = "label";
	  return axisPtr;
	}
      }
    }
    if (ops->title) {	/* and then the title string. */
      Point2d bbox[5];
      Point2d t;
      double rw, rh;
      unsigned int w, h;

      Blt_GetTextExtents(ops->titleFont, 0, ops->title,-1,&w,&h);
      Blt_GetBoundingBox(w, h, axisPtr->titleAngle, &rw, &rh, bbox);
      t = Blt_AnchorPoint(axisPtr->titlePos_.x, axisPtr->titlePos_.y, 
			  rw, rh, axisPtr->titleAnchor);
      /* Translate the point so that the 0,0 is the upper left 
       * corner of the bounding box.  */
      t.x = x - t.x - (rw * 0.5);
      t.y = y - t.y - (rh * 0.5);
	    
      bbox[4] = bbox[0];
      if (Blt_PointInPolygon(&t, bbox, 5)) {
	axisPtr->detail_ = "title";
	return axisPtr;
      }
    }
    if (ops->lineWidth > 0) {	/* Check for the axis region */
      if ((x <= axisPtr->right) && (x >= axisPtr->left) && 
	  (y <= axisPtr->bottom) && (y >= axisPtr->top)) {
	axisPtr->detail_ = "line";
	return axisPtr;
      }
    }
  }
  return NULL;
}
 
ClientData Blt_MakeAxisTag(Graph* graphPtr, const char *tagName)
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&graphPtr->axes.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->axes.tagTable, hPtr);
}
