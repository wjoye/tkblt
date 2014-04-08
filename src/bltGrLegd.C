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

extern "C" {
#include "bltGraph.h"
#include "bltOp.h"
};

#include "bltGrLegd.h"
#include "bltConfig.h"
#include "bltGrElem.h"
#include "bltGrElemOp.h"

#define LABEL_PAD	2

// Defs

extern "C" {
  Tcl_ObjCmdProc Blt_GraphInstCmdProc;
};

extern int EntryIsSelected(Legend* legendPtr, Element* elemPtr);
extern void DeselectElement(Legend* legendPtr, Element* elemPtr);
extern void SelectEntry(Legend* legendPtr, Element* elemPtr);

static void SetLegendOrigin(Legend*);
static Tcl_IdleProc DisplayLegend;
static Blt_BindPickProc PickEntryProc;
static Tk_SelectionProc SelectionProc;

// OptionSpecs

static const char* selectmodeObjOption[] = {"single", "multiple", NULL};

static Tk_CustomOptionSetProc PositionSetProc;
static Tk_CustomOptionGetProc PositionGetProc;
Tk_ObjCustomOption positionObjOption =
  {
    "position", PositionSetProc, PositionGetProc, NULL, NULL, NULL
  };

static int PositionSetProc(ClientData clientData, Tcl_Interp* interp,
			   Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			   int offset, char* save, int flags)
{
  LegendOptions* ops = (LegendOptions*)widgRec;
  Legend* legendPtr = ops->legendPtr;

  int length;
  const char* string = Tcl_GetStringFromObj(*objPtr, &length);
  char c;
  c = string[0];
  if (c == '\0')
    legendPtr->site_ = LEGEND_RIGHT;
  else if ((c == 'l') && (strncmp(string, "leftmargin", length) == 0))
    legendPtr->site_ = LEGEND_LEFT;
  else if ((c == 'r') && (strncmp(string, "rightmargin", length) == 0))
    legendPtr->site_ = LEGEND_RIGHT;
  else if ((c == 't') && (strncmp(string, "topmargin", length) == 0))
    legendPtr->site_ = LEGEND_TOP;
  else if ((c == 'b') && (strncmp(string, "bottommargin", length) == 0))
    legendPtr->site_ = LEGEND_BOTTOM;
  else if ((c == 'p') && (strncmp(string, "plotarea", length) == 0))
    legendPtr->site_ = LEGEND_PLOT;
  else if (c == '@') {
    char *comma;
    long x, y;
    int result;
	
    comma = (char*)strchr(string + 1, ',');
    if (comma == NULL) {
      Tcl_AppendResult(interp, "bad screen position \"", string,
		       "\": should be @x,y", (char *)NULL);
      return TCL_ERROR;
    }
    x = y = 0;
    *comma = '\0';
    result = ((Tcl_ExprLong(interp, string + 1, &x) == TCL_OK) &&
	      (Tcl_ExprLong(interp, comma + 1, &y) == TCL_OK));
    *comma = ',';
    if (!result) {
      return TCL_ERROR;
    }
    legendPtr->xReq_ = x;
    legendPtr->yReq_ = y;
    legendPtr->site_ = LEGEND_XY;
  }
  else {
    Tcl_AppendResult(interp, "bad position \"", string, "\": should be  \
\"leftmargin\", \"rightmargin\", \"topmargin\", \"bottommargin\", \
\"plotarea\", or @x,y", (char *)NULL);
    return TCL_ERROR;
  }
  return TCL_OK;
}

