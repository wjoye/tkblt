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

#include <stdlib.h>

#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrElem.h"
#include "bltGrLegd.h"
#include "bltGrMisc.h"

using namespace Blt;

#define ROUND(x) 	((int)((x) + (((x)<0.0) ? -0.5 : 0.5)))
#define AXIS_PAD_TITLE 2

/*
 *---------------------------------------------------------------------------
 *
 * layoutGraph --
 *
 *	Calculate the layout of the graph.  Based upon the data, axis limits,
 *	X and Y titles, and title height, determine the cavity left which is
 *	the plotting surface.  The first step get the data and axis limits for
 *	calculating the space needed for the top, bottom, left, and right
 *	margins.
 *
 * 	1) The LEFT margin is the area from the left border to the Y axis 
 *	   (not including ticks). It composes the border width, the width an 
 *	   optional Y axis label and its padding, and the tick numeric labels. 
 *	   The Y axis label is rotated 90 degrees so that the width is the 
 *	   font height.
 *
 * 	2) The RIGHT margin is the area from the end of the graph
 *	   to the right window border. It composes the border width,
 *	   some padding, the font height (this may be dubious. It
 *	   appears to provide a more even border), the max of the
 *	   legend width and 1/2 max X tick number. This last part is
 *	   so that the last tick label is not clipped.
 *
 *           Window Width
 *      ___________________________________________________________
 *      |          |                               |               |
 *      |          |   TOP  height of title        |               |
 *      |          |                               |               |
 *      |          |           x2 title            |               |
 *      |          |                               |               |
 *      |          |        height of x2-axis      |               |
 *      |__________|_______________________________|_______________|  W
 *      |          | -plotpady                     |               |  i
 *      |__________|_______________________________|_______________|  n
 *      |          | top                   right   |               |  d
 *      |          |                               |               |  o
 *      |   LEFT   |                               |     RIGHT     |  w
 *      |          |                               |               |
 *      | y        |     Free area = 104%          |      y2       |  H
 *      |          |     Plotting surface = 100%   |               |  e
 *      | t        |     Tick length = 2 + 2%      |      t        |  i
 *      | i        |                               |      i        |  g
 *      | t        |                               |      t  legend|  h
 *      | l        |                               |      l   width|  t
 *      | e        |                               |      e        |
 *      |    height|                               |height         |
 *      |       of |                               | of            |
 *      |    y-axis|                               |y2-axis        |
 *      |          |                               |               |
 *      |          |origin 0,0                     |               |
 *      |__________|_left_________________bottom___|_______________|
 *      |          |-plotpady                      |               |
 *      |__________|_______________________________|_______________|
 *      |          | (xoffset, yoffset)            |               |
 *      |          |                               |               |
 *      |          |       height of x-axis        |               |
 *      |          |                               |               |
 *      |          |   BOTTOM   x title            |               |
 *      |__________|_______________________________|_______________|
 *
 * 3) The TOP margin is the area from the top window border to the top
 *    of the graph. It composes the border width, twice the height of
 *    the title font (if one is given) and some padding between the
 *    title.
 *
 * 4) The BOTTOM margin is area from the bottom window border to the
 *    X axis (not including ticks). It composes the border width, the height
 *    an optional X axis label and its padding, the height of the font
 *    of the tick labels.
 *
 * The plotting area is between the margins which includes the X and Y axes
 * including the ticks but not the tick numeric labels. The length of the
 * ticks and its padding is 5% of the entire plotting area.  Hence the entire
 * plotting area is scaled as 105% of the width and height of the area.
 *
 * The axis labels, ticks labels, title, and legend may or may not be
 * displayed which must be taken into account.
 *
 * if reqWidth > 0 : set outer size
 * if reqPlotWidth > 0 : set plot size
 *---------------------------------------------------------------------------
 */

