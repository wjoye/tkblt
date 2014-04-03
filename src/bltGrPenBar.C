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

#include "bltGrPenBar.h"

static Tk_OptionSpec barPenOptionSpecs[] = {
  {TK_OPTION_BORDER, "-background", "background", "Background",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPenOptions, fill), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(BarPenOptions, borderWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(BarPenOptions, errorBarColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarwidth", "errorBarWidth","ErrorBarWidth",
   "1", -1, Tk_Offset(BarPenOptions, errorBarLineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(BarPenOptions, errorBarCapWidth), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPenOptions, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "raised", -1, Tk_Offset(BarPenOptions, relief), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(BarPenOptions, errorBarShow), 0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(BarPenOptions, valueShow), 0, &fillObjOption, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple", 
   NULL, -1, Tk_Offset(BarPenOptions, stipple), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(BarPenOptions, valueStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPenOptions, valueStyle.color),
   0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(BarPenOptions, valueStyle.font), 0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(BarPenOptions, valueFormat), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(BarPenOptions, valueStyle.angle), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

Pen* CreateBarPen(Graph* graphPtr, const char *penName)
{
  BarPen* penPtr = (BarPen*)calloc(1, sizeof(BarPen));
  penPtr->ops = (BarPenOptions*)calloc(1, sizeof(BarPenOptions));
  penPtr->manageOptions =1;

  InitBarPen(graphPtr, penPtr, penName);
  return (Pen*)penPtr;
}

void InitBarPen(Graph* graphPtr, BarPen* penPtr, const char* penName)
{
  BarPenOptions* ops = (BarPenOptions*)penPtr->ops;

  penPtr->configProc = ConfigureBarPenProc;
  penPtr->destroyProc = DestroyBarPenProc;

  penPtr->classId = CID_ELEM_BAR;
  penPtr->name = Blt_Strdup(penName);

  Blt_Ts_InitStyle(ops->valueStyle);

  penPtr->optionTable = 
    Tk_CreateOptionTable(graphPtr->interp, barPenOptionSpecs);
}

int ConfigureBarPenProc(Graph* graphPtr, Pen *penPtr)
{
  BarPen* bpPtr = (BarPen*)penPtr;
  BarPenOptions* ops = (BarPenOptions*)bpPtr->ops;

  // outlineGC
  {
    unsigned long gcMask = GCForeground | GCLineWidth;
    XGCValues gcValues;
    gcValues.line_width = LineWidth(ops->errorBarLineWidth);
    if (ops->outlineColor)
      gcValues.foreground = ops->outlineColor->pixel;
    else if (ops->fill)
      gcValues.foreground = Tk_3DBorderColor(ops->fill)->pixel;
    GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (bpPtr->outlineGC)
      Tk_FreeGC(graphPtr->display, bpPtr->outlineGC);
    bpPtr->outlineGC = newGC;
  }

  // fillGC
  {
    GC newGC = NULL;
    if (ops->stipple != None) {
      // Handle old-style -stipple specially
      unsigned long gcMask = 
	GCForeground | GCBackground | GCFillStyle | GCStipple;
      XGCValues gcValues;
      int screenNum = Tk_ScreenNumber(graphPtr->tkwin);
      gcValues.foreground = BlackPixel(graphPtr->display, screenNum);
      gcValues.background = WhitePixel(graphPtr->display, screenNum);
      if (ops->fill)
	gcValues.foreground = Tk_3DBorderColor(ops->fill)->pixel;
      else if (ops->outlineColor)
	gcValues.foreground = ops->outlineColor->pixel;
      gcValues.stipple = ops->stipple;
      gcValues.fill_style = FillStippled;
      newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    }
    if (bpPtr->fillGC)
      Tk_FreeGC(graphPtr->display, bpPtr->fillGC);
    bpPtr->fillGC = newGC;
  }

  // errorBarGC
  {
    unsigned long gcMask = GCForeground | GCLineWidth;
    XColor* colorPtr = ops->errorBarColor;
    if (!colorPtr)
      colorPtr = ops->outlineColor;
    XGCValues gcValues;
    gcValues.foreground = colorPtr->pixel;
    gcValues.line_width = LineWidth(ops->errorBarLineWidth);
    GC newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (bpPtr->errorBarGC)
      Tk_FreeGC(graphPtr->display, bpPtr->errorBarGC);
    bpPtr->errorBarGC = newGC;
  }

  return TCL_OK;
}

void DestroyBarPenProc(Graph* graphPtr, Pen* penPtr)
{
  BarPen* bpPtr = (BarPen*)penPtr;
  BarPenOptions* ops = (BarPenOptions*)bpPtr->ops;

  Blt_Ts_FreeStyle(graphPtr->display, &ops->valueStyle);
  if (bpPtr->outlineGC)
    Tk_FreeGC(graphPtr->display, bpPtr->outlineGC);
  if (bpPtr->fillGC)
    Tk_FreeGC(graphPtr->display, bpPtr->fillGC);
  if (bpPtr->errorBarGC)
    Tk_FreeGC(graphPtr->display, bpPtr->errorBarGC);

  Tk_FreeConfigOptions((char*)bpPtr->ops, bpPtr->optionTable, graphPtr->tkwin);

  if (bpPtr->manageOptions)
    if (bpPtr->ops)
      free(bpPtr->ops);
}