static Tcl_Obj* PositionGetProc(ClientData clientData, Tk_Window tkwin, 
				char *widgRec, int offset)
{
  LegendOptions* ops = (LegendOptions*)widgRec;
  Legend* legendPtr = ops->legendPtr;

  Tcl_Obj *objPtr;

  switch (legendPtr->site_) {
  case LEGEND_LEFT:
    objPtr = Tcl_NewStringObj("leftmargin", -1);
    break;
  case LEGEND_RIGHT:
    objPtr = Tcl_NewStringObj("rightmargin", -1);
    break;
  case LEGEND_TOP:
    objPtr = Tcl_NewStringObj("topmargin", -1);
    break;
  case LEGEND_BOTTOM:
    objPtr = Tcl_NewStringObj("bottommargin", -1);
    break;
  case LEGEND_PLOT:
    objPtr = Tcl_NewStringObj("plotarea", -1);
    break;
  case LEGEND_XY:
    {
      char string[200];

      sprintf_s(string, 200, "@%d,%d", legendPtr->xReq_, legendPtr->yReq_);
      objPtr = Tcl_NewStringObj(string, -1);
    }
  default:
    objPtr = Tcl_NewStringObj("unknown legend position", -1);
  }
  return objPtr;
}

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_BORDER, "-activebackground", "activeBackground",
   "ActiveBackground", 
   STD_ACTIVE_BACKGROUND, -1, Tk_Offset(LegendOptions, activeBg), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-activeborderwidth", "activeBorderWidth", 
   "ActiveBorderWidth", 
   STD_BORDERWIDTH, -1, Tk_Offset(LegendOptions, entryBW), 0, NULL, 0}, 
  {TK_OPTION_COLOR, "-activeforeground", "activeForeground", "ActiveForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(LegendOptions, activeFgColor), 0, NULL, 0},
  {TK_OPTION_RELIEF, "-activerelief", "activeRelief", "ActiveRelief",
   "flat", -1, Tk_Offset(LegendOptions, activeRelief), 0, NULL, 0},
  {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor", 
   "n", -1, Tk_Offset(LegendOptions, anchor), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-bg", NULL, NULL, NULL, -1, 0, 0, "-background", 0},
  {TK_OPTION_BORDER, "-background", "background", "Background",
   NULL, -1, Tk_Offset(LegendOptions, normalBg), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
   STD_BORDERWIDTH, -1, Tk_Offset(LegendOptions, borderWidth), 0, NULL, 0}, 
  {TK_OPTION_SYNONYM, "-bd", NULL, NULL, NULL, -1, 0, 0, "-borderwidth", 0},
  {TK_OPTION_INT, "-columns", "columns", "columns",
   "0", -1, Tk_Offset(LegendOptions, reqColumns), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-exportselection", "exportSelection", "ExportSelection", 
   "no",  -1, Tk_Offset(LegendOptions, exportSelection), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-focusdashes", "focusDashes", "FocusDashes",
   "dot", -1, Tk_Offset(LegendOptions, focusDashes), 
   TK_OPTION_NULL_OK, &dashesObjOption, 0},
  {TK_OPTION_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(LegendOptions, focusColor), 0, NULL, 0},
  {TK_OPTION_FONT, "-font", "font", "Font", 
   STD_FONT_SMALL, -1, Tk_Offset(LegendOptions, style.font), 0, NULL, 0},
  {TK_OPTION_SYNONYM, "-fg", NULL, NULL, NULL, -1, 0, 0, "-foreground", 0},
  {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LegendOptions, fgColor), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-hide", "hide", "Hide", 
   "no", -1, Tk_Offset(LegendOptions, hide), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-ipadx", "iPadX", "Pad", 
   "1", -1, Tk_Offset(LegendOptions, ixPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-ipady", "iPadY", "Pad", 
   "1", -1, Tk_Offset(LegendOptions, iyPad), 0, NULL, 0},
  {TK_OPTION_BORDER, "-nofocusselectbackground", "noFocusSelectBackground", 
   "NoFocusSelectBackground", 
   STD_ACTIVE_BACKGROUND, -1, Tk_Offset(LegendOptions, selOutFocusBg), 0, NULL, 0},
  {TK_OPTION_COLOR, "-nofocusselectforeground", "noFocusSelectForeground", 
   "NoFocusSelectForeground", 
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(LegendOptions, selOutFocusFgColor), 0, NULL,0},
  {TK_OPTION_PIXELS, "-padx", "padX", "Pad", 
   "1", -1, Tk_Offset(LegendOptions, xPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-pady", "padY", "Pad", 
   "1", -1, Tk_Offset(LegendOptions, yPad), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-position", "position", "Position", 
   "rightmargin", -1, 0, 0, &positionObjOption, 0},
  {TK_OPTION_BOOLEAN, "-raised", "raised", "Raised", 
   "no", -1, Tk_Offset(LegendOptions, raised), 0, NULL, 0},
  {TK_OPTION_RELIEF, "-relief", "relief", "Relief", 
   "flat", -1, Tk_Offset(LegendOptions, relief), 0, NULL, 0},
  {TK_OPTION_INT, "-rows", "rows", "rows", 
   "0", -1, Tk_Offset(LegendOptions, reqRows), 0, NULL, 0},
  {TK_OPTION_BORDER, "-selectbackground", "selectBackground", 
   "SelectBackground", 
   STD_ACTIVE_BACKGROUND, -1, Tk_Offset(LegendOptions, selInFocusBg), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-selectborderwidth", "selectBorderWidth", 
   "SelectBorderWidth", 
   "1", -1, Tk_Offset(LegendOptions, selBW), 0, NULL, 0},
  {TK_OPTION_STRING, "-selectcommand", "selectCommand", "SelectCommand",
   NULL, -1, Tk_Offset(LegendOptions, selectCmd), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_COLOR, "-selectforeground", "selectForeground", "SelectForeground",
   STD_ACTIVE_FOREGROUND, -1, Tk_Offset(LegendOptions, selInFocusFgColor), 0, NULL, 0},
  {TK_OPTION_STRING_TABLE, "-selectmode", "selectMode", "SelectMode",
   "multiple", -1, Tk_Offset(LegendOptions, selectMode), 0, &selectmodeObjOption, 0},
  {TK_OPTION_RELIEF, "-selectrelief", "selectRelief", "SelectRelief",
   "flat", -1, Tk_Offset(LegendOptions, selRelief), 0, NULL, 0},
  {TK_OPTION_STRING, "-takefocus", "takeFocus", "TakeFocus", 
   NULL, -1, Tk_Offset(LegendOptions, takeFocus), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_STRING, "-title", "title", "Title", 
   NULL, -1, Tk_Offset(LegendOptions, title), TK_OPTION_NULL_OK, NULL, 0},
  {TK_OPTION_ANCHOR, "-titleanchor", "titleAnchor", "TitleAnchor", 
   "nw", -1, Tk_Offset(LegendOptions, titleStyle.anchor), 0, NULL, 0},
  {TK_OPTION_COLOR, "-titlecolor", "titleColor", "TitleColor",
   STD_NORMAL_FOREGROUND, -1, Tk_Offset(LegendOptions, titleStyle.color), 0, NULL, 0},
  {TK_OPTION_FONT, "-titlefont", "titleFont", "TitleFont",
   STD_FONT_SMALL, -1, Tk_Offset(LegendOptions, titleStyle.font), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

// Create

Legend::Legend(Graph* graphPtr)
{
  ops_ = (void*)calloc(1, sizeof(LegendOptions));
  LegendOptions* ops = (LegendOptions*)ops_;
  ops->legendPtr = this;

  graphPtr_ = graphPtr;
  flags =0;
  nEntries_ =0;
  nColumns_ =0;
  nRows_ =0;
  width_ =0;
  height_ =0;
  entryWidth_ =0;
  entryHeight_ =0;
  site_ =0;
  xReq_ = -SHRT_MAX;
  yReq_ = -SHRT_MAX;
  x_ =0;
  y_ =0;
  maxSymSize_ =0;
  bindTable_ =NULL;
  focusGC_ =NULL;
  focus_ =0;
  cursorX_ =0;
  cursorY_ =0;
  cursorWidth_ =0;
  cursorHeight_ =0;
  focusPtr_ =NULL;
  selAnchorPtr_ =NULL;
  selMarkPtr_ =NULL;
  selFirstPtr_ =NULL;
  selLastPtr_ =NULL;
  selected_ = Blt_Chain_Create();
  titleWidth_ =0;
  titleHeight_ =0;

  Blt_Ts_InitStyle(ops->style);
  Blt_Ts_InitStyle(ops->titleStyle);
  ops->style.justify = TK_JUSTIFY_LEFT;
  ops->style.anchor = TK_ANCHOR_NW;
  ops->titleStyle.justify = TK_JUSTIFY_LEFT;
  ops->titleStyle.anchor = TK_ANCHOR_NW;

  bindTable_ = Blt_CreateBindingTable(graphPtr->interp, graphPtr->tkwin, 
				      graphPtr, PickEntryProc, Blt_GraphTags);

  Tcl_InitHashTable(&selectTable_, TCL_ONE_WORD_KEYS);

  Tk_CreateSelHandler(graphPtr_->tkwin, XA_PRIMARY, XA_STRING, 
		      SelectionProc, this, XA_STRING);

  optionTable_ =Tk_CreateOptionTable(graphPtr->interp, optionSpecs);
  Tk_InitOptions(graphPtr->interp, (char*)ops_, optionTable_, graphPtr->tkwin);
}

Legend::~Legend()
{
  LegendOptions* ops = (LegendOptions*)ops_;

  Blt_Ts_FreeStyle(graphPtr_->display, &ops->style);
  Blt_Ts_FreeStyle(graphPtr_->display, &ops->titleStyle);
  Blt_DestroyBindingTable(bindTable_);
    
  if (focusGC_)
    Blt_FreePrivateGC(graphPtr_->display, focusGC_);

    if (graphPtr_->tkwin)
    Tk_DeleteSelHandler(graphPtr_->tkwin, XA_PRIMARY, XA_STRING);

  Blt_Chain_Destroy(selected_);

  Tk_FreeConfigOptions((char*)ops_, optionTable_, graphPtr_->tkwin);
  if (ops_)
    free(ops_);
}

void Legend::configure()
{
  LegendOptions* ops = (LegendOptions*)ops_;

  /* GC for active label. Dashed outline. */
  unsigned long gcMask = GCForeground | GCLineStyle;
  XGCValues gcValues;
  gcValues.foreground = ops->focusColor->pixel;
  gcValues.line_style = (LineIsDashed(ops->focusDashes))
    ? LineOnOffDash : LineSolid;
  GC newGC = Blt_GetPrivateGC(graphPtr_->tkwin, gcMask, &gcValues);
  if (LineIsDashed(ops->focusDashes)) {
    ops->focusDashes.offset = 2;
    Blt_SetDashes(graphPtr_->display, newGC, &ops->focusDashes);
  }
  if (focusGC_)
    Blt_FreePrivateGC(graphPtr_->display, focusGC_);

  focusGC_ = newGC;
}

void Legend::map(int plotWidth, int plotHeight)
{
  LegendOptions* ops = (LegendOptions*)ops_;
  
  entryWidth_ =0;
  entryHeight_ = 0;
  nRows_ =0;
  nColumns_ =0;
  nEntries_ =0;
  height_ =0;
  width_ = 0;

  Blt_Ts_GetExtents(&ops->titleStyle, ops->title, 
		    &titleWidth_, &titleHeight_);
  /*   
   * Count the number of legend entries and determine the widest and tallest
   * label.  The number of entries would normally be the number of elements,
   * but elements can have no legend entry (-label "").
   */
  int nEntries =0;
  int maxWidth =0;
  int maxHeight =0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr_->elements.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
    unsigned int w, h;
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();

    if (elemOps->label == NULL)
      continue;

    Blt_Ts_GetExtents(&ops->style, elemOps->label, &w, &h);
    if (maxWidth < (int)w)
      maxWidth = w;

    if (maxHeight < (int)h)
      maxHeight = h;

    nEntries++;
  }
  if (nEntries == 0)
    return;				/* No visible legend entries. */

  Tk_FontMetrics fontMetrics;
  Tk_GetFontMetrics(ops->style.font, &fontMetrics);
  int symbolWidth = 2 * fontMetrics.ascent;

  maxWidth += 2 * ops->entryBW + 2*ops->ixPad +
    + symbolWidth + 3 * LABEL_PAD;

  maxHeight += 2 * ops->entryBW + 2*ops->iyPad;

  maxWidth |= 0x01;
  maxHeight |= 0x01;

  int lw = plotWidth - 2 * ops->borderWidth - 2*ops->xPad;
  int lh = plotHeight - 2 * ops->borderWidth - 2*ops->yPad;

  /*
   * The number of rows and columns is computed as one of the following:
   *
   *	both options set		User defined. 
   *  -rows				Compute columns from rows.
   *  -columns			Compute rows from columns.
   *	neither set			Compute rows and columns from
   *					size of plot.  
   */
  int nRows =0;
  int nColumns =0;
  if (ops->reqRows > 0) {
    nRows = MIN(ops->reqRows, nEntries); 
    if (ops->reqColumns > 0)
      nColumns = MIN(ops->reqColumns, nEntries);
    else
      nColumns = ((nEntries - 1) / nRows) + 1; /* Only -rows. */
  }
  else if (ops->reqColumns > 0) { /* Only -columns. */
    nColumns = MIN(ops->reqColumns, nEntries);
    nRows = ((nEntries - 1) / nColumns) + 1;
  }
  else {			
    // Compute # of rows and columns from the legend size
    nRows = lh / maxHeight;
    nColumns = lw / maxWidth;
    if (nRows < 1) {
      nRows = nEntries;
    }
    if (nColumns < 1) {
      nColumns = nEntries;
    }
    if (nRows > nEntries) {
      nRows = nEntries;
    } 
    switch (site_) {
    case LEGEND_TOP:
    case LEGEND_BOTTOM:
      nRows = ((nEntries - 1) / nColumns) + 1;
      break;
    case LEGEND_LEFT:
    case LEGEND_RIGHT:
    default:
      nColumns = ((nEntries - 1) / nRows) + 1;
      break;
    }
  }
  if (nColumns < 1)
    nColumns = 1;

  if (nRows < 1)
    nRows = 1;

  lh = (nRows * maxHeight);
  if (titleHeight_ > 0)
    lh += titleHeight_ + ops->yPad;

  lw = nColumns * maxWidth;
  if (lw < (int)(titleWidth_))
    lw = titleWidth_;

  width_ = lw + 2 * ops->borderWidth + 2*ops->xPad;
  height_ = lh + 2 * ops->borderWidth + 2*ops->yPad;
  nRows_ = nRows;
  nColumns_ = nColumns;
  nEntries_ = nEntries;
  entryHeight_ = maxHeight;
  entryWidth_ = maxWidth;

  {
    int row =0;
    int col =0;
    int count =0;
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr_->elements.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      count++;
      elemPtr->row_ = row;
      elemPtr->col_ = col;
      row++;
      if ((count % nRows) == 0) {
	col++;
	row = 0;
      }
    }
  }
}

void Legend::draw(Drawable drawable)
{
  LegendOptions* ops = (LegendOptions*)ops_;

  if ((ops->hide) || (nEntries_ == 0))
    return;

  SetLegendOrigin(this);
  Tk_Window tkwin = graphPtr_->tkwin;
  int w = width_;
  int h = height_;

  Pixmap pixmap = Tk_GetPixmap(graphPtr_->display, Tk_WindowId(tkwin), w, h, 
			       Tk_Depth(tkwin));

  if (ops->normalBg)
    Tk_Fill3DRectangle(tkwin, pixmap, ops->normalBg, 0, 0, 
		       w, h, 0, TK_RELIEF_FLAT);

  else if (site_ & LEGEND_PLOTAREA_MASK) {
    // Legend background is transparent and is positioned over the the
    // plot area.  Either copy the part of the background from the backing
    // store pixmap or (if no backing store exists) just fill it with the
    // background color of the plot.
    if (graphPtr_->cache != None)
      XCopyArea(graphPtr_->display, graphPtr_->cache, pixmap, 
		graphPtr_->drawGC, x_, y_, w, h, 0, 0);
    else 
      Tk_Fill3DRectangle(tkwin, pixmap, graphPtr_->plotBg, 0, 0, 
			 w, h, TK_RELIEF_FLAT, 0);
  }
  else
    // The legend is located in one of the margins
    Tk_Fill3DRectangle(tkwin, pixmap, graphPtr_->normalBg, 0, 0, 
		       w, h, 0, TK_RELIEF_FLAT);

  Tk_FontMetrics fontMetrics;
  Tk_GetFontMetrics(ops->style.font, &fontMetrics);

  int symbolSize = fontMetrics.ascent;
  int xMid = symbolSize + 1 + ops->entryBW;
  int yMid = (symbolSize / 2) + 1 + ops->entryBW;
  int xLabel = 2 * symbolSize + ops->entryBW +  ops->ixPad + 2 * LABEL_PAD;
  int ySymbol = yMid + ops->iyPad; 
  int xSymbol = xMid + LABEL_PAD;

  int x = ops->xPad + ops->borderWidth;
  int y = ops->yPad + ops->borderWidth;
  Blt_DrawText(tkwin, pixmap, ops->title, &ops->titleStyle, x, y);
  if (titleHeight_ > 0)
    y += titleHeight_ + ops->yPad;

  int count = 0;
  int yStart = y;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr_->elements.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    int isSelected = EntryIsSelected(this, elemPtr);
    if (elemPtr->flags & LABEL_ACTIVE) {
      Tk_Fill3DRectangle(tkwin, pixmap, ops->activeBg, 
			 x, y, entryWidth_, entryHeight_, 
			 ops->entryBW, ops->activeRelief);
    }
    else if (isSelected) {
      XColor* fg = (flags & FOCUS) ?
	ops->selInFocusFgColor : ops->selOutFocusFgColor;
      Tk_3DBorder bg = (flags & FOCUS) ?
	ops->selInFocusBg : ops->selOutFocusBg;
      Blt_Ts_SetForeground(ops->style, fg);
      Tk_Fill3DRectangle(tkwin, pixmap, bg, x, y, 
			 entryWidth_, entryHeight_, 
			 ops->selBW, ops->selRelief);
    }
    else {
      Blt_Ts_SetForeground(ops->style, ops->fgColor);
      if (elemOps->legendRelief != TK_RELIEF_FLAT)
	Tk_Fill3DRectangle(tkwin, pixmap, graphPtr_->normalBg, 
			   x, y, entryWidth_, 
			   entryHeight_, ops->entryBW, 
			   elemOps->legendRelief);
    }
    elemPtr->drawSymbol(pixmap, x + xSymbol, y + ySymbol, symbolSize);
    Blt_DrawText(tkwin, pixmap, elemOps->label, &ops->style, 
		 x + xLabel, 
		 y + ops->entryBW + ops->iyPad);
    count++;
    if (focusPtr_ == elemPtr) { /* Focus outline */
      if (isSelected) {
	XColor* color = (flags & FOCUS) ?
	  ops->selInFocusFgColor : ops->selOutFocusFgColor;
	XSetForeground(graphPtr_->display, focusGC_, 
		       color->pixel);
      }
      XDrawRectangle(graphPtr_->display, pixmap, focusGC_, 
		     x + 1, y + 1, entryWidth_ - 3, 
		     entryHeight_ - 3);
      if (isSelected) {
	XSetForeground(graphPtr_->display, focusGC_, 
		       ops->focusColor->pixel);
      }
    }
    // Check when to move to the next column
    if ((count % nRows_) > 0)
      y += entryHeight_;
    else {
      x += entryWidth_;
      y = yStart;
    }
  }

  Tk_3DBorder bg = ops->normalBg;
  if (bg == NULL)
    bg = graphPtr_->normalBg;

  // Disable crosshairs before redisplaying to the screen
  if (site_ & LEGEND_PLOTAREA_MASK)
    Blt_DisableCrosshairs(graphPtr_);

  Tk_Draw3DRectangle(tkwin, pixmap, bg, 0, 0, w, h, 
		     ops->borderWidth, ops->relief);
  XCopyArea(graphPtr_->display, pixmap, drawable, graphPtr_->drawGC, 0, 0, w, h,
	    x_, y_);

  if (site_ & LEGEND_PLOTAREA_MASK)
    Blt_EnableCrosshairs(graphPtr_);

  Tk_FreePixmap(graphPtr_->display, pixmap);
  graphPtr_->flags &= ~DRAW_LEGEND;
}

// Support

static void DisplayLegend(ClientData clientData)
{
  Legend* legendPtr = (Legend*)clientData;

  legendPtr->flags &= ~REDRAW_PENDING;
  if (Tk_IsMapped(legendPtr->graphPtr_->tkwin))
    legendPtr->draw(Tk_WindowId(legendPtr->graphPtr_->tkwin));
}

void Blt_Legend_EventuallyRedraw(Graph* graphPtr) 
{
  Legend* legendPtr = graphPtr->legend;

  if ((graphPtr->tkwin) && !(legendPtr->flags & REDRAW_PENDING)) {
    Tcl_DoWhenIdle(DisplayLegend, legendPtr);
    legendPtr->flags |= REDRAW_PENDING;
  }
}

static void SetLegendOrigin(Legend* legendPtr)
{
  Graph* graphPtr = legendPtr->graphPtr_;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;

  int x =0;
  int y =0;
  int w =0;
  int h =0;
  switch (legendPtr->site_) {
  case LEGEND_RIGHT:
    w = graphPtr->rightMargin.width - graphPtr->rightMargin.axesOffset;
    h = graphPtr->bottom - graphPtr->top;
    x = graphPtr->right + graphPtr->rightMargin.axesOffset;
    y = graphPtr->top;
    break;

  case LEGEND_LEFT:
    w = graphPtr->leftMargin.width - graphPtr->leftMargin.axesOffset;
    h = graphPtr->bottom - graphPtr->top;
    x = graphPtr->inset;
    y = graphPtr->top;
    break;

  case LEGEND_TOP:
    w = graphPtr->right - graphPtr->left;
    h = graphPtr->topMargin.height - graphPtr->topMargin.axesOffset;
    if (graphPtr->title) {
      h -= graphPtr->titleHeight;
    }
    x = graphPtr->left;
    y = graphPtr->inset;
    if (graphPtr->title) {
      y += graphPtr->titleHeight;
    }
    break;

  case LEGEND_BOTTOM:
    w = graphPtr->right - graphPtr->left;
    h = graphPtr->bottomMargin.height - graphPtr->bottomMargin.axesOffset;
    x = graphPtr->left;
    y = graphPtr->bottom + graphPtr->bottomMargin.axesOffset;
    break;

  case LEGEND_PLOT:
    w = graphPtr->right - graphPtr->left;
    h = graphPtr->bottom - graphPtr->top;
    x = graphPtr->left;
    y = graphPtr->top;
    break;

  case LEGEND_XY:
    w = legendPtr->width_;
    h = legendPtr->height_;
    x = legendPtr->xReq_;
    y = legendPtr->yReq_;
    if (x < 0) {
      x += graphPtr->width;
    }
    if (y < 0) {
      y += graphPtr->height;
    }
    break;
  }

  switch (ops->anchor) {
  case TK_ANCHOR_NW:			/* Upper left corner */
    break;
  case TK_ANCHOR_W:			/* Left center */
    if (h > legendPtr->height_) {
      y += (h - legendPtr->height_) / 2;
    }
    break;
  case TK_ANCHOR_SW:			/* Lower left corner */
    if (h > legendPtr->height_) {
      y += (h - legendPtr->height_);
    }
    break;
  case TK_ANCHOR_N:			/* Top center */
    if (w > legendPtr->width_) {
      x += (w - legendPtr->width_) / 2;
    }
    break;
  case TK_ANCHOR_CENTER:		/* Center */
    if (h > legendPtr->height_) {
      y += (h - legendPtr->height_) / 2;
    }
    if (w > legendPtr->width_) {
      x += (w - legendPtr->width_) / 2;
    }
    break;
  case TK_ANCHOR_S:			/* Bottom center */
    if (w > legendPtr->width_) {
      x += (w - legendPtr->width_) / 2;
    }
    if (h > legendPtr->height_) {
      y += (h - legendPtr->height_);
    }
    break;
  case TK_ANCHOR_NE:			/* Upper right corner */
    if (w > legendPtr->width_) {
      x += w - legendPtr->width_;
    }
    break;
  case TK_ANCHOR_E:			/* Right center */
    if (w > legendPtr->width_) {
      x += w - legendPtr->width_;
    }
    if (h > legendPtr->height_) {
      y += (h - legendPtr->height_) / 2;
    }
    break;
  case TK_ANCHOR_SE:		/* Lower right corner */
    if (w > legendPtr->width_) {
      x += w - legendPtr->width_;
    }
    if (h > legendPtr->height_) {
      y += (h - legendPtr->height_);
    }
    break;
  }
  legendPtr->x_ = x + ops->xPad;
  legendPtr->y_ = y + ops->yPad;
}

int EntryIsSelected(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry*hPtr = 
    Tcl_FindHashEntry(&legendPtr->selectTable_, (char *)elemPtr);
  return (hPtr != NULL);
}

static void SelectElement(Legend* legendPtr, Element* elemPtr)
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&legendPtr->selectTable_, (char *)elemPtr,&isNew);
  if (isNew) {
    Blt_ChainLink link = Blt_Chain_Append(legendPtr->selected_, elemPtr);
    Tcl_SetHashValue(hPtr, link);
  }
}

