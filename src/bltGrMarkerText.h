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

#ifndef _BLT_GR_MARKER_TEXT_H
#define _BLT_GR_MARKER_TEXT_H

#include "bltGrMarker.h"

typedef struct {
  GraphObj obj;			/* Must be first field in marker. */
  MarkerClass *classPtr;

  Tk_OptionTable optionTable;	/* Configuration specifications */
  Tcl_HashEntry *hashPtr;
  Blt_ChainLink link;
  const char* elemName;		/* Element associated with marker. Let's
				 * you link a marker to an element. The
				 * marker is drawn only if the element
				 * is also visible. */
  Axis2d axes;
  Point2d *worldPts;			/* Coordinate array to position
					 * marker. */
  int nWorldPts;			/* # of points in above array */
  int drawUnder;			/* If non-zero, draw the marker
					 * underneath any elements. This can be
					 * a performance penalty because the
					 * graph must be redraw entirely each
					 * time the marker is redrawn. */
  int clipped;			/* Indicates if the marker is totally
				 * clipped by the plotting area. */
  int hide;
  unsigned int flags;		


  int xOffset, yOffset;		/* Pixel offset from graph position */
  int state;

  /* Fields specific to text markers. */
  const char* string;			/* Text string to be display.  The
					 * string make contain newlines. */
  Tk_Anchor anchor;			/* Indicates how to translate the given
					 * marker position. */
  Point2d anchorPt;			/* Translated anchor point. */
  int width, height;			/* Dimension of bounding box. */
  TextStyle style;			/* Text attributes (font, fg, anchor,
					 * etc) */
  Point2d outline[5];
  XColor* fillColor;
  GC fillGC;
} TextMarker;

#endif
