/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrAxis.h --
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
 *	KIND, EXPRESS OR IMPIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _BLT_GR_AXIS_H
#define _BLT_GR_AXIS_H

#include "bltList.h"
#include "bltConfig.h"

typedef struct {
  Blt_Dashes dashes;		/* Dash style of the grid. This represents an
				 * array of alternatingly drawn pixel
				 * values. */
  int lineWidth;		/* Width of the grid lines */
  XColor* color;		/* Color of the grid lines */
  GC gc;			/* Graphics context for the grid. */

  Segment2d *segments;	/* Array of line segments representing the
			 * grid lines */
  int nUsed;			/* # of axis segments in use. */
  int nAllocated;		/* # of axis segments allocated. */
} Grid;

typedef struct {
  double min, max, range, scale;
} AxisRange;

typedef struct {
  Point2d anchorPos;
  unsigned int width, height;
  char string[1];
} TickLabel;

typedef struct {
  unsigned int nTicks;	/* # of ticks on axis */
  double values[1];		/* Array of tick values (malloc-ed). */
} Ticks;

typedef struct {
  double initial;		/* Initial value */
  double step;		/* Size of interval */
  unsigned int nSteps;	/* Number of intervals. */
} TickSweep;

typedef struct {
  GraphObj obj;			/* Must be first field in axis. */

  int use;
  int hide;
  int showTicks;
  int showGrid;
  int showGridMinor;
  int exterior;
  int checkLimits;
  unsigned int flags;		

  Tk_OptionTable optionTable;	/* Configuration specifications */
  Tcl_HashEntry *hashPtr;

  /* Fields specific to axes. */

  const char *detail;

  int refCount;			/* Number of elements referencing this
				 * axis. */
  int logScale;			/* If non-zero, generate log scale
				 * ticks for the axis. */
  int descending;			/* If non-zero, display the range of
					 * values on the axis in descending
					 * order, from high to low. */

  int looseMin, looseMax;		/* If zero, axis range extends to
					 * the outer major ticks, otherwise at
					 * the limits of the data values. This
					 * is overriddened by setting the -min
					 * and -max options.  */

  const char *title;			/* Title of the axis. */

  int titleAlternate;			/* Indicates whether to position the
					 * title above/left of the axis. */

  Point2d titlePos;			/* Position of the title */

  unsigned short int titleWidth, titleHeight;	


  int lineWidth;			/* Width of lines representing axis
					 * (including ticks).  If zero, then
					 * no axis lines or ticks are
					 * drawn. */

  const char *limitsFormat;		/* String of sprintf-like
					 * formats describing how to display
					 * virtual axis limits. If NULL,
					 * display no limits. */

  TextStyle limitsTextStyle;		/* Text attributes (color, font,
					 * rotation, etc.)  of the limits. */

  double windowSize;			/* Size of a sliding window of values
					 * used to scale the axis
					 * automatically as new data values
					 * are added. The axis will always
					 * display the latest values in this
					 * range. */

  double shiftBy;			/* Shift maximum by this interval. */

  int tickLength;			/* Length of major ticks in pixels */

  const char *formatCmd;		/* Specifies a TCL command, to be
					 * invoked by the axis whenever it has
					 * to generate tick labels. */

  Tcl_Obj *scrollCmdObjPtr;
  int scrollUnits;

  double min, max;			/* The actual axis range. */

  double reqMin, reqMax;		/* Requested axis bounds. Consult the
					 * axisPtr->flags field for
					 * AXIS_CONFIG_MIN and AXIS_CONFIG_MAX
					 * to see if the requested bound have
					 * been set.  They override the
					 * computed range of the axis
					 * (determined by auto-scaling). */

  double reqScrollMin, reqScrollMax;

  double scrollMin, scrollMax;	/* Defines the scrolling reqion of the
				 * axis.  Normally the region is
				 * determined from the data limits. If
				 * specified, these values override
				 * the data-range. */

  AxisRange valueRange;		/* Range of data values of elements
				 * mapped to this axis. This is used
				 * to auto-scale the axis in "tight"
				 * mode. */
  AxisRange axisRange;		/* Smallest and largest major tick
				 * values for the axis.  The tick
				 * values lie outside the range of
				 * data values.  This is used to
				 * auto-scale the axis in "loose"
				 * mode. */

  double prevMin, prevMax;

  double reqStep;		/* If > 0.0, overrides the computed major 
				 * tick interval.  Otherwise a stepsize 
				 * is automatically calculated, based 
				 * upon the range of elements mapped to the 
				 * axis. The default value is 0.0. */

  Ticks *t1Ptr;		/* Array of major tick positions. May be
			 * set by the user or generated from the 
			 * major sweep below. */

  Ticks *t2Ptr;		/* Array of minor tick positions. May be
			 * set by the user or generated from the
			 * minor sweep below. */

  TickSweep minorSweep, majorSweep;

  int reqNumMajorTicks;	/* Default number of ticks to be displayed. */
  int reqNumMinorTicks;	/* If non-zero, represents the
			 * requested the number of minor ticks
			 * to be uniformally displayed along
			 * each major tick. */


  int labelOffset;		/* If non-zero, indicates that the tick
				 * label should be offset to sit in the
				 * middle of the next interval. */

  /* The following fields are specific to logical axes */

  int margin;				/* Margin that contains this axis. */
  Blt_ChainLink link;			/* Axis link in margin list. */
  Blt_Chain chain;
  Segment2d *segments;		/* Array of line segments representing
				 * the major and minor ticks, but also
				 * the * axis line itself. The segment
				 * coordinates * are relative to the
				 * axis. */
  int nSegments;			/* Number of segments in the above
					 * array. */
  Blt_Chain tickLabels;		/* Contains major tick label strings
				 * and their offsets along the
				 * axis. */
  short int left, right, top, bottom;	/* Region occupied by the of axis. */
  short int width, height;		/* Extents of axis */
  short int maxTickWidth, maxTickHeight;
  Tk_3DBorder normalBg;
  XColor* activeFgColor;

  int relief;
  int borderWidth;
  int activeRelief;

  double tickAngle;	
  Tk_Font tickFont;
  Tk_Anchor tickAnchor;
  Tk_Anchor reqTickAnchor;
  XColor* tickColor;
  GC tickGC;				/* Graphics context for axis and tick
					 * labels */
  GC activeTickGC;

  double titleAngle;	
  Tk_Font titleFont;
  Tk_Anchor titleAnchor;
  Tk_Justify titleJustify;
  XColor* titleColor;
    
  Grid major, minor;			/* Axis grid information. */

  double screenScale;
  int screenMin, screenRange;

} Axis;

typedef struct {
  Axis *x, *y;
} Axis2d;

#endif /* _BLT_GR_AXIS_H */