void DeselectElement(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry* hPtr = 
    Tcl_FindHashEntry(&legendPtr->selectTable_, (char *)elemPtr);
  if (hPtr) {
    Blt_ChainLink link = (Blt_ChainLink)Tcl_GetHashValue(hPtr);
    Blt_Chain_DeleteLink(legendPtr->selected_, link);
    Tcl_DeleteHashEntry(hPtr);
  }
}

void SelectEntry(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry *hPtr;

  switch (legendPtr->flags & SELECT_TOGGLE) {
  case SELECT_CLEAR:
    DeselectElement(legendPtr, elemPtr);
    break;

  case SELECT_SET:
    SelectElement(legendPtr, elemPtr);
    break;

  case SELECT_TOGGLE:
    hPtr = Tcl_FindHashEntry(&legendPtr->selectTable_, (char *)elemPtr);
    if (hPtr) {
      DeselectElement(legendPtr, elemPtr);
    } else {
      SelectElement(legendPtr, elemPtr);
    }
    break;
  }
}

static ClientData PickEntryProc(ClientData clientData, int x, int y, 
				ClientData *contextPtr)
{
  Graph* graphPtr = (Graph*)clientData;
  Legend* legendPtr = graphPtr->legend;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;

  int w = legendPtr->width_;
  int h = legendPtr->height_;

  if (legendPtr->titleHeight_ > 0)
    y -= legendPtr->titleHeight_ + ops->yPad;

  x -= legendPtr->x_ + ops->borderWidth;
  y -= legendPtr->y_ + ops->borderWidth;
  w -= 2 * ops->borderWidth + 2*ops->xPad;
  h -= 2 * ops->borderWidth + 2*ops->yPad;

  if ((x >= 0) && (x < w) && (y >= 0) && (y < h)) {
    int row, column;
    int n;

    /*
     * It's in the bounding box, so compute the index.
     */
    row    = y / legendPtr->entryHeight_;
    column = x / legendPtr->entryWidth_;
    n = (column * legendPtr->nRows_) + row;
    if (n < legendPtr->nEntries_) {
      Blt_ChainLink link;
      int count;

      /* Legend entries are stored in bottom-to-top. */
      count = 0;
      for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	   link != NULL; link = Blt_Chain_NextLink(link)) {
	Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
	ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
	if (elemOps->label) {
	  if (count == n)
	    return elemPtr;
	  count++;
	}
      }	      
      if (link)
	return Blt_Chain_GetValue(link);
    }
  }
  return NULL;
}