void Graph::layoutGraph()
{
  GraphOptions* ops = (GraphOptions*)ops_;
  int titleY;
  int plotWidth, plotHeight;
  int inset, inset2;

  int width = width_;
  int height = height_;

  /* 
   * Step 1:  Compute the amount of space needed to display the axes
   *		associated with each margin.  They can be overridden by 
   *		-leftmargin, -rightmargin, -bottommargin, and -topmargin
   *		graph options, respectively.
   */
  int left   = getMarginGeometry(&ops->leftMargin);
  int right  = getMarginGeometry(&ops->rightMargin);
  int top    = getMarginGeometry(&ops->topMargin);
  int bottom = getMarginGeometry(&ops->bottomMargin);

  int pad = ops->bottomMargin.maxTickWidth;
  if (pad < ops->topMargin.maxTickWidth)
    pad = ops->topMargin.maxTickWidth;

  pad = pad / 2 + 3;
  if (right < pad)
    right = pad;

  if (left < pad)
    left = pad;

  pad = ops->leftMargin.maxTickHeight;
  if (pad < ops->rightMargin.maxTickHeight)
    pad = ops->rightMargin.maxTickHeight;

  pad = pad / 2;
  if (top < pad)
    top = pad;

  if (bottom < pad)
    bottom = pad;

  if (ops->leftMargin.reqSize > 0)
    left = ops->leftMargin.reqSize;

  if (ops->rightMargin.reqSize > 0)
    right = ops->rightMargin.reqSize;

  if (ops->topMargin.reqSize > 0)
    top = ops->topMargin.reqSize;

  if (ops->bottomMargin.reqSize > 0)
    bottom = ops->bottomMargin.reqSize;

  /* 
   * Step 2:  Add the graph title height to the top margin. 
   */
  if (ops->title)
    top += titleHeight_ + 6;

  inset = (inset_ + ops->plotBW);
  inset2 = 2 * inset;

  /* 
   * Step 3: Estimate the size of the plot area from the remaining
   *	       space.  This may be overridden by the -plotwidth and
   *	       -plotheight graph options.  We use this to compute the
   *	       size of the legend. 
   */
  if (width == 0)
    width = 400;

  if (height == 0)
    height = 400;

  plotWidth  = (ops->reqPlotWidth > 0) ? ops->reqPlotWidth :
    width - (inset2 + left + right); /* Plot width. */
  plotHeight = (ops->reqPlotHeight > 0) ? ops->reqPlotHeight : 
    height - (inset2 + top + bottom); /* Plot height. */
  legend_->map(plotWidth, plotHeight);

  /* 
   * Step 2:  Add the legend to the appropiate margin. 
   */
  if (!legend_->isHidden()) {
    switch (legend_->position()) {
    case Legend::RIGHT:
      right += legend_->width() + 2;
      break;
    case Legend::LEFT:
      left += legend_->width() + 2;
      break;
    case Legend::TOP:
      top += legend_->height() + 2;
      break;
    case Legend::BOTTOM:
      bottom += legend_->height() + 2;
      break;
    case Legend::XY:
    case Legend::PLOT:
      /* Do nothing. */
      break;
    }
  }

  /* 
   * Recompute the plotarea or graph size, now accounting for the legend. 
   */
  if (ops->reqPlotWidth == 0) {
    plotWidth = width  - (inset2 + left + right);
    if (plotWidth < 1)
      plotWidth = 1;
  }
  if (ops->reqPlotHeight == 0) {
    plotHeight = height - (inset2 + top + bottom);
    if (plotHeight < 1)
      plotHeight = 1;
  }

  /*
   * Step 5: If necessary, correct for the requested plot area aspect
   *	       ratio.
   */
  if ((ops->reqPlotWidth == 0) && (ops->reqPlotHeight == 0) && 
      (ops->aspect > 0.0f)) {
    float ratio;

    /* 
     * Shrink one dimension of the plotarea to fit the requested
     * width/height aspect ratio.
     */
    ratio = (float)plotWidth / (float)plotHeight;
    if (ratio > ops->aspect) {
      // Shrink the width
      int scaledWidth = (int)(plotHeight * ops->aspect);
      if (scaledWidth < 1)
	scaledWidth = 1;

      // Add the difference to the right margin.
      // CHECK THIS: w = scaledWidth
      right += (plotWidth - scaledWidth);
    }
    else {
      // Shrink the height
      int scaledHeight = (int)(plotWidth / ops->aspect);
      if (scaledHeight < 1)
	scaledHeight = 1;

      // Add the difference to the top margin
      // CHECK THIS: h = scaledHeight;
      top += (plotHeight - scaledHeight); 
    }
  }

  /* 
   * Step 6: If there's multiple axes in a margin, the axis titles will be
   *	       displayed in the adjoining margins.  Make sure there's room 
   *	       for the longest axis titles.
   */

  if (top < ops->leftMargin.axesTitleLength)
    top = ops->leftMargin.axesTitleLength;

  if (right < ops->bottomMargin.axesTitleLength)
    right = ops->bottomMargin.axesTitleLength;

  if (top < ops->rightMargin.axesTitleLength)
    top = ops->rightMargin.axesTitleLength;

  if (right < ops->topMargin.axesTitleLength)
    right = ops->topMargin.axesTitleLength;

  /* 
   * Step 7: Override calculated values with requested margin sizes.
   */
  if (ops->leftMargin.reqSize > 0)
    left = ops->leftMargin.reqSize;

  if (ops->rightMargin.reqSize > 0)
    right = ops->rightMargin.reqSize;

  if (ops->topMargin.reqSize > 0)
    top = ops->topMargin.reqSize;

  if (ops->bottomMargin.reqSize > 0)
    bottom = ops->bottomMargin.reqSize;

  if (ops->reqPlotWidth > 0) {	
    /* 
     * Width of plotarea is constained.  If there's extra space, add it to
     * th left and/or right margins.  If there's too little, grow the
     * graph width to accomodate it.
     */
    int w = plotWidth + inset2 + left + right;
    if (width > w) {		/* Extra space in window. */
      int extra = (width - w) / 2;
      if (ops->leftMargin.reqSize == 0) { 
	left += extra;
	if (ops->rightMargin.reqSize == 0) { 
	  right += extra;
	}
	else {
	  left += extra;
	}
      }
      else if (ops->rightMargin.reqSize == 0) {
	right += extra + extra;
      }
    }
    else if (width < w) {
      width = w;
    }
  } 
  if (ops->reqPlotHeight > 0) {	/* Constrain the plotarea height. */
    /* 
     * Height of plotarea is constained.  If there's extra space, 
     * add it to th top and/or bottom margins.  If there's too little,
     * grow the graph height to accomodate it.
     */
    int h = plotHeight + inset2 + top + bottom;
    if (height > h) {		/* Extra space in window. */
      int extra;

      extra = (height - h) / 2;
      if (ops->topMargin.reqSize == 0) { 
	top += extra;
	if (ops->bottomMargin.reqSize == 0) { 
	  bottom += extra;
	}
	else {
	  top += extra;
	}
      }
      else if (ops->bottomMargin.reqSize == 0) {
	bottom += extra + extra;
      }
    }
    else if (height < h) {
      height = h;
    }
  }	
  width_  = width;
  height_ = height;
  left_   = left + inset;
  top_    = top + inset;
  right_  = width - right - inset;
  bottom_ = height - bottom - inset;

  ops->leftMargin.width    = left   + inset_;
  ops->rightMargin.width   = right  + inset_;
  ops->topMargin.height    = top    + inset_;
  ops->bottomMargin.height = bottom + inset_;
	    
  vOffset_ = top_ + ops->yPad;
  vRange_  = plotHeight - 2*ops->yPad;
  hOffset_ = left_ + ops->xPad;
  hRange_  = plotWidth  - 2*ops->xPad;

  if (vRange_ < 1)
    vRange_ = 1;

  if (hRange_ < 1)
    hRange_ = 1;

  hScale_ = 1.0f / (float)hRange_;
  vScale_ = 1.0f / (float)vRange_;

  // Calculate the placement of the graph title so it is centered within the
  // space provided for it in the top margin
  titleY = titleHeight_;
  titleY_ = 3 + inset_;
  titleX_ = (right_ + left_) / 2;
}

