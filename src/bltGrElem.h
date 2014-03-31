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

#ifndef __bltgrelem_h__
#define __bltgrelem_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

extern "C" {
#include <bltVector.h>
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

typedef struct _Element Element;

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

typedef void (ElementDrawProc) (Graph *graphPtr, Drawable drawable, 
				Element* elemPtr);
typedef void (ElementToPostScriptProc) (Graph *graphPtr, Blt_Ps ps, 
					Element* elemPtr);
typedef void (ElementDestroyProc) (Graph *graphPtr, Element* elemPtr);
typedef int (ElementConfigProc) (Graph *graphPtr, Element* elemPtr);
typedef void (ElementMapProc) (Graph *graphPtr, Element* elemPtr);
typedef void (ElementExtentsProc) (Element* elemPtr, Region2d *extsPtr);
typedef void (ElementClosestProc) (Graph *graphPtr, Element* elemPtr);
typedef void (ElementDrawSymbolProc) (Graph *graphPtr, Drawable drawable, 
				      Element* elemPtr, int x, int y, 
				      int symbolSize);
typedef void (ElementSymbolToPostScriptProc) (Graph *graphPtr, Blt_Ps ps, 
					      Element* elemPtr, double x, 
					      double y, int symSize);

typedef struct {
  ElementClosestProc *closestProc;
  ElementConfigProc *configProc;
  ElementDestroyProc *destroyProc;
  ElementDrawProc *drawActiveProc;
  ElementDrawProc *drawNormalProc;
  ElementDrawSymbolProc *drawSymbolProc;
  ElementExtentsProc *extentsProc;
  ElementToPostScriptProc *printActiveProc;
  ElementToPostScriptProc *printNormalProc;
  ElementSymbolToPostScriptProc *printSymbolProc;
  ElementMapProc *mapProc;
} ElementProcs;

typedef struct _Element {
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
  Pen *activePenPtr;
  Pen *normalPenPtr;
  Pen *builtinPenPtr;
  Blt_Chain stylePalette;
  int scaleSymbols;
  double xRange;
  double yRange;
  int state;
  Blt_ChainLink link;
} Element;

extern double Blt_FindElemValuesMinimum(ElemValues *vecPtr, double minLimit);
extern void Blt_FreeStylePalette (Blt_Chain stylePalette);
extern PenStyle** Blt_StyleMap (Element* elemPtr);

#endif
