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

#ifndef _BLT_GR_MARKER_LINE_H
#define _BLT_GR_MARKER_LINE_H

#include "bltGrMarker.h"

typedef struct {
  Coords* worldPts;
  const char* elemName;
  Axis2d axes;
  int hide;
  int state;
  int drawUnder;
  int xOffset;
  int yOffset;

  int capStyle;
  Blt_Dashes dashes;
  XColor* fillColor;
  int joinStyle;
  int lineWidth;
  XColor* outlineColor;
  int xorr;
} LineMarkerOptions;

class LineMarker {
 public:
  GraphObj obj;
  MarkerClass *classPtr;
  Tk_OptionTable optionTable;
  Tcl_HashEntry *hashPtr;
  Blt_ChainLink link;
  int clipped;
  unsigned int flags;		

  LineMarkerOptions* ops;

  // Fields specific to line

  GC gc;
  Segment2d *segments;
  int nSegments;
  int xorState;
};

#endif
