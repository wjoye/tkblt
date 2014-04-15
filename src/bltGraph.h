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

#ifndef __BltGraph_h__
#define __BltGraph_h__

#include "bltGrMisc.h"
#include "bltBind.h"
#include "bltPs.h"

extern "C" {
#include "bltChain.h"
};

class Graph;
class Crosshairs;
class Axis;
class Element;
class Legend;

typedef struct {
  Axis *x, *y;
} Axis2d;

typedef struct {
  int halo;
  int mode;
  int x;
  int y;
  int along;

  Element* elemPtr;
  Point2d point;
  int index;
  double dist;
} ClosestSearch;

typedef struct {
  int nSegments;
  Axis2d axes;
  float sum;
  int count;
  float lastY;
  size_t index;
} BarGroup;

typedef struct {
  float value;
  Axis2d axes;
} BarSetKey;

typedef enum BarModes {
  BARS_INFRONT, BARS_STACKED, BARS_ALIGNED, BARS_OVERLAP
} BarMode;

typedef struct {
  short int width;
  short int height;
  short int axesOffset;
  short int axesTitleLength;
  short int maxTickWidth;
  short int maxTickHeight;
  unsigned int nAxes;
  Blt_Chain axes;
  const char *varName;
  int reqSize;
  int site;
} Margin;

typedef struct {
  Tcl_HashTable table;
  Blt_Chain displayList;
  Tcl_HashTable tagTable;
} Component;

#define rightMargin	margins[MARGIN_RIGHT]
#define leftMargin	margins[MARGIN_LEFT]
#define topMargin	margins[MARGIN_TOP]
#define bottomMargin	margins[MARGIN_BOTTOM]

class Graph {
 public:
  Tcl_Interp* interp;
  Tk_Window tkwin;
  Display *display;
  Tcl_Command cmdToken;
  Tk_OptionTable optionTable;
  ClassId classId;
  Tk_Cursor cursor;
  int inset;
  int borderWidth;
  int relief;
  unsigned int flags;
  Tk_3DBorder normalBg;
  int highlightWidth;
  XColor* highlightBgColor;
  XColor* highlightColor;
  const char *title;
  short int titleX;
  short int titleY;
  short int titleWidth;
  short int titleHeight;
  TextStyle titleTextStyle;
  const char *takeFocus; // nor used in C code
  Axis *focusPtr;
  int reqWidth;
  int reqHeight;
  int reqPlotWidth;
  int reqPlotHeight;
  int width;
  int height;
  Tcl_HashTable penTable;

  Component elements;
  Component markers;
  Component axes;

  Tcl_HashTable dataTables;
  Blt_BindTable bindTable;
  int nextMarkerId;
  Blt_Chain axisChain[4];
  Margin margins[4];
  PageSetup *pageSetup;
  Legend *legend;
  Crosshairs *crosshairs;

  int halo;
  int inverted;
  int stackAxes;
  GC drawGC;
  int plotBW;
  int plotRelief;
  Tk_3DBorder plotBg;
  double aspect;
  short int left;
  short int right;
  short int top;
  short int bottom;	
  int xPad;
  int vRange;
  int vOffset;
  int yPad;
  int hRange;
  int hOffset;
  float vScale;
  float hScale;
  int doubleBuffer;
  int backingStore;
  Pixmap cache;
  short int cacheWidth;
  short int cacheHeight;
  ClosestSearch search;

  // barchart specific information
  double baseline;
  double barWidth;
  BarMode barMode;
  BarGroup *barGroups;
  int nBarGroups;
  Tcl_HashTable setTable;
  int maxBarSetSize;
};

extern void Blt_MapGraph(Graph* graphPtr);
extern void Blt_ReconfigureGraph(Graph* graphPtr);
extern void Blt_GraphExtents(Graph* graphPtr, Region2d *extsPtr);
extern void Blt_EventuallyRedrawGraph(Graph* graphPtr);

typedef ClientData (MakeTagProc)(Graph* graphPtr, const char *tagName);
extern MakeTagProc Blt_MakeElementTag;
extern MakeTagProc Blt_MakeAxisTag;
extern Blt_BindTagProc Blt_GraphTags;
extern Blt_BindTagProc Blt_AxisTags;

#endif
