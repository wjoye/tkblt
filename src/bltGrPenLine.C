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

#include "bltGrPenLine.h"

typedef struct {
  const char* name;
  unsigned int minChars;
  SymbolType type;
} GraphSymbolType;

static GraphSymbolType graphSymbols[] = {
  { "arrow",	  1, SYMBOL_ARROW,	},
  { "circle",	  2, SYMBOL_CIRCLE,	},
  { "cross",	  2, SYMBOL_CROSS,	}, 
  { "diamond",  1, SYMBOL_DIAMOND,	}, 
  { "none",	  1, SYMBOL_NONE,	}, 
  { "plus",	  1, SYMBOL_PLUS,	}, 
  { "scross",	  2, SYMBOL_SCROSS,	}, 
  { "splus",	  2, SYMBOL_SPLUS,	}, 
  { "square",	  2, SYMBOL_SQUARE,	}, 
  { "triangle", 1, SYMBOL_TRIANGLE,	}, 
  { NULL,       0, SYMBOL_NONE,		}, 
};

// Defs

static void DestroySymbol(Display *display, Symbol *symbolPtr);

static Tk_CustomOptionSetProc SymbolSetProc;
static Tk_CustomOptionGetProc SymbolGetProc;
Tk_ObjCustomOption symbolObjOption =
  {
    "symbol", SymbolSetProc, SymbolGetProc, NULL, NULL, NULL
  };

static int SymbolSetProc(ClientData clientData, Tcl_Interp* interp,
			 Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			 int offset, char* save, int flags)
{
  Symbol* symbolPtr = (Symbol*)(widgRec + offset);
  const char* string;

  // string
  {
    int length;
    string = Tcl_GetStringFromObj(*objPtr, &length);
    if (length == 0) {
      DestroySymbol(Tk_Display(tkwin), symbolPtr);
      symbolPtr->type = SYMBOL_NONE;
      return TCL_OK;
    }
    char c = string[0];

    for (GraphSymbolType* p = graphSymbols; p->name; p++) {
      if (length < p->minChars) {
	continue;
      }
      if ((c == p->name[0]) && (strncmp(string, p->name, length) == 0)) {
	DestroySymbol(Tk_Display(tkwin), symbolPtr);
	symbolPtr->type = p->type;
	return TCL_OK;
      }
    }
  }

  // bitmap
  {
    Tcl_Obj **objv;
    int objc;
    if ((Tcl_ListObjGetElements(NULL, *objPtr, &objc, &objv) != TCL_OK) || 
	(objc > 2)) {
      goto error;
    }
    Pixmap bitmap = None;
    Pixmap mask = None;
    if (objc > 0) {
      bitmap = Tk_AllocBitmapFromObj((Tcl_Interp*)NULL, tkwin, objv[0]);
      if (!bitmap)
	goto error;

    }
    if (objc > 1) {
      mask = Tk_AllocBitmapFromObj((Tcl_Interp*)NULL, tkwin, objv[1]);
      if (!mask)
	goto error;
    }
    DestroySymbol(Tk_Display(tkwin), symbolPtr);
    symbolPtr->bitmap = bitmap;
    symbolPtr->mask = mask;
    symbolPtr->type = SYMBOL_BITMAP;

    return TCL_OK;
  }
 error:
  Tcl_AppendResult(interp, "bad symbol \"", string, 
		   "\": should be \"none\", \"circle\", \"square\", \"diamond\", "
		   "\"plus\", \"cross\", \"splus\", \"scross\", \"triangle\", "
		   "\"arrow\" or the name of a bitmap", NULL);
  return TCL_ERROR;
}

static Tcl_Obj* SymbolGetProc(ClientData clientData, Tk_Window tkwin, 
			      char* widgRec, int offset)
{
  Symbol* symbolPtr = (Symbol*)(widgRec + offset);

  Tcl_Obj* ll[2];
  if (symbolPtr->type == SYMBOL_BITMAP) {
    ll[0] = 
      Tcl_NewStringObj(Tk_NameOfBitmap(Tk_Display(tkwin),symbolPtr->bitmap),-1);
    ll[1] = symbolPtr->mask ? 
      Tcl_NewStringObj(Tk_NameOfBitmap(Tk_Display(tkwin),symbolPtr->mask),-1) :
      Tcl_NewStringObj("", -1);
    return Tcl_NewListObj(2, ll);
  }
  else {
    for (GraphSymbolType* p = graphSymbols; p->name; p++) {
      if (p->type == symbolPtr->type)
	return Tcl_NewStringObj(p->name, -1);
    }
    return Tcl_NewStringObj("?unknown symbol type?", -1);
  }
}

