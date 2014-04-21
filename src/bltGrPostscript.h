/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1991-2004 George A Howlett.
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

#ifndef __BltGrPageSetup_h__
#define __BltGrPageSetup_h__

typedef struct  {
  Tk_OptionTable optionTable;
  int reqWidth, reqHeight;	/* If greater than zero, represents the
				 * requested dimensions of the printed graph */
  int reqPaperWidth;
  int reqPaperHeight;		/* Requested dimensions for the PostScript
				 * page. Can constrain the size of the graph
				 * if the graph (plus padding) is larger than
				 * the size of the page. */
  int xPad, yPad;		/* Requested padding on the exterior of the
				 * graph. This forms the bounding box for
				 * the page. */
  const char *fontVarName;	/* If non-NULL, is the name of a TCL array
				 * variable containing X to output device font
				 * translations */
  int level;			/* PostScript Language level 1-3 */

  int decorations;
  int center;
  int footer;
  int greyscale;
  int landscape;
  unsigned int flags;

  const char **comments;	/* User supplied comments to be added. */

  /* Computed fields */

  short int left, bottom;	/* Bounding box of the plot in the page. */
  short int right, top;

  float scale;		/* Scale of page. Set if "-maxpect" option
			 * is set, otherwise 1.0. */

  int paperHeight;
  int paperWidth;
    
} PageSetup;

#endif