void Blt_LegendToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Legend* legendPtr = graphPtr->legend;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;

  double x, y, yStart;
  int xLabel, xSymbol, ySymbol;
  int count;
  Blt_ChainLink link;
  int symbolSize, xMid, yMid;
  int width, height;
  Tk_FontMetrics fontMetrics;

  if ((ops->hide) || (legendPtr->nEntries_ == 0)) {
    return;
  }
  SetLegendOrigin(legendPtr);

  x = legendPtr->x_;
  y = legendPtr->y_;
  width = legendPtr->width_ - 2*ops->xPad;
  height = legendPtr->height_ - 2*ops->yPad;

  Blt_Ps_Append(ps, "% Legend\n");
  graphPtr = legendPtr->graphPtr_;
  if (graphPtr->pageSetup->decorations) {
    if (ops->normalBg)
      Blt_Ps_Fill3DRectangle(ps, ops->normalBg, x, y, width, height, 
			     ops->borderWidth, ops->relief);
    else
      Blt_Ps_Draw3DRectangle(ps, graphPtr->normalBg, x, y, width, height, 
			     ops->borderWidth, ops->relief);

  } else {
    Blt_Ps_SetClearBackground(ps);
    Blt_Ps_XFillRectangle(ps, x, y, width, height);
  }
  Tk_GetFontMetrics(ops->style.font, &fontMetrics);
  symbolSize = fontMetrics.ascent;
  xMid = symbolSize + 1 + ops->entryBW;
  yMid = (symbolSize / 2) + 1 + ops->entryBW;
  xLabel = 2 * symbolSize + ops->entryBW + ops->ixPad + 5;
  xSymbol = xMid + ops->ixPad;
  ySymbol = yMid + ops->iyPad;

  x += ops->borderWidth;
  y += ops->borderWidth;
  Blt_Ps_DrawText(ps, ops->title, &ops->titleStyle, x, y);
  if (legendPtr->titleHeight_ > 0)
    y += legendPtr->titleHeight_ + ops->yPad;

  count = 0;
  yStart = y;
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    if (elemPtr->flags & LABEL_ACTIVE) {
      Blt_Ts_SetForeground(ops->style, ops->activeFgColor);
      Blt_Ps_Fill3DRectangle(ps, ops->activeBg, x, y, legendPtr->entryWidth_, 
			     legendPtr->entryHeight_, ops->entryBW, 
			     ops->activeRelief);
    } else {
      Blt_Ts_SetForeground(ops->style, ops->fgColor);
      if (elemOps->legendRelief != TK_RELIEF_FLAT) {
	Blt_Ps_Draw3DRectangle(ps, graphPtr->normalBg, x, y, 
			       legendPtr->entryWidth_,
			       legendPtr->entryHeight_, ops->entryBW, 
			       elemOps->legendRelief);
      }
    }
    elemPtr->printSymbol(ps, x + xSymbol, y + ySymbol, symbolSize);
    Blt_Ps_DrawText(ps, elemOps->label, &ops->style, 
		    x + xLabel, y + ops->entryBW + ops->iyPad);
    count++;
    if ((count % legendPtr->nRows_) > 0) {
      y += legendPtr->entryHeight_;
    } else {
      x += legendPtr->entryWidth_;
      y = yStart;
    }
  }
}

