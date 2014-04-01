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
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPen, fill), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(BarPen, borderWidth), 0, NULL, 0},
  {TK_OPTION_COLOR, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
   NULL, -1, Tk_Offset(BarPen, errorBarColor), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarwidth", "errorBarWidth","ErrorBarWidth",
   "1", -1, Tk_Offset(BarPen, errorBarLineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-errorbarcap", "errorBarCap", "ErrorBarCap", 
   "2", -1, Tk_Offset(BarPen, errorBarCapWidth), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_SYNONYM, "-fill", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPen, outlineColor), 
   TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_SYNONYM, "-outline", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
   "raised", -1, Tk_Offset(BarPen, relief), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-showerrorbars", "showErrorBars", "ShowErrorBars",
   "both", -1, Tk_Offset(BarPen, errorBarShow), 0, &fillObjOption, 0},
  {TK_OPTION_STRING_TABLE, "-showvalues", "showValues", "ShowValues",
   "none", -1, Tk_Offset(BarPen, valueShow), 0, &fillObjOption, 0},
  {TK_OPTION_BITMAP, "-stipple", "stipple", "Stipple", 
   NULL, -1, Tk_Offset(BarPen, stipple), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
   "s", -1, Tk_Offset(BarPen, valueStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-valuecolor", "valueColor", "ValueColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(BarPen, valueStyle.color), 0, NULL, 0},
  {TK_OPTION_FONT, "-valuefont", "valueFont", "ValueFont",
   STD_FONT_SMALL, -1, Tk_Offset(BarPen, valueStyle.font), 0, NULL, 0},
  {TK_OPTION_STRING, "-valueformat", "valueFormat", "ValueFormat",
   "%g", -1, Tk_Offset(BarPen, valueFormat), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_DOUBLE, "-valuerotate", "valueRotate", "ValueRotate",
   "0", -1, Tk_Offset(BarPen, valueStyle.angle), 0, NULL, 0},
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
  penPtr->configProc = ConfigureBarPenProc;
  penPtr->destroyProc = DestroyBarPenProc;

  penPtr->classId = CID_ELEM_BAR;
  penPtr->name = Blt_Strdup(penName);

  Blt_Ts_InitStyle(penPtr->valueStyle);

  penPtr->optionTable = 
    Tk_CreateOptionTable(graphPtr->interp, barPenOptionSpecs);
}

int ConfigureBarPenProc(Graph* graphPtr, Pen *basePtr)
{
  BarPen* penPtr = (BarPen*)basePtr;
  int screenNum = Tk_ScreenNumber(graphPtr->tkwin);
  XGCValues gcValues;
  unsigned long gcMask;
  GC newGC;

  // outlineGC
  gcMask = GCForeground | GCLineWidth;
  gcValues.line_width = LineWidth(penPtr->errorBarLineWidth);

  if (penPtr->outlineColor)
    gcValues.foreground = penPtr->outlineColor->pixel;
  else if (penPtr->fill)
    gcValues.foreground = Tk_3DBorderColor(penPtr->fill)->pixel;

  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (penPtr->outlineGC)
    Tk_FreeGC(graphPtr->display, penPtr->outlineGC);
  penPtr->outlineGC = newGC;

  newGC = NULL;
  if (penPtr->stipple != None) {
    // Handle old-style -stipple specially
    gcMask = GCForeground | GCBackground | GCFillStyle | GCStipple;
    gcValues.foreground = BlackPixel(graphPtr->display, screenNum);
    gcValues.background = WhitePixel(graphPtr->display, screenNum);
    if (penPtr->fill)
      gcValues.foreground = Tk_3DBorderColor(penPtr->fill)->pixel;
    else if (penPtr->outlineColor)
      gcValues.foreground = penPtr->outlineColor->pixel;

    gcValues.stipple = penPtr->stipple;
    gcValues.fill_style = FillStippled;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (penPtr->fillGC)
    Tk_FreeGC(graphPtr->display, penPtr->fillGC);
  penPtr->fillGC = newGC;

  // errorBarGC
  gcMask = GCForeground | GCLineWidth;
  XColor* colorPtr = penPtr->errorBarColor;
  if (!colorPtr)
    colorPtr = penPtr->outlineColor;
  gcValues.foreground = colorPtr->pixel;
  gcValues.line_width = LineWidth(penPtr->errorBarLineWidth);
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (penPtr->errorBarGC)
    Tk_FreeGC(graphPtr->display, penPtr->errorBarGC);
  penPtr->errorBarGC = newGC;

  return TCL_OK;
}

void DestroyBarPenProc(Graph* graphPtr, Pen* basePtr)
{
  BarPen* penPtr = (BarPen*)basePtr;

  Blt_Ts_FreeStyle(graphPtr->display, &penPtr->valueStyle);
  if (penPtr->outlineGC)
    Tk_FreeGC(graphPtr->display, penPtr->outlineGC);

  if (penPtr->fillGC)
    Tk_FreeGC(graphPtr->display, penPtr->fillGC);

  if (penPtr->errorBarGC)
    Tk_FreeGC(graphPtr->display, penPtr->errorBarGC);

  Tk_FreeConfigOptions((char*)penPtr, penPtr->optionTable, graphPtr->tkwin);
}

