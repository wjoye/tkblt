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

#ifndef __BltGrElem_h__
#define __BltGrElem_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

extern "C" {
#include "bltGraph.h"
#include "bltVector.h"
};

#include "bltGrPen.h"

#define SHOW_NONE	0
#define SHOW_X		1
#define SHOW_Y		2
#define SHOW_BOTH	3

#define	LABEL_ACTIVE 	(1<<9)	/* Non-zero indicates that the element's entry
				 * in the legend should be drawn in its active
				 * foreground and background colors. */

#define NUMBEROFPOINTS(e) MIN( \
			      (e)->coords.x ? (e)->coords.x->nValues : 0, \
			      (e)->coords.y ? (e)->coords.y->nValues : 0 \
			       )

#define NORMALPEN(e) ((((e)->normalPenPtr == NULL) ? (e)->builtinPenPtr : (e)->normalPenPtr))

class Element;

typedef struct {
  Blt_VectorId vector;
} VectorDataSource;

typedef struct {
  int type;
  Element* elemPtr;
  VectorDataSource vectorSource;
  double *values;
  int nValues;
  int arraySize;
  double min, max;
} ElemValues;

typedef struct {
  ElemValues* x;
  ElemValues* y;
} ElemCoords;

typedef struct {
  double min;
  double max;
  double range;
} Weight;

typedef struct {
  Weight weight;
  Pen* penPtr;
} PenStyle;

typedef struct {
  Element* elemPtr;
  const char* label;
  const char** tags;
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
  Pen* builtinPenPtr;
  Pen* activePenPtr;
  Pen* normalPenPtr;
  PenOptions builtinPen;
} ElementOptions;

class Element {
 public:
  Graph* graphPtr_;
  ClassId classId_;
  const char* name_;
  Tk_OptionTable optionTable_;
  void* ops_;
  Tcl_HashEntry* hashPtr_;
  int hide_;

  unsigned short row_;
  unsigned short col_;
  int *activeIndices_;
  int nActiveIndices_;
  double xRange_;
  double yRange_;

  Blt_ChainLink link;
  unsigned int flags;		

 protected:
  double FindElemValuesMinimum(ElemValues*, double);
  PenStyle** StyleMap();

 public:
  Element(Graph*, const char*, Tcl_HashEntry*);
  virtual ~Element();

  virtual int configure() =0;
  virtual void map() =0;
  virtual void extents(Region2d*) =0;
  virtual void drawActive(Drawable) =0;
  virtual void drawNormal(Drawable) =0;
  virtual void drawSymbol(Drawable, int, int, int) =0;
  virtual void closest() =0;
  virtual void printActive(Blt_Ps) =0;
  virtual void printNormal(Blt_Ps) =0;
  virtual void printSymbol(Blt_Ps, double, double, int) =0;

  ClassId classId() {return classId_;}
  const char* name() {return name_;}
  virtual const char* className() =0;
  Tk_OptionTable optionTable() {return optionTable();}
  void* ops() {return ops_;}
};

extern void Blt_FreeStylePalette (Blt_Chain stylePalette);
extern void Blt_InitBarSetTable(Graph* graphPtr);
extern void Blt_ComputeBarStacks(Graph* graphPtr);
extern void Blt_ResetBarGroups(Graph* graphPtr);
extern void Blt_DestroyBarSets(Graph* graphPtr);

#endif
