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

static Tk_OptionSpec linePenOptionSpecs[] = {
  {TK_OPTION_COLOR, "-color", "color", "Color", 
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LinePen, traceColor), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes", 
   NULL, -1, Tk_Offset(LinePen, traceDashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(LinePen, errorBarColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarwidth", "errorBarWidth", "ErrorBarWidth",
   "1", -1, Tk_Offset(LinePen, errorBarLineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(LinePen, errorBarCapWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-fill", "fill", "Fill", 
   NULL, -1, Tk_Offset(LinePen, symbol.fillColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", -1, Tk_Offset(LinePen, traceWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-offdash", "offDash", "OffDash", 
   NULL, -1, Tk_Offset(LinePen, traceOffColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-outline", "outline", "Outline", 
   NULL, -1, Tk_Offset(LinePen, symbol.outlineColor), 
   TK_OPTION_NULL_OK, NULL,0},
  {TK_OPTION_PIXELS, "-outlinewidth", "outlineWidth", "OutlineWidth",
   "1", -1, Tk_Offset(LinePen, symbol.outlineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-pixels", "pixels", "Pixels", 
   "0.1i", -1, Tk_Offset(LinePen, symbol.size), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(LinePen, errorBarShow), 0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(LinePen, valueShow), 0, &fillObjOption, 0},
  {TK_OPTION_CUSTOM, "-symbol", "symbol", "Symbol",
   "none", -1, Tk_Offset(LinePen, symbol), 0, &symbolObjOption, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(LinePen, valueStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LinePen, valueStyle.color), 0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(LinePen, valueStyle.font), 0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(LinePen, valueFormat), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(LinePen, valueStyle.angle), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

Pen* CreateLinePen(Graph* graphPtr, const char* penName)
{
  LinePen* penPtr = (LinePen*)calloc(1, sizeof(LinePen));
  penPtr->ops = (LinePenOptions*)calloc(1, sizeof(LinePenOptions));
  penPtr->manageOptions =1;

  InitLinePen(graphPtr, penPtr, penName);
  return (Pen*)penPtr;
}

void InitLinePen(Graph* graphPtr, LinePen* penPtr, const char* penName)
{
  penPtr->configProc = ConfigureLinePenProc;
  penPtr->destroyProc = DestroyLinePenProc;

  penPtr->classId = CID_ELEM_LINE;
  penPtr->name = Blt_Strdup(penName);

  Blt_Ts_InitStyle(penPtr->valueStyle);
  penPtr->symbol.bitmap = None;
  penPtr->symbol.mask = None;
  penPtr->symbol.type = SYMBOL_NONE;

  penPtr->optionTable = 
    Tk_CreateOptionTable(graphPtr->interp, linePenOptionSpecs);
}

int ConfigureLinePenProc(Graph* graphPtr, Pen* basePtr)
{
  LinePen* lpPtr = (LinePen*)basePtr;

  // symbol outline
  unsigned long gcMask = (GCLineWidth | GCForeground);
  XColor* colorPtr = lpPtr->symbol.outlineColor;
  if (!colorPtr)
    colorPtr = lpPtr->traceColor;

  XGCValues gcValues;
  gcValues.foreground = colorPtr->pixel;
  if (lpPtr->symbol.type == SYMBOL_BITMAP) {
    colorPtr = lpPtr->symbol.fillColor;
    if (!colorPtr)
      colorPtr = lpPtr->traceColor;

    if (colorPtr) {
      gcValues.background = colorPtr->pixel;
      gcMask |= GCBackground;
      if (lpPtr->symbol.mask != None) {
	gcValues.clip_mask = lpPtr->symbol.mask;
	gcMask |= GCClipMask;
      }
    }
    else {
      gcValues.clip_mask = lpPtr->symbol.bitmap;
      gcMask |= GCClipMask;
    }
  }
  gcValues.line_width = LineWidth(lpPtr->symbol.outlineWidth);
  GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lpPtr->symbol.outlineGC)
    Tk_FreeGC(graphPtr->display, lpPtr->symbol.outlineGC);
  lpPtr->symbol.outlineGC = newGC;

  // symbol fill
  gcMask = (GCLineWidth | GCForeground);
  colorPtr = lpPtr->symbol.fillColor;
  if (!colorPtr)
    colorPtr = lpPtr->traceColor;

  newGC = NULL;
  if (colorPtr) {
    gcValues.foreground = colorPtr->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (lpPtr->symbol.fillGC)
    Tk_FreeGC(graphPtr->display, lpPtr->symbol.fillGC);
  lpPtr->symbol.fillGC = newGC;

  // trace
  gcMask = (GCLineWidth | GCForeground | GCLineStyle | GCCapStyle |
	    GCJoinStyle);
  gcValues.cap_style = CapButt;
  gcValues.join_style = JoinRound;
  gcValues.line_style = LineSolid;
  gcValues.line_width = LineWidth(lpPtr->traceWidth);

  gcValues.foreground = lpPtr->traceColor->pixel;
  colorPtr = lpPtr->traceOffColor;
  if (colorPtr) {
    gcMask |= GCBackground;
    gcValues.background = colorPtr->pixel;
  }
  if (LineIsDashed(lpPtr->traceDashes)) {
    gcValues.line_width = lpPtr->traceWidth;
    gcValues.line_style = !colorPtr ? LineOnOffDash : LineDoubleDash;
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lpPtr->traceGC)
    Blt_FreePrivateGC(graphPtr->display, lpPtr->traceGC);

  if (LineIsDashed(lpPtr->traceDashes)) {
    lpPtr->traceDashes.offset = lpPtr->traceDashes.values[0] / 2;
    Blt_SetDashes(graphPtr->display, newGC, &lpPtr->traceDashes);
  }
  lpPtr->traceGC = newGC;

  // errorbar
  gcMask = (GCLineWidth | GCForeground);
  colorPtr = lpPtr->errorBarColor;
  if (!colorPtr)
    colorPtr = lpPtr->traceColor;

  gcValues.line_width = LineWidth(lpPtr->errorBarLineWidth);
  gcValues.foreground = colorPtr->pixel;
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lpPtr->errorBarGC) {
    Tk_FreeGC(graphPtr->display, lpPtr->errorBarGC);
  }
  lpPtr->errorBarGC = newGC;

  return TCL_OK;
}

void DestroyLinePenProc(Graph* graphPtr, Pen* basePtr)
{
  LinePen* penPtr = (LinePen*)basePtr;
  
  Blt_Ts_FreeStyle(graphPtr->display, &penPtr->valueStyle);
  if (penPtr->symbol.outlineGC)
    Tk_FreeGC(graphPtr->display, penPtr->symbol.outlineGC);

  if (penPtr->symbol.fillGC)
    Tk_FreeGC(graphPtr->display, penPtr->symbol.fillGC);

  if (penPtr->errorBarGC)
    Tk_FreeGC(graphPtr->display, penPtr->errorBarGC);

  if (penPtr->traceGC)
    Blt_FreePrivateGC(graphPtr->display, penPtr->traceGC);

  if (penPtr->symbol.bitmap != None) {
    Tk_FreeBitmap(graphPtr->display, penPtr->symbol.bitmap);
    penPtr->symbol.bitmap = None;
  }
  if (penPtr->symbol.mask != None) {
    Tk_FreeBitmap(graphPtr->display, penPtr->symbol.mask);
    penPtr->symbol.mask = None;
  }

  Tk_FreeConfigOptions((char*)penPtr, penPtr->optionTable, graphPtr->tkwin);

  if (penPtr->manageOptions)
    if (penPtr->ops)
      free(penPtr->ops);
}

static void DestroySymbol(Display *display, Symbol *symbolPtr)
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