int Graph::getMarginGeometry(Margin *marginPtr)
{
  GraphOptions* ops = (GraphOptions*)ops_;
  int isHoriz = !(marginPtr->site & 0x1); /* Even sites are horizontal */

  // Count the visible axes.
  unsigned int nVisible = 0;
  unsigned int l =0;
  int w =0;
  int h =0;

  marginPtr->maxTickWidth =0;
  marginPtr->maxTickHeight =0;

  if (ops->stackAxes) {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(marginPtr->axes);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_) {
	nVisible++;
	if (flags & GET_AXIS_GEOMETRY)
	  getAxisGeometry(axisPtr);

	if (isHoriz) {
	  if (h < axisPtr->height_)
	    h = axisPtr->height_;
	}
	else {
	  if (w < axisPtr->width_)
	    w = axisPtr->width_;
	}
	if (axisPtr->maxTickWidth_ > marginPtr->maxTickWidth)
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth_;

	if (axisPtr->maxTickHeight_ > marginPtr->maxTickHeight)
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight_;
      }
    }
  }
  else {
    for (Blt_ChainLink link = Blt_Chain_FirstLink(marginPtr->axes);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Axis* axisPtr = (Axis*)Blt_Chain_GetValue(link);
      AxisOptions* ops = (AxisOptions*)axisPtr->ops();
      if (!ops->hide && axisPtr->use_) {
	nVisible++;
	if (flags & GET_AXIS_GEOMETRY)
	  getAxisGeometry(axisPtr);

	if ((ops->titleAlternate) && (l < axisPtr->titleWidth_))
	  l = axisPtr->titleWidth_;

	if (isHoriz)
	  h += axisPtr->height_;
	else
	  w += axisPtr->width_;

	if (axisPtr->maxTickWidth_ > marginPtr->maxTickWidth)
	  marginPtr->maxTickWidth = axisPtr->maxTickWidth_;

	if (axisPtr->maxTickHeight_ > marginPtr->maxTickHeight)
	  marginPtr->maxTickHeight = axisPtr->maxTickHeight_;
      }
    }
  }
  // Enforce a minimum size for margins.
  if (w < 3)
    w = 3;

  if (h < 3)
    h = 3;

  marginPtr->nAxes = nVisible;
  marginPtr->axesTitleLength = l;
  marginPtr->width = w;
  marginPtr->height = h;
  marginPtr->axesOffset = (isHoriz) ? h : w;
  return marginPtr->axesOffset;
}

