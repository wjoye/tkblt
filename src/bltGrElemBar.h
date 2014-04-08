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

typedef struct {
  Element* elemPtr;
  const char *label;
  char** tags;
  Axis2d axes;
  ElemCoords coords;
  ElemValues* w;
  ElemValues* xError;
  ElemValues* yError;
  ElemValues* xHigh;
  ElemValues* xLow;
  ElemValues* yHigh;
  ElemValues* yLow;
  int hide;
  int legendRelief;
  Blt_Chain stylePalette;
  BarPen *builtinPenPtr;
  BarPen *activePenPtr;
  BarPen *normalPenPtr;
  BarPenOptions builtinPen;

  // derived
  double barWidth;
  const char *groupName;
} BarElementOptions;

class BarElement : public Element {
 protected:
  BarPen* builtinPenPtr;
  int* barToData_;
  XRectangle* bars_;
  int* activeToData_;
  XRectangle* activeRects_;
  int nBars_;
  int nActive_;
  int xPad_;
  GraphSegments xeb_;
  GraphSegments yeb_;

 protected:
  void ResetStylePalette(Blt_Chain);
  void CheckBarStacks(Axis2d*, double*, double*);
  void MergePens(BarStyle**);
  void MapActiveBars();
  void ResetBar();
  void MapErrorBars(BarStyle**);
  void SetBackgroundClipRegion(Tk_Window, Tk_3DBorder, TkRegion);
  void UnsetBackgroundClipRegion(Tk_Window, Tk_3DBorder);
  void DrawBarSegments(Drawable, BarPen*, XRectangle*, int);
  void DrawBarValues(Drawable, BarPen*, XRectangle*, int, int*);
  void SegmentsToPostScript(Blt_Ps, BarPen*, XRectangle*, int);
  void BarValuesToPostScript(Blt_Ps, BarPen*, XRectangle*, int, int*);

 public:
  BarElement(Graph*, const char*, Tcl_HashEntry*);
  virtual ~BarElement();

  ClassId classId() {return CID_ELEM_BAR;}
  const char* className() {return "BarElement";}
  const char* typeName() {return "bar";}

  int configure();
  void map();
  void extents(Region2d*);
  void closest();
  void drawActive(Drawable);
  void drawNormal(Drawable);
  void drawSymbol(Drawable, int, int, int);
  void printActive(Blt_Ps);
  void printNormal(Blt_Ps);
  void printSymbol(Blt_Ps, double, double, int);
};

#endif