static void DestroySymbol(Display* display, Symbol* symbolPtr)
{
  if (symbolPtr->bitmap != None) {
    Tk_FreeBitmap(display, symbolPtr->bitmap);
    symbolPtr->bitmap = None;
  }
  if (symbolPtr->mask != None) {
    Tk_FreeBitmap(display, symbolPtr->mask);
    symbolPtr->mask = None;
  }

  symbolPtr->type = SYMBOL_NONE;
}

static Tk_OptionSpec linePenOptionSpecs[] = {
  {TK_OPTION_COLOR, "-color", "color", "Color", 
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LinePenOptions, traceColor), 
   0, NULL, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes", 
   NULL, -1, Tk_Offset(LinePenOptions, traceDashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(LinePenOptions, errorBarColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarwidth", "errorBarWidth", "ErrorBarWidth",
   "1", -1, Tk_Offset(LinePenOptions, errorBarLineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(LinePenOptions, errorBarCapWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill", 
   NULL, -1, Tk_Offset(LinePenOptions, symbol.fillColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(LinePenOptions, traceWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-offdash", "offDash", "OffDash", 
   NULL, -1, Tk_Offset(LinePenOptions, traceOffColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline", 
   NULL, -1, Tk_Offset(LinePenOptions, symbol.outlineColor), 
   TK_OPTION_NULL_OK, NULL,0},
  {TK_OPTION_PIXELS, "-outlinewidth", "outlineWidth", "OutlineWidth",
   "1", -1, Tk_Offset(LinePenOptions, symbol.outlineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-pixels", "pixels", "Pixels", 
   "0.1i", -1, Tk_Offset(LinePenOptions, symbol.size), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(LinePenOptions, errorBarShow), 0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(LinePenOptions, valueShow), 0, &fillObjOption, 0},
  {TK_OPTION_CUSTOM, "-symbol", "symbol", "Symbol",
   "none", -1, Tk_Offset(LinePenOptions, symbol), 0, &symbolObjOption, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(LinePenOptions, valueStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LinePenOptions, valueStyle.color), 
   0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(LinePenOptions, valueStyle.font), 0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(LinePenOptions, valueFormat), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(LinePenOptions, valueStyle.angle), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

LinePen::LinePen(Graph* graphPtr, const char* name, Tcl_HashEntry* hPtr)
  : Pen(graphPtr, name, hPtr)
{
  optionTable_ = Tk_CreateOptionTable(graphPtr_->interp, linePenOptionSpecs);
  ops_ = calloc(1, sizeof(LinePenOptions));
  manageOptions_ =1;

  traceGC_ =NULL;
  errorBarGC_ =NULL;

  LinePenOptions* ops = (LinePenOptions*)ops_;
  Blt_Ts_InitStyle(ops->valueStyle);
  ops->symbol.bitmap = None;
  ops->symbol.mask = None;
  ops->symbol.type = SYMBOL_NONE;
}

LinePen::LinePen(Graph* graphPtr, const char* name, void* options)
  : Pen(graphPtr, name, NULL)
{
  optionTable_ = Tk_CreateOptionTable(graphPtr_->interp, linePenOptionSpecs);
  ops_ = options;
  manageOptions_ =0;

  traceGC_ =NULL;
  errorBarGC_ =NULL;

  LinePenOptions* ops = (LinePenOptions*)ops_;
  Blt_Ts_InitStyle(ops->valueStyle);
  ops->symbol.bitmap = None;
  ops->symbol.mask = None;
  ops->symbol.type = SYMBOL_NONE;
}

LinePen::~LinePen()
{
  LinePenOptions* ops = (LinePenOptions*)ops_;
  
  if (errorBarGC_)
    Tk_FreeGC(graphPtr_->display, errorBarGC_);

  if (traceGC_)
    Blt_FreePrivateGC(graphPtr_->display, traceGC_);

  if (ops->symbol.outlineGC)
    Tk_FreeGC(graphPtr_->display, ops->symbol.outlineGC);

  if (ops->symbol.fillGC)
    Tk_FreeGC(graphPtr_->display, ops->symbol.fillGC);

  if (ops->symbol.bitmap != None) {
    Tk_FreeBitmap(graphPtr_->display, ops->symbol.bitmap);
    ops->symbol.bitmap = None;
  }

  if (ops->symbol.mask != None) {
    Tk_FreeBitmap(graphPtr_->display, ops->symbol.mask);
    ops->symbol.mask = None;
  }
}

int LinePen::configure()
{
  LinePenOptions* ops = (LinePenOptions*)ops_;

  // symbol outline
  {
    unsigned long gcMask = (GCLineWidth | GCForeground);
    XColor* colorPtr = ops->symbol.outlineColor;
    if (!colorPtr)
      colorPtr = ops->traceColor;
    XGCValues gcValues;
    gcValues.foreground = colorPtr->pixel;
    if (ops->symbol.type == SYMBOL_BITMAP) {
      colorPtr = ops->symbol.fillColor;
      if (!colorPtr)
	colorPtr = ops->traceColor;

      if (colorPtr) {
	gcValues.background = colorPtr->pixel;
	gcMask |= GCBackground;
	if (ops->symbol.mask != None) {
	  gcValues.clip_mask = ops->symbol.mask;
	  gcMask |= GCClipMask;
	}
      }
      else {
	gcValues.clip_mask = ops->symbol.bitmap;
	gcMask |= GCClipMask;
      }
    }
    gcValues.line_width = LineWidth(ops->symbol.outlineWidth);
    GC newGC = Tk_GetGC(graphPtr_->tkwin, gcMask, &gcValues);
    if (ops->symbol.outlineGC)
      Tk_FreeGC(graphPtr_->display, ops->symbol.outlineGC);
    ops->symbol.outlineGC = newGC;
  }

  // symbol fill
  {
    unsigned long gcMask = (GCLineWidth | GCForeground);
    XColor* colorPtr = ops->symbol.fillColor;
    if (!colorPtr)
      colorPtr = ops->traceColor;
    GC newGC = NULL;
    XGCValues gcValues;
    if (colorPtr) {
      gcValues.foreground = colorPtr->pixel;
      newGC = Tk_GetGC(graphPtr_->tkwin, gcMask, &gcValues);
    }
    if (ops->symbol.fillGC)
      Tk_FreeGC(graphPtr_->display, ops->symbol.fillGC);
    ops->symbol.fillGC = newGC;
  }

  // trace
  {
    unsigned long gcMask = 
      (GCLineWidth | GCForeground | GCLineStyle | GCCapStyle | GCJoinStyle);
    XGCValues gcValues;
    gcValues.cap_style = CapButt;
    gcValues.join_style = JoinRound;
    gcValues.line_style = LineSolid;
    gcValues.line_width = LineWidth(ops->traceWidth);

    gcValues.foreground = ops->traceColor->pixel;
    XColor* colorPtr = ops->traceOffColor;
    if (colorPtr) {
      gcMask |= GCBackground;
      gcValues.background = colorPtr->pixel;
    }
    if (LineIsDashed(ops->traceDashes)) {
      gcValues.line_width = ops->traceWidth;
      gcValues.line_style = !colorPtr ? LineOnOffDash : LineDoubleDash;
    }
    GC newGC = Blt_GetPrivateGC(graphPtr_->tkwin, gcMask, &gcValues);
    if (traceGC_)
      Blt_FreePrivateGC(graphPtr_->display, traceGC_);

    if (LineIsDashed(ops->traceDashes)) {
      ops->traceDashes.offset = ops->traceDashes.values[0] / 2;
      Blt_SetDashes(graphPtr_->display, newGC, &ops->traceDashes);
    }
    traceGC_ = newGC;
  }

  // errorbar
  {
    unsigned long gcMask = (GCLineWidth | GCForeground);
    XColor* colorPtr = ops->errorBarColor;
    if (!colorPtr)
      colorPtr = ops->traceColor;
    XGCValues gcValues;
    gcValues.line_width = LineWidth(ops->errorBarLineWidth);
    gcValues.foreground = colorPtr->pixel;
    GC newGC = Tk_GetGC(graphPtr_->tkwin, gcMask, &gcValues);
    if (errorBarGC_) {
      Tk_FreeGC(graphPtr_->display, errorBarGC_);
    }
    errorBarGC_ = newGC;
  }

  return TCL_OK;
}


