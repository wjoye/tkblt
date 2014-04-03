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

#ifndef __BltGrPenLine_h__
#define __BltGrPenLine_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "bltGrPen.h"

typedef enum {
  SYMBOL_NONE, SYMBOL_SQUARE, SYMBOL_CIRCLE, SYMBOL_DIAMOND, SYMBOL_PLUS,
  SYMBOL_CROSS, SYMBOL_SPLUS, SYMBOL_SCROSS, SYMBOL_TRIANGLE, SYMBOL_ARROW,
  SYMBOL_BITMAP
} SymbolType;

typedef struct {
  SymbolType type;
  int size;
  XColor* outlineColor;
  int outlineWidth;
  GC outlineGC;
  XColor* fillColor;
  GC fillGC;

  // The last two fields are used only for bitmap symbols
  Pixmap bitmap;
  Pixmap mask;
} Symbol;

typedef struct {
  int errorBarShow;
  int errorBarLineWidth;
  int errorBarCapWidth;
  XColor* errorBarColor;
  int valueShow;
  const char* valueFormat;
  TextStyle valueStyle;

  Symbol symbol;
  int traceWidth;
  Blt_Dashes traceDashes;
  XColor* traceColor;
  XColor* traceOffColor;
} LinePenOptions;

class LinePen {
 public:
  const char* name;
  ClassId classId;
  unsigned int flags;
  int refCount;
  Tcl_HashEntry *hashPtr;
  Tk_OptionTable optionTable;
  PenConfigureProc *configProc;
  PenDestroyProc *destroyProc;
  Graph* graphPtr;
  void* ops;
  int manageOptions;

  GC traceGC;
  GC errorBarGC;
};

extern Tk_ObjCustomOption symbolObjOption;

extern Pen* CreateLinePen(Graph* graphPtr, const char* penName);
extern void InitLinePen(Graph* graphPtr, LinePen* penPtr, const char* penName);
extern int ConfigureLinePenProc(Graph* graphPtr, Pen *basePtr);
extern void DestroyLinePenProc(Graph* graphPtr, Pen* basePtr);

#endif