static Element *GetNextRow(Graph* graphPtr, Element *focusPtr)
{
  Blt_ChainLink link;
  int row, col;

  col = focusPtr->col_;
  row = focusPtr->row_ + 1;
  for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    if ((elemPtr->col_ == col) && (elemPtr->row_ == row))
      return elemPtr;	
  }
  return NULL;
}

static Element *GetNextColumn(Graph* graphPtr, Element *focusPtr)
{
  Blt_ChainLink link;
  int row, col;

  col = focusPtr->col_ + 1;
  row = focusPtr->row_;
  for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    if ((elemPtr->col_ == col) && (elemPtr->row_ == row))
      return elemPtr;
  }
  return NULL;
}

static Element *GetPreviousRow(Graph* graphPtr, Element *focusPtr)
{
  int col = focusPtr->col_;
  int row = focusPtr->row_ - 1;
  for (Blt_ChainLink link = focusPtr->link; link != NULL; 
       link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    if ((elemPtr->col_ == col) && (elemPtr->row_ == row))
      return elemPtr;	
  }
  return NULL;
}

static Element *GetPreviousColumn(Graph* graphPtr, Element *focusPtr)
{
  int col = focusPtr->col_ - 1;
  int row = focusPtr->row_;
  for (Blt_ChainLink link = focusPtr->link; link != NULL;
       link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    if ((elemPtr->col_ == col) && (elemPtr->row_ == row))
      return elemPtr;	
  }
  return NULL;
}

