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

#ifndef __BltGrElemLine_h__
#define __BltGrElemLine_h__

#include "bltGrElem.h"
#include "bltGrPenLine.h"

typedef enum {
  PEN_INCREASING, PEN_DECREASING, PEN_BOTH_DIRECTIONS
} PenDirection;

typedef struct {
  Point2d *screenPts;
  int nScreenPts;
  int *styleMap;
  int *map;
} MapInfo;

typedef enum {
  PEN_SMOOTH_LINEAR, PEN_SMOOTH_STEP, PEN_SMOOTH_NATURAL,
  PEN_SMOOTH_QUADRATIC, PEN_SMOOTH_CATROM
} Smoothing;

typedef struct {
  Point2d *points;
  int length;
  int *map;
} GraphPoints;

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

typedef struct {
  Element* elemPtr;
  const char* label;
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
  LinePen *builtinPenPtr;
  LinePen *activePenPtr;
  LinePen *normalPenPtr;
  LinePenOptions builtinPen;

  // derived
  XColor* fillFgColor;
  Tk_3DBorder fillBg;
  int reqMaxSymbols;
  double rTolerance;
  int scaleSymbols;
  Smoothing reqSmooth;
  int penDir;
} LineElementOptions;

class LineElement : public Element {
 protected:
  LinePen* builtinPenPtr;
  Smoothing smooth_;
  XColor* fillBgColor_;
  GC fillGC_;
  Point2d *fillPts_;
  int nFillPts_;
  GraphPoints symbolPts_;
  GraphPoints activePts_;
  GraphSegments xeb_;
  GraphSegments yeb_;
  GraphSegments lines_;
  int symbolInterval_;
  int symbolCounter_;
  Blt_Chain traces_;

 protected:
  int ScaleSymbol(int);
  void GetScreenPoints(MapInfo*);
  void ReducePoints(MapInfo*, double);
  void GenerateSteps(MapInfo*);
  void GenerateSpline(MapInfo*);
  void GenerateParametricSpline(MapInfo*);
  void MapSymbols(MapInfo*);
  void MapActiveSymbols();
  void MergePens(LineStyle**);
  int OutCode(Region2d*, Point2d*);
  int ClipSegment(Region2d*, int, int, Point2d*, Point2d*);
  void SaveTrace(int, int, MapInfo*);
  void FreeTraces();
  void MapTraces(MapInfo*);
  void MapFillArea(MapInfo*);
  void ResetLine();
  void MapErrorBars(LineStyle**);
  int ClosestTrace();
  void ClosestPoint(ClosestSearch*);
  void DrawCircles(Display*, Drawable, LinePen*, int, Point2d*, int);
  void DrawSquares(Display*, Drawable, LinePen*, int, Point2d*, int);
  void DrawSymbols(Drawable, LinePen*, int, int, Point2d*);
  void DrawTraces(Drawable, LinePen*);
  void DrawValues(Drawable, LinePen*, int, Point2d*, int*);
  void SetLineAttributes(Blt_Ps, LinePen*);
  void TracesToPostScript(Blt_Ps, LinePen*);
  void ValuesToPostScript(Blt_Ps, LinePen*, int, Point2d*, int*);
  double DistanceToLine(int, int, Point2d*, Point2d*, Point2d*);
  double DistanceToX(int, int, Point2d*, Point2d*, Point2d*);
  double DistanceToY(int, int, Point2d*, Point2d*, Point2d*);
  void GetSymbolPostScriptInfo(Blt_Ps, LinePen*, int);
  void SymbolsToPostScript(Blt_Ps, LinePen*, int, int, Point2d*);

 public:
  LineElement(Graph*, const char*, Tcl_HashEntry*);
  virtual ~LineElement();

  ClassId classId() {return CID_ELEM_LINE;}
  const char* className() {return "LineElement";}
  const char* typeName() {return "line";}

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
