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
#include "bltGraph.h"
};

#include "bltGrMarker.h"

using namespace Blt;

Marker::Marker(Graph* gPtr, const char* nPtr, Tcl_HashEntry* hPtr)
{
  classId_ =CID_NONE;
  name_ = dupstr(nPtr);
  className_ =NULL;
  optionTable_ =NULL;
  ops_ =NULL;
  graphPtr_ =gPtr;  
  hashPtr_ = hPtr;
  clipped_ =0;

  link =NULL;
  flags =0;
}

Marker::~Marker()
{
  // If the marker to be deleted is currently displayed below the
  // elements, then backing store needs to be repaired.
  if (((MarkerOptions*)ops_)->drawUnder)
    graphPtr_->flags |= CACHE_DIRTY;

  Blt_DeleteBindings(graphPtr_->bindTable, this);

  if (className_)
    delete [] className_;

  if (name_)
    delete [] name_;

  if (hashPtr_)
    Tcl_DeleteHashEntry(hashPtr_);

  if (link)
    Blt_Chain_DeleteLink(graphPtr_->markers.displayList, link);

  Tk_FreeConfigOptions((char*)ops_, optionTable_, graphPtr_->tkwin);

  if (ops_)
    free(ops_);
}

double Marker::HMap(Axis *axisPtr, double x)
{
  if (x == DBL_MAX)
    x = 1.0;
  else if (x == -DBL_MAX)
    x = 0.0;
  else {
    if (axisPtr->logScale) {
      if (x > 0.0)
	x = log10(x);
      else if (x < 0.0)
	x = 0.0;
    }
    x = (x - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
  }
  if (axisPtr->descending)
    x = 1.0 - x;

  // Horizontal transformation
  return (x * axisPtr->screenRange + axisPtr->screenMin);
}

double Marker::VMap(Axis *axisPtr, double y)
{
  if (y == DBL_MAX)
    y = 1.0;
  else if (y == -DBL_MAX)
    y = 0.0;
  else {
    if (axisPtr->logScale) {
      if (y > 0.0)
	y = log10(y);
      else if (y < 0.0)
	y = 0.0;
    }
    y = (y - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
  }
  if (axisPtr->descending)
    y = 1.0 - y;

  // Vertical transformation
  return (((1.0 - y) * axisPtr->screenRange) + axisPtr->screenMin);
}

Point2d Marker::mapPoint(Point2d* pointPtr, Axis2d* axesPtr)
{
  Point2d result;
  if (graphPtr_->inverted) {
    result.x = HMap(axesPtr->y, pointPtr->y);
    result.y = VMap(axesPtr->x, pointPtr->x);
  }
  else {
    result.x = HMap(axesPtr->x, pointPtr->x);
    result.y = VMap(axesPtr->y, pointPtr->y);
  }

  return result;
}

int Marker::boxesDontOverlap(Graph* graphPtr_, Region2d *extsPtr)
{
  return (((double)graphPtr_->right < extsPtr->left) ||
	  ((double)graphPtr_->bottom < extsPtr->top) ||
	  (extsPtr->right < (double)graphPtr_->left) ||
	  (extsPtr->bottom < (double)graphPtr_->top));
}
