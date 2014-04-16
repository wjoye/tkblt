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

typedef struct {
  double aspect;
  Tk_3DBorder normalBg;
  int borderWidth;
  Margin margins[4];
  int backingStore;
  int doubleBuffer;
  Tk_Cursor cursor;
  TextStyle titleTextStyle;
  int reqHeight;
  XColor* highlightBgColor;
  XColor* highlightColor;
  int highlightWidth;
  int inverted;
  Tk_3DBorder plotBg;
  int plotBW;
  int xPad;
  int yPad;
  int plotRelief;
  int relief;
  ClosestSearch search;
  int stackAxes;
  const char *takeFocus; // nor used in C code
  const char *title;
  int reqWidth;
  int reqPlotWidth;
  int reqPlotHeight;

  // bar graph
  BarMode barMode;
  double barWidth;
  double baseline;
} GraphOptions;

class Graph {
 public:
  int valid_;
  Tcl_Interp* interp;
  Tk_Window tkwin;
  Display *display;
  Tcl_Command cmdToken;
  Tk_OptionTable optionTable_;
  void* ops_;

  ClassId classId;
  unsigned int flags;
  int nextMarkerId;

  Component axes;
  Component elements;
  Component markers;
  Tcl_HashTable dataTables;
  Tcl_HashTable penTable;
  Blt_BindTable bindTable;
  Blt_Chain axisChain[4];

  Legend *legend;
  Crosshairs *crosshairs;
  PageSetup *pageSetup;

  int inset;
  short int titleX;
  short int titleY;
  short int titleWidth;
  short int titleHeight;
  Axis *focusPtr;
  int width;
  int height;
  int halo;
  GC drawGC;
  short int left;
  short int right;
  short int top;
  short int bottom;	
  int vRange;
  int vOffset;
  int hRange;
  int hOffset;
  float vScale;
  float hScale;
  Pixmap cache;
  short int cacheWidth;
  short int cacheHeight;

  // barchart specific information
  BarGroup *barGroups;
  int nBarGroups;
  Tcl_HashTable setTable;
  int maxBarSetSize;

 public:
  Graph(ClientData clientData, Tcl_Interp*interp, 
	int objc, Tcl_Obj* const objv[], ClassId classId);
  virtual ~Graph();
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
