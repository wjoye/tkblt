/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
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

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

extern "C" {
#include "bltGraph.h"
}

typedef struct {
  const char *name;
  ClassId classId;
  int margin, invertMargin;
} AxisName;

extern AxisName axisNames[];

typedef struct {
  Blt_Dashes dashes;
  int lineWidth;
  XColor* color;
  GC gc;
  Segment2d *segments;
  int nUsed;
  int nAllocated;
} Grid;

typedef struct {
  double min;
  double max;
  double range;
  double scale;
} AxisRange;

typedef struct {
  Point2d anchorPos;
  unsigned int width;
  unsigned int height;
  char string[1];
} TickLabel;

typedef struct {
  int nTicks;
  double values[1];
} Ticks;

typedef struct {
  double initial;
  double step;
  int nSteps;
} TickSweep;

typedef struct {
  const char** tags;
  int checkLimits;
  int exterior;
  int showGrid;
  int showGridMinor;
  int hide;
  int showTicks;

  // Fields specific to axes

  double windowSize;
  const char *formatCmd;
  int descending;
  int labelOffset;
  TextStyle limitsTextStyle;
  const char *limitsFormat;
  int lineWidth;
  int logScale;
  int looseMin;
  int looseMax;
  Ticks* t1UPtr;
  Ticks* t2UPtr;
  double reqMin;
  double reqMax;
  Tcl_Obj *scrollCmdObjPtr;
  int scrollUnits;
  double reqScrollMin;
  double reqScrollMax;
  double shiftBy;
  double reqStep;
  int reqNumMajorTicks;
  int reqNumMinorTicks;
  int tickLength;
  const char *title;
  int titleAlternate;

  // The following fields are specific to logical axes

  XColor* activeFgColor;
  int activeRelief;
  Tk_3DBorder normalBg;
  int borderWidth;
  XColor* tickColor;
  Grid major;
  Grid minor;
  Tk_Justify titleJustify;
  int relief;
  double tickAngle;	
  Tk_Anchor reqTickAnchor;
  Tk_Font tickFont;
  Tk_Font titleFont;
  XColor* titleColor;

} AxisOptions;

typedef struct _Axis {
  GraphObj obj;
  int use;
  unsigned int flags;		
  Tk_OptionTable optionTable;
  void* ops;
  Tcl_HashEntry *hashPtr;

  /* Fields specific to axes. */

  const char *detail;
  int refCount;
  Point2d titlePos;
  unsigned short int titleWidth;
  unsigned short int titleHeight;	
  double min;
  double max;
  double scrollMin;
  double scrollMax;
  AxisRange valueRange;
  AxisRange axisRange;
  double prevMin;
  double prevMax;
  Ticks* t1Ptr;
  Ticks* t2Ptr;
  TickSweep minorSweep;
  TickSweep majorSweep;

  /* The following fields are specific to logical axes */

  int margin;
  Blt_ChainLink link;
  Blt_Chain chain;
  Segment2d *segments;
  int nSegments;
  Blt_Chain tickLabels;
  short int left;
  short int right;
  short int top;
  short int bottom;
  short int width;
  short int height;
  short int maxTickWidth;
  short int maxTickHeight; 
  Tk_Anchor tickAnchor;
  GC tickGC;
  GC activeTickGC;
  double titleAngle;	
  Tk_Anchor titleAnchor;
  Grid major;
  Grid minor;
  double screenScale;
  int screenMin, screenRange;
} Axis;

#endif