void Graph::getAxisGeometry(Axis *axisPtr)
{
  GraphOptions* ops = (GraphOptions*)ops_;
  AxisOptions* aops = (AxisOptions*)axisPtr->ops();
  axisPtr->freeTickLabels();

  // Leave room for axis baseline and padding
  unsigned int y =0;
  if (aops->exterior && (ops->plotRelief != TK_RELIEF_SOLID))
    y += aops->lineWidth + 2;

  axisPtr->maxTickHeight_ = axisPtr->maxTickWidth_ = 0;

  if (axisPtr->t1Ptr_)
    free(axisPtr->t1Ptr_);
  axisPtr->t1Ptr_ = axisPtr->generateTicks(&axisPtr->majorSweep_);
  if (axisPtr->t2Ptr_)
    free(axisPtr->t2Ptr_);
  axisPtr->t2Ptr_ = axisPtr->generateTicks(&axisPtr->minorSweep_);

  if (aops->showTicks) {
    Ticks* t1Ptr = aops->t1UPtr ? aops->t1UPtr : axisPtr->t1Ptr_;
	
    int nTicks =0;
    if (t1Ptr)
      nTicks = t1Ptr->nTicks;
	
    unsigned int nLabels =0;
    for (int ii=0; ii<nTicks; ii++) {
      double x = t1Ptr->values[ii];
      double x2 = t1Ptr->values[ii];
      if (aops->labelOffset)
	x2 += axisPtr->majorSweep_.step * 0.5;

      if (!axisPtr->inRange(x2, &axisPtr->axisRange_))
	continue;

      TickLabel* labelPtr = axisPtr->makeLabel(x);
      Blt_Chain_Append(axisPtr->tickLabels_, labelPtr);
      nLabels++;

      // Get the dimensions of each tick label.  Remember tick labels
      // can be multi-lined and/or rotated.
      int lw, lh;
      Blt_GetTextExtents(aops->tickFont, labelPtr->string, -1, &lw, &lh);
      labelPtr->width  = lw;
      labelPtr->height = lh;

      if (aops->tickAngle != 0.0f) {
	// Rotated label width and height
	double rlw, rlh;
	Blt_GetBoundingBox(lw, lh, aops->tickAngle, &rlw, &rlh, NULL);
	lw = ROUND(rlw), lh = ROUND(rlh);
      }
      if (axisPtr->maxTickWidth_ < int(lw))
	axisPtr->maxTickWidth_ = lw;

      if (axisPtr->maxTickHeight_ < int(lh))
	axisPtr->maxTickHeight_ = lh;
    }
	
    unsigned int pad =0;
    if (aops->exterior) {
      /* Because the axis cap style is "CapProjecting", we need to
       * account for an extra 1.5 linewidth at the end of each line.  */
      pad = ((aops->lineWidth * 12) / 8);
    }
    if (axisPtr->isHorizontal())
      y += axisPtr->maxTickHeight_ + pad;
    else {
      y += axisPtr->maxTickWidth_ + pad;
      if (axisPtr->maxTickWidth_ > 0)
	// Pad either size of label.
	y += 5;
    }
    y += 2 * AXIS_PAD_TITLE;
    if ((aops->lineWidth > 0) && aops->exterior)
      // Distance from axis line to tick label.
      y += aops->tickLength;

  } // showTicks

  if (aops->title) {
    if (aops->titleAlternate) {
      if (y < axisPtr->titleHeight_)
	y = axisPtr->titleHeight_;
    } 
    else
      y += axisPtr->titleHeight_ + AXIS_PAD_TITLE;
  }

  // Correct for orientation of the axis
  if (axisPtr->isHorizontal())
    axisPtr->height_ = y;
  else
    axisPtr->width_ = y;
}


