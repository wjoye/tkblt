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

extern "C" {
#include "bltInt.h"
#include "bltGraph.h"
#include "bltOp.h"
};

#include "bltGrElemOp.h"
#include "bltGrElem.h"

PenStyle** Blt_StyleMap(Element* elemPtr)
{
  int i;
  int nWeights;		/* Number of weights to be examined.
			 * If there are more data points than
			 * weights, they will default to the
			 * normal pen. */
  Blt_ChainLink link;
  double *w;			/* Weight vector */
  int nPoints;

  nPoints = NUMBEROFPOINTS(elemPtr);
  nWeights = MIN(elemPtr->w ? elemPtr->w->nValues : 0, nPoints);
  w = elemPtr->w ? elemPtr->w->values : NULL;
  link = Blt_Chain_FirstLink(elemPtr->stylePalette);
  PenStyle *stylePtr = (PenStyle*)Blt_Chain_GetValue(link);

  /* 
   * Create a style mapping array (data point index to style), 
   * initialized to the default style.
   */
  PenStyle **dataToStyle = (PenStyle**)malloc(nPoints * sizeof(PenStyle *));
  for (i = 0; i < nPoints; i++)
    dataToStyle[i] = stylePtr;

  for (i = 0; i < nWeights; i++) {
    for (link = Blt_Chain_LastLink(elemPtr->stylePalette); link != NULL; 
	 link = Blt_Chain_PrevLink(link)) {
      stylePtr = (PenStyle*)Blt_Chain_GetValue(link);

      if (stylePtr->weight.range > 0.0) {
	double norm;

	norm = (w[i] - stylePtr->weight.min) / stylePtr->weight.range;
	if (((norm - 1.0) <= DBL_EPSILON) && 
	    (((1.0 - norm) - 1.0) <= DBL_EPSILON)) {
	  dataToStyle[i] = stylePtr;
	  break;		/* Done: found range that matches. */
	}
      }
    }
  }
  return dataToStyle;
}

void Blt_FreeStylePalette(Blt_Chain stylePalette)
{
  // Skip the first slot. It contains the built-in "normal" pen of the element
  Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette);
  if (link) {
    Blt_ChainLink next;
    for (link = Blt_Chain_NextLink(link); link != NULL; link = next) {
      next = Blt_Chain_NextLink(link);
      PenStyle *stylePtr = (PenStyle*)Blt_Chain_GetValue(link);
      Blt_FreePen(stylePtr->penPtr);
      Blt_Chain_DeleteLink(stylePalette, link);
    }
  }
}

double Blt_FindElemValuesMinimum(ElemValues* valuesPtr, double minLimit)
{
  double min = DBL_MAX;
  if (!valuesPtr)
    return min;

  for (int ii=0; ii<valuesPtr->nValues; ii++) {
    double x = valuesPtr->values[ii];
    // What do you do about negative values when using log
    // scale values seems like a grey area. Mirror.
    if (x < 0.0)
      x = -x;
    if ((x > minLimit) && (min > x))
      min = x;
  }
  if (min == DBL_MAX)
    min = minLimit;

  return min;
}
