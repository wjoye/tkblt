/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

extern "C" {
#include "bltGraph.h"
}

#include "bltGrHairs.h"

extern void TurnOffHairs(Tk_Window tkwin, Crosshairs *chPtr);
extern void TurnOnHairs(Graph* graphPtr, Crosshairs *chPtr);

#define PointInGraph(g,x,y) (((x) <= (g)->right) && ((x) >= (g)->left) && ((y) <= (g)->bottom) && ((y) >= (g)->top))

// OptionSpecs

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_COLOR, "-color", "color", "Color", 
   "green", -1, Tk_Offset(Crosshairs, colorPtr), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes", 
   NULL, -1, Tk_Offset(Crosshairs, dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "yes", -1, Tk_Offset(Crosshairs, hide), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "Linewidth",
   "0", -1, Tk_Offset(Crosshairs, lineWidth), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-position", "position", "Position", 
   NULL, -1, Tk_Offset(Crosshairs, hotSpot), 
   TK_OPTION_NULL_OK, &pointObjOption, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

int Blt_CreateCrosshairs(Graph* graphPtr)
{
  Crosshairs* chPtr = (Crosshairs*)calloc(1, sizeof(Crosshairs));
  chPtr->hide = 1;
  chPtr->hotSpot.x = chPtr->hotSpot.y = -1;
  graphPtr->crosshairs = chPtr;

  chPtr->optionTable = Tk_CreateOptionTable(graphPtr->interp, optionSpecs);
  return Tk_InitOptions(graphPtr->interp, (char*)chPtr, chPtr->optionTable, 
			graphPtr->tkwin);
}

void Blt_DestroyCrosshairs(Graph* graphPtr)
{
  Crosshairs *chPtr = graphPtr->crosshairs;
  if (!chPtr)
    return;

  if (chPtr->gc)
    Blt_FreePrivateGC(graphPtr->display, chPtr->gc);

  Tk_FreeConfigOptions((char*)chPtr, chPtr->optionTable, graphPtr->tkwin);
  free(chPtr);
}

// Configure

void ConfigureCrosshairs(Graph* graphPtr)
{
  Crosshairs *chPtr = graphPtr->crosshairs;

  // Turn off the crosshairs temporarily. This is in case the new
  // configuration changes the size, style, or position of the lines.
  TurnOffHairs(graphPtr->tkwin, chPtr);

  XGCValues gcValues;
  gcValues.function = GXxor;

  unsigned long int pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
  gcValues.background = pixel;
  gcValues.foreground = (pixel ^ chPtr->colorPtr->pixel);

  gcValues.line_width = LineWidth(chPtr->lineWidth);
  unsigned long gcMask = 
    (GCForeground | GCBackground | GCFunction | GCLineWidth);
  if (LineIsDashed(chPtr->dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  GC newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(chPtr->dashes))
    Blt_SetDashes(graphPtr->display, newGC, &chPtr->dashes);

  if (chPtr->gc != NULL)
    Blt_FreePrivateGC(graphPtr->display, chPtr->gc);

  chPtr->gc = newGC;

  // Are the new coordinates on the graph?
  chPtr->segArr[0].x2 = chPtr->segArr[0].x1 = chPtr->hotSpot.x;
  chPtr->segArr[0].y1 = graphPtr->bottom;
  chPtr->segArr[0].y2 = graphPtr->top;
  chPtr->segArr[1].y2 = chPtr->segArr[1].y1 = chPtr->hotSpot.y;
  chPtr->segArr[1].x1 = graphPtr->left;
  chPtr->segArr[1].x2 = graphPtr->right;

  if (!chPtr->hide)
    TurnOnHairs(graphPtr, chPtr);
}

// Support

void Blt_EnableCrosshairs(Graph* graphPtr)
{
  if (!graphPtr->crosshairs->hide)
    TurnOnHairs(graphPtr, graphPtr->crosshairs);
}

void Blt_DisableCrosshairs(Graph* graphPtr)
{
  if (!graphPtr->crosshairs->hide)
    TurnOffHairs(graphPtr->tkwin, graphPtr->crosshairs);
}

void TurnOffHairs(Tk_Window tkwin, Crosshairs *chPtr)
{
  if (Tk_IsMapped(tkwin) && (chPtr->visible)) {
    XDrawSegments(Tk_Display(tkwin), Tk_WindowId(tkwin), chPtr->gc,
		  chPtr->segArr, 2);
    chPtr->visible = 0;
  }
}

void TurnOnHairs(Graph* graphPtr, Crosshairs *chPtr)
{
  if (Tk_IsMapped(graphPtr->tkwin) && (!chPtr->visible)) {
    if (!PointInGraph(graphPtr, chPtr->hotSpot.x, chPtr->hotSpot.y)) {
      return;		/* Coordinates are off the graph */
    }
    XDrawSegments(graphPtr->display, Tk_WindowId(graphPtr->tkwin),
		  chPtr->gc, chPtr->segArr, 2);
    chPtr->visible = 1;
  }
}