static Element *GetFirstElement(Graph* graphPtr)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements.displayList);link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label)
      return elemPtr;
  }
  return NULL;
}

static Element *GetLastElement(Graph* graphPtr)
{
  for (Blt_ChainLink link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label)
      return elemPtr;
  }
  return NULL;
}

int GetElementFromObj(Graph* graphPtr, Tcl_Obj *objPtr, Element **elemPtrPtr)
{
  Legend* legendPtr = graphPtr->legend;
  Tcl_Interp* interp = graphPtr->interp;
  const char *string = Tcl_GetString(objPtr);
  char c = string[0];
  Element* elemPtr = NULL;

  //  int last = Blt_Chain_GetLength(graphPtr->elements.displayList) - 1;
  if ((c == 'a') && (strcmp(string, "anchor") == 0))
    elemPtr = legendPtr->selAnchorPtr_;
  else if ((c == 'c') && (strcmp(string, "current") == 0))
    elemPtr = (Element *)Blt_GetCurrentItem(legendPtr->bindTable_);
  else if ((c == 'f') && (strcmp(string, "first") == 0))
    elemPtr = GetFirstElement(graphPtr);
  else if ((c == 'f') && (strcmp(string, "focus") == 0))
    elemPtr = legendPtr->focusPtr_;
  else if ((c == 'l') && (strcmp(string, "last") == 0))
    elemPtr = GetLastElement(graphPtr);
  else if ((c == 'e') && (strcmp(string, "end") == 0))
    elemPtr = GetLastElement(graphPtr);
  else if ((c == 'n') && (strcmp(string, "next.row") == 0))
    elemPtr = GetNextRow(graphPtr, legendPtr->focusPtr_);
  else if ((c == 'n') && (strcmp(string, "next.column") == 0))
    elemPtr = GetNextColumn(graphPtr, legendPtr->focusPtr_);
  else if ((c == 'p') && (strcmp(string, "previous.row") == 0))
    elemPtr = GetPreviousRow(graphPtr, legendPtr->focusPtr_);
  else if ((c == 'p') && (strcmp(string, "previous.column") == 0))
    elemPtr = GetPreviousColumn(graphPtr, legendPtr->focusPtr_);
  else if ((c == 's') && (strcmp(string, "sel.first") == 0))
    elemPtr = legendPtr->selFirstPtr_;
  else if ((c == 's') && (strcmp(string, "sel.last") == 0))
    elemPtr = legendPtr->selLastPtr_;
  else if (c == '@') {
    int x, y;

    if (Blt_GetXY(interp, graphPtr->tkwin, string, &x, &y) != TCL_OK)
      return TCL_ERROR;

    elemPtr = (Element *)PickEntryProc(graphPtr, x, y, NULL);
  }
  else {
    if (Blt_GetElement(interp, graphPtr, objPtr, &elemPtr) != TCL_OK)
      return TCL_ERROR;

    if (elemPtr->link == NULL) {
      Tcl_AppendResult(interp, "bad legend index \"", string, "\"",
		       (char *)NULL);
      return TCL_ERROR;
    }
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      elemPtr = NULL;
  }
  *elemPtrPtr = elemPtr;
  return TCL_OK;
}

