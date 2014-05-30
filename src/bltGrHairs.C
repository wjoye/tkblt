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

#include <stdlib.h>

#include "bltGraph.h"
#include "bltGrHairs.h"
#include "bltGrMisc.h"
#include "bltConfig.h"

using namespace Blt;

#define PointInGraph(g,x,y) (((x) <= (g)->right_) && ((x) >= (g)->left_) && ((y) <= (g)->bottom_) && ((y) >= (g)->top_))

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_COLOR, "-color", "color", "Color", 
   "green", -1, Tk_Offset(CrosshairsOptions, colorPtr), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-dashes", "dashes", "Dashes", 
   NULL, -1, Tk_Offset(CrosshairsOptions, dashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "yes", -1, Tk_Offset(CrosshairsOptions, hide), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-linewidth", "lineWidth", "Linewidth",
   "0", -1, Tk_Offset(CrosshairsOptions, lineWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-x", "x", "X",
   "0", -1, Tk_Offset(CrosshairsOptions, x), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-y", "y", "Y",
   "0", -1, Tk_Offset(CrosshairsOptions, y), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

Crosshairs::Crosshairs(Graph* graphPtr)
{
  ops_ = (CrosshairsOptions*)calloc(1, sizeof(CrosshairsOptions));

  graphPtr_ = graphPtr;
  visible_ =0;
  gc_ =NULL;

  optionTable_ = Tk_CreateOptionTable(graphPtr->interp_, optionSpecs);
  Tk_InitOptions(graphPtr->interp_, (char*)ops_, optionTable_, graphPtr->tkwin_);
}

Crosshairs::~Crosshairs()
{
  if (gc_)
    graphPtr_->freePrivateGC(gc_);

  Tk_FreeConfigOptions((char*)ops_, optionTable_, graphPtr_->tkwin_);
  free(ops_);
}

// Configure

int Crosshairs::configure()
{
  CrosshairsOptions* ops = (CrosshairsOptions*)ops_;
  GraphOptions* gops = (GraphOptions*)graphPtr_->ops_;

  // Turn off the crosshairs temporarily. This is in case the new
  // configuration changes the size, style, or position of the lines.
  off();

  XGCValues gcValues;
  gcValues.function = GXxor;

  unsigned long int pixel = Tk_3DBorderColor(gops->plotBg)->pixel;
  gcValues.background = pixel;
  gcValues.foreground = (pixel ^ ops->colorPtr->pixel);

  gcValues.line_width = ops->lineWidth;
  unsigned long gcMask = 
    (GCForeground | GCBackground | GCFunction | GCLineWidth);
  if (LineIsDashed(ops->dashes)) {
    gcValues.line_style = LineOnOffDash;
    gcMask |= GCLineStyle;
  }
  GC newGC = graphPtr_->getPrivateGC(gcMask, &gcValues);
  if (LineIsDashed(ops->dashes))
    Blt_SetDashes(graphPtr_->display_, newGC, &ops->dashes);

  if (gc_ != NULL)
    graphPtr_->freePrivateGC(gc_);

  gc_ = newGC;

  // Are the new coordinates on the graph?
  segArr_[0].x1 = ops->x;
  segArr_[0].x2 = ops->x;
  segArr_[0].y1 = graphPtr_->bottom_;
  segArr_[0].y2 = graphPtr_->top_;
  segArr_[1].y1 = ops->y;
  segArr_[1].y2 = ops->y;
  segArr_[1].x1 = graphPtr_->left_;
  segArr_[1].x2 = graphPtr_->right_;

  enable();

  return TCL_OK;
}

void Crosshairs::enable()
{
  CrosshairsOptions* ops = (CrosshairsOptions*)ops_;
  if (!ops->hide)
    on();
}

void Crosshairs::disable()
{
  CrosshairsOptions* ops = (CrosshairsOptions*)ops_;
  if (!ops->hide)
    off();
}

void Crosshairs::off()
{
  if (Tk_IsMapped(graphPtr_->tkwin_) && (visible_)) {
    XDrawSegments(graphPtr_->display_, Tk_WindowId(graphPtr_->tkwin_),
		  gc_, segArr_, 2);
    visible_ = 0;
  }
}

void Crosshairs::on()
{
  CrosshairsOptions* ops = (CrosshairsOptions*)ops_;

  if (Tk_IsMapped(graphPtr_->tkwin_) && (!visible_)) {
    if (!PointInGraph(graphPtr_, ops->x, ops->y))
      return;

    XDrawSegments(graphPtr_->display_, Tk_WindowId(graphPtr_->tkwin_),
		  gc_, segArr_, 2);
    visible_ = 1;
  }
}



