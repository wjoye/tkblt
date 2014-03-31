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

#ifndef __BltGrElemBar_h__
#define __BltGrElemBar_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "bltGrElem.h"
#include "bltGrPenBar.h"

typedef struct {
  GraphObj obj;
  unsigned int flags;		
  int hide;
  Tcl_HashEntry *hashPtr;

  // Fields specific to elements
  const char *label;
  unsigned short row;
  unsigned short col;
  int legendRelief;
  Axis2d axes;
  ElemCoords coords;
  ElemValues* w;
  int *activeIndices;
  int nActiveIndices;
  ElementProcs *procsPtr;
  Tk_OptionTable optionTable;
  BarPen *activePenPtr;
  BarPen *normalPenPtr;
  BarPen *builtinPenPtr;
  Blt_Chain stylePalette;

  // Symbol scaling
  int scaleSymbols;
  double xRange;
  double yRange;
  int state;
  Blt_ChainLink link;

  // Fields specific to the barchart element
  double barWidth;
  const char *groupName;
  int *barToData;
  XRectangle *bars;
  int *activeToData;
  XRectangle *activeRects;
  int nBars;
  int nActive;
  int xPad;
  ElemValues* xError;
  ElemValues* yError;
  ElemValues* xHigh;
  ElemValues* xLow;
  ElemValues* yHigh;
  ElemValues* yLow;
  BarPen builtinPen;
  GraphSegments xeb;
  GraphSegments yeb;
  int errorBarCapWidth;
} BarElement;

#endif