int SelectRange(Legend* legendPtr, Element *fromPtr, Element *toPtr)
{
  if (Blt_Chain_IsBefore(fromPtr->link, toPtr->link)) {
    for (Blt_ChainLink link = fromPtr->link; link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      SelectEntry(legendPtr, elemPtr);
      if (link == toPtr->link) {
	break;
      }
    }
  } 
  else {
    for (Blt_ChainLink link = fromPtr->link; link != NULL;
	 link = Blt_Chain_PrevLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      SelectEntry(legendPtr, elemPtr);
      if (link == toPtr->link) {
	break;
      }
    }
  }
  return TCL_OK;
}

int Blt_Legend_Site(Graph* graphPtr)
{
  return graphPtr->legend->site_;
}

int Blt_Legend_Width(Graph* graphPtr)
{
  return graphPtr->legend->width_;
}

int Blt_Legend_Height(Graph* graphPtr)
{
  return graphPtr->legend->height_;
}

int Blt_Legend_IsHidden(Graph* graphPtr)
{
  Legend* legendPtr = graphPtr->legend;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;
  return (ops->hide);
}

int Blt_Legend_IsRaised(Graph* graphPtr)
{
  Legend* legendPtr = graphPtr->legend;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;
  return (ops->raised);
}

int Blt_Legend_X(Graph* graphPtr)
{
  return graphPtr->legend->x_;
}

int Blt_Legend_Y(Graph* graphPtr)
{
  return graphPtr->legend->y_;
}

void Blt_Legend_RemoveElement(Graph* graphPtr, Element* elemPtr)
{
  Blt_DeleteBindings(graphPtr->legend->bindTable_, elemPtr);
}

static int SelectionProc(ClientData clientData, int offset, 
			 char *buffer, int maxBytes)
{
  Legend* legendPtr = (Legend*)clientData;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;

  int nBytes;
  Tcl_DString dString;

  if ((ops->exportSelection) == 0) {
    return -1;
  }
  /* Retrieve the names of the selected entries. */
  Tcl_DStringInit(&dString);
  if (legendPtr->flags & SELECT_SORTED) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(legendPtr->selected_); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      Tcl_DStringAppend(&dString, elemPtr->name(), -1);
      Tcl_DStringAppend(&dString, "\n", -1);
    }
  }
  else {
    Graph* graphPtr = legendPtr->graphPtr_;
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      if (EntryIsSelected(legendPtr, elemPtr)) {
	Tcl_DStringAppend(&dString, elemPtr->name(), -1);
	Tcl_DStringAppend(&dString, "\n", -1);
      }
    }
  }
  nBytes = Tcl_DStringLength(&dString) - offset;
  strncpy(buffer, Tcl_DStringValue(&dString) + offset, maxBytes);
  Tcl_DStringFree(&dString);
  buffer[maxBytes] = '\0';
  return MIN(nBytes, maxBytes);
}

