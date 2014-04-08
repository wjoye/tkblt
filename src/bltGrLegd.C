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
    legendPtr->site = LEGEND_RIGHT;
  else if ((c == 'l') && (strncmp(string, "leftmargin", length) == 0))
    legendPtr->site = LEGEND_LEFT;
  else if ((c == 'r') && (strncmp(string, "rightmargin", length) == 0))
    legendPtr->site = LEGEND_RIGHT;
  else if ((c == 't') && (strncmp(string, "topmargin", length) == 0))
    legendPtr->site = LEGEND_TOP;
  else if ((c == 'b') && (strncmp(string, "bottommargin", length) == 0))
    legendPtr->site = LEGEND_BOTTOM;
  else if ((c == 'p') && (strncmp(string, "plotarea", length) == 0))
    legendPtr->site = LEGEND_PLOT;
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
    legendPtr->xReq = x;
    legendPtr->yReq = y;
    legendPtr->site = LEGEND_XY;
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

  switch (legendPtr->site) {
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

      sprintf_s(string, 200, "@%d,%d", legendPtr->xReq, legendPtr->yReq);
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
  nEntries =0;
  nColumns =0;
  nRows =0;
  width =0;
  height =0;
  entryWidth =0;
  entryHeight =0;
  site =0;
  xReq = -SHRT_MAX;
  yReq = -SHRT_MAX;
  x =0;
  y =0;
  maxSymSize =0;
  bindTable =NULL;
  focusGC =NULL;
  focus =0;
  cursorX =0;
  cursorY =0;
  cursorWidth =0;
  cursorHeight =0;
  focusPtr =NULL;
  selAnchorPtr =NULL;
  selMarkPtr =NULL;
  selFirstPtr =NULL;
  selLastPtr =NULL;
  active =0;
  cursorOn =0;
  onTime = 600;
  offTime = 300;
  timerToken =NULL;
  selected = Blt_Chain_Create();
  titleWidth =0;
  titleHeight =0;

  Blt_Ts_InitStyle(ops->style);
  Blt_Ts_InitStyle(ops->titleStyle);
  ops->style.justify = TK_JUSTIFY_LEFT;
  ops->style.anchor = TK_ANCHOR_NW;
  ops->titleStyle.justify = TK_JUSTIFY_LEFT;
  ops->titleStyle.anchor = TK_ANCHOR_NW;

  bindTable = Blt_CreateBindingTable(graphPtr->interp, 
				     graphPtr->tkwin, graphPtr, 
				     PickEntryProc, Blt_GraphTags);

  Tcl_InitHashTable(&selectTable, TCL_ONE_WORD_KEYS);

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
  Blt_DestroyBindingTable(bindTable);
    
  if (focusGC)
    Blt_FreePrivateGC(graphPtr_->display, focusGC);

  if (timerToken)
    Tcl_DeleteTimerHandler(timerToken);

  if (graphPtr_->tkwin)
    Tk_DeleteSelHandler(graphPtr_->tkwin, XA_PRIMARY, XA_STRING);

  Blt_Chain_Destroy(selected);

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
  if (focusGC)
    Blt_FreePrivateGC(graphPtr_->display, focusGC);

  focusGC = newGC;
}

// Support

static void DisplayLegend(ClientData clientData)
{
  Legend* legendPtr = (Legend*)clientData;

  legendPtr->flags &= ~REDRAW_PENDING;
  if (Tk_IsMapped(legendPtr->graphPtr_->tkwin))
    Blt_DrawLegend(legendPtr->graphPtr_, 
		   Tk_WindowId(legendPtr->graphPtr_->tkwin));
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
  switch (legendPtr->site) {
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
    w = legendPtr->width;
    h = legendPtr->height;
    x = legendPtr->xReq;
    y = legendPtr->yReq;
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
    if (h > legendPtr->height) {
      y += (h - legendPtr->height) / 2;
    }
    break;
  case TK_ANCHOR_SW:			/* Lower left corner */
    if (h > legendPtr->height) {
      y += (h - legendPtr->height);
    }
    break;
  case TK_ANCHOR_N:			/* Top center */
    if (w > legendPtr->width) {
      x += (w - legendPtr->width) / 2;
    }
    break;
  case TK_ANCHOR_CENTER:		/* Center */
    if (h > legendPtr->height) {
      y += (h - legendPtr->height) / 2;
    }
    if (w > legendPtr->width) {
      x += (w - legendPtr->width) / 2;
    }
    break;
  case TK_ANCHOR_S:			/* Bottom center */
    if (w > legendPtr->width) {
      x += (w - legendPtr->width) / 2;
    }
    if (h > legendPtr->height) {
      y += (h - legendPtr->height);
    }
    break;
  case TK_ANCHOR_NE:			/* Upper right corner */
    if (w > legendPtr->width) {
      x += w - legendPtr->width;
    }
    break;
  case TK_ANCHOR_E:			/* Right center */
    if (w > legendPtr->width) {
      x += w - legendPtr->width;
    }
    if (h > legendPtr->height) {
      y += (h - legendPtr->height) / 2;
    }
    break;
  case TK_ANCHOR_SE:		/* Lower right corner */
    if (w > legendPtr->width) {
      x += w - legendPtr->width;
    }
    if (h > legendPtr->height) {
      y += (h - legendPtr->height);
    }
    break;
  }
  legendPtr->x = x + ops->xPad;
  legendPtr->y = y + ops->yPad;
}

int EntryIsSelected(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry*hPtr = 
    Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
  return (hPtr != NULL);
}

static void SelectElement(Legend* legendPtr, Element* elemPtr)
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&legendPtr->selectTable, (char *)elemPtr,&isNew);
  if (isNew) {
    Blt_ChainLink link = Blt_Chain_Append(legendPtr->selected, elemPtr);
    Tcl_SetHashValue(hPtr, link);
  }
}

void DeselectElement(Legend* legendPtr, Element* elemPtr)
{
  Tcl_HashEntry* hPtr = 
    Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
  if (hPtr) {
    Blt_ChainLink link = (Blt_ChainLink)Tcl_GetHashValue(hPtr);
    Blt_Chain_DeleteLink(legendPtr->selected, link);
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
    hPtr = Tcl_FindHashEntry(&legendPtr->selectTable, (char *)elemPtr);
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

  int w = legendPtr->width;
  int h = legendPtr->height;

  if (legendPtr->titleHeight > 0)
    y -= legendPtr->titleHeight + ops->yPad;

  x -= legendPtr->x + ops->borderWidth;
  y -= legendPtr->y + ops->borderWidth;
  w -= 2 * ops->borderWidth + 2*ops->xPad;
  h -= 2 * ops->borderWidth + 2*ops->yPad;

  if ((x >= 0) && (x < w) && (y >= 0) && (y < h)) {
    int row, column;
    int n;

    /*
     * It's in the bounding box, so compute the index.
     */
    row    = y / legendPtr->entryHeight;
    column = x / legendPtr->entryWidth;
    n = (column * legendPtr->nRows) + row;
    if (n < legendPtr->nEntries) {
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

void Blt_MapLegend(Graph* graphPtr, int plotWidth, int plotHeight)
{
  Legend* legendPtr = graphPtr->legend;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;

  Blt_ChainLink link;
  int nRows, nColumns, nEntries;
  int lw, lh;
  int maxWidth, maxHeight;
  int symbolWidth;
  Tk_FontMetrics fontMetrics;

  /* Initialize legend values to default (no legend displayed) */
  legendPtr->entryWidth = legendPtr->entryHeight = 0;
  legendPtr->nRows = legendPtr->nColumns = legendPtr->nEntries = 0;
  legendPtr->height = legendPtr->width = 0;

  Blt_Ts_GetExtents(&ops->titleStyle, ops->title, 
		    &legendPtr->titleWidth, &legendPtr->titleHeight);
  /*   
   * Count the number of legend entries and determine the widest and tallest
   * label.  The number of entries would normally be the number of elements,
   * but elements can have no legend entry (-label "").
   */
  nEntries = 0;
  maxWidth = maxHeight = 0;
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
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

  Tk_GetFontMetrics(ops->style.font, &fontMetrics);
  symbolWidth = 2 * fontMetrics.ascent;

  maxWidth += 2 * ops->entryBW + 2*ops->ixPad +
    + symbolWidth + 3 * LABEL_PAD;

  maxHeight += 2 * ops->entryBW + 2*ops->iyPad;

  maxWidth |= 0x01;
  maxHeight |= 0x01;

  lw = plotWidth - 2 * ops->borderWidth - 2*ops->xPad;
  lh = plotHeight - 2 * ops->borderWidth - 2*ops->yPad;

  /*
   * The number of rows and columns is computed as one of the following:
   *
   *	both options set		User defined. 
   *  -rows				Compute columns from rows.
   *  -columns			Compute rows from columns.
   *	neither set			Compute rows and columns from
   *					size of plot.  
   */
  if (ops->reqRows > 0) {
    nRows = MIN(ops->reqRows, nEntries); 
    if (ops->reqColumns > 0) {
      nColumns = MIN(ops->reqColumns, nEntries);
    } else {
      nColumns = ((nEntries - 1) / nRows) + 1; /* Only -rows. */
    }
  } else if (ops->reqColumns > 0) { /* Only -columns. */
    nColumns = MIN(ops->reqColumns, nEntries);
    nRows = ((nEntries - 1) / nColumns) + 1;
  } else {			
    /* Compute # of rows and columns from the legend size. */
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
    switch (legendPtr->site) {
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
  if (nColumns < 1) {
    nColumns = 1;
  } 
  if (nRows < 1) {
    nRows = 1;
  }

  lh = (nRows * maxHeight);
  if (legendPtr->titleHeight > 0) {
    lh += legendPtr->titleHeight + ops->yPad;
  }
  lw = nColumns * maxWidth;
  if (lw < (int)(legendPtr->titleWidth)) {
    lw = legendPtr->titleWidth;
  }
  legendPtr->width = lw + 2 * ops->borderWidth + 
    2*ops->xPad;
  legendPtr->height = lh + 2 * ops->borderWidth + 
    2*ops->yPad;
  legendPtr->nRows = nRows;
  legendPtr->nColumns = nColumns;
  legendPtr->nEntries = nEntries;
  legendPtr->entryHeight = maxHeight;
  legendPtr->entryWidth = maxWidth;

  {
    int row, col, count;

    row = col = count = 0;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
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

void Blt_DrawLegend(Graph* graphPtr, Drawable drawable)
{
  Blt_ChainLink link;
  Tk_FontMetrics fontMetrics;
  Legend* legendPtr = graphPtr->legend;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops_;

  Pixmap pixmap;
  Tk_Window tkwin;
  int count;
  int symbolSize, xMid, yMid;
  int x, y, w, h;
  int xLabel, yStart, xSymbol, ySymbol;

  if ((ops->hide) || (legendPtr->nEntries == 0)) {
    return;
  }

  SetLegendOrigin(legendPtr);
  graphPtr = legendPtr->graphPtr_;
  tkwin = graphPtr->tkwin;
  w = legendPtr->width;
  h = legendPtr->height;

  pixmap = Tk_GetPixmap(graphPtr->display, Tk_WindowId(tkwin), w, h, 
			Tk_Depth(tkwin));

  if (ops->normalBg) {
    Tk_Fill3DRectangle(tkwin, pixmap, ops->normalBg, 0, 0, 
		       w, h, 0, TK_RELIEF_FLAT);
  }
  else if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
    /* 
     * Legend background is transparent and is positioned over the the
     * plot area.  Either copy the part of the background from the backing
     * store pixmap or (if no backing store exists) just fill it with the
     * background color of the plot.
     */
    if (graphPtr->cache != None) {
      XCopyArea(graphPtr->display, graphPtr->cache, pixmap, 
		graphPtr->drawGC, legendPtr->x, legendPtr->y, w, h, 0, 0);
    } else {
      Tk_Fill3DRectangle(tkwin, pixmap, graphPtr->plotBg, 0, 0, 
			 w, h, TK_RELIEF_FLAT, 0);
    }
  }
  else {
    /* 
     * The legend is located in one of the margins or the external window.
     */
    Tk_Fill3DRectangle(tkwin, pixmap, graphPtr->normalBg, 0, 0, 
		       w, h, 0, TK_RELIEF_FLAT);
  }
  Tk_GetFontMetrics(ops->style.font, &fontMetrics);

  symbolSize = fontMetrics.ascent;
  xMid = symbolSize + 1 + ops->entryBW;
  yMid = (symbolSize / 2) + 1 + ops->entryBW;
  xLabel = 2 * symbolSize + ops->entryBW +  ops->ixPad + 2 * LABEL_PAD;
  ySymbol = yMid + ops->iyPad; 
  xSymbol = xMid + LABEL_PAD;

  x = ops->xPad + ops->borderWidth;
  y = ops->yPad + ops->borderWidth;
  Blt_DrawText(tkwin, pixmap, ops->title, &ops->titleStyle, x, y);
  if (legendPtr->titleHeight > 0) {
    y += legendPtr->titleHeight + ops->yPad;
  }
  count = 0;
  yStart = y;
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    int isSelected;

    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    ElementOptions* elemOps = (ElementOptions*)elemPtr->ops();
    if (elemOps->label == NULL)
      continue;

    isSelected = EntryIsSelected(legendPtr, elemPtr);
    if (elemPtr->flags & LABEL_ACTIVE) {
      Tk_Fill3DRectangle(tkwin, pixmap, ops->activeBg, 
			 x, y, legendPtr->entryWidth, legendPtr->entryHeight, 
			 ops->entryBW, ops->activeRelief);
    } else if (isSelected) {
      XColor* fg = (legendPtr->flags & FOCUS) ?
	ops->selInFocusFgColor : ops->selOutFocusFgColor;
      Tk_3DBorder bg = (legendPtr->flags & FOCUS) ?
	ops->selInFocusBg : ops->selOutFocusBg;
      Blt_Ts_SetForeground(ops->style, fg);
      Tk_Fill3DRectangle(tkwin, pixmap, bg, x, y, 
			 legendPtr->entryWidth, legendPtr->entryHeight, 
			 ops->selBW, ops->selRelief);
    } else {
      Blt_Ts_SetForeground(ops->style, ops->fgColor);
      if (elemOps->legendRelief != TK_RELIEF_FLAT) {
	Tk_Fill3DRectangle(tkwin, pixmap, graphPtr->normalBg, 
			   x, y, legendPtr->entryWidth, 
			   legendPtr->entryHeight, ops->entryBW, 
			   elemOps->legendRelief);
      }
    }
    elemPtr->drawSymbol(pixmap, x + xSymbol, y + ySymbol, symbolSize);
    Blt_DrawText(tkwin, pixmap, elemOps->label, &ops->style, 
		 x + xLabel, 
		 y + ops->entryBW + ops->iyPad);
    count++;
    if (legendPtr->focusPtr == elemPtr) { /* Focus outline */
      if (isSelected) {
	XColor* color;

	color = (legendPtr->flags & FOCUS) ?
	  ops->selInFocusFgColor :
	  ops->selOutFocusFgColor;
	XSetForeground(graphPtr->display, legendPtr->focusGC, 
		       color->pixel);
      }
      XDrawRectangle(graphPtr->display, pixmap, legendPtr->focusGC, 
		     x + 1, y + 1, legendPtr->entryWidth - 3, 
		     legendPtr->entryHeight - 3);
      if (isSelected) {
	XSetForeground(graphPtr->display, legendPtr->focusGC, 
		       ops->focusColor->pixel);
      }
    }
    /* Check when to move to the next column */
    if ((count % legendPtr->nRows) > 0) {
      y += legendPtr->entryHeight;
    } else {
      x += legendPtr->entryWidth;
      y = yStart;
    }
  }
  /*
   * Draw the border and/or background of the legend.
   */
  Tk_3DBorder bg = ops->normalBg;
  if (bg == NULL)
    bg = graphPtr->normalBg;

  /* Disable crosshairs before redisplaying to the screen */
  if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
    Blt_DisableCrosshairs(graphPtr);
  }
  Tk_Draw3DRectangle(tkwin, pixmap, bg, 0, 0, w, h, 
		     ops->borderWidth, ops->relief);
  XCopyArea(graphPtr->display, pixmap, drawable, graphPtr->drawGC, 0, 0, w, h,
	    legendPtr->x, legendPtr->y);
  if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
    Blt_EnableCrosshairs(graphPtr);
  }
  Tk_FreePixmap(graphPtr->display, pixmap);
  graphPtr->flags &= ~DRAW_LEGEND;
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

  if ((ops->hide) || (legendPtr->nEntries == 0)) {
    return;
  }
  SetLegendOrigin(legendPtr);

  x = legendPtr->x, y = legendPtr->y;
  width = legendPtr->width - 2*ops->xPad;
  height = legendPtr->height - 2*ops->yPad;

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
  if (legendPtr->titleHeight > 0) {
    y += legendPtr->titleHeight + ops->yPad;
  }

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
      Blt_Ps_Fill3DRectangle(ps, ops->activeBg, x, y, legendPtr->entryWidth, 
			     legendPtr->entryHeight, ops->entryBW, 
			     ops->activeRelief);
    } else {
      Blt_Ts_SetForeground(ops->style, ops->fgColor);
      if (elemOps->legendRelief != TK_RELIEF_FLAT) {
	Blt_Ps_Draw3DRectangle(ps, graphPtr->normalBg, x, y, 
			       legendPtr->entryWidth,
			       legendPtr->entryHeight, ops->entryBW, 
			       elemOps->legendRelief);
      }
    }
    elemPtr->printSymbol(ps, x + xSymbol, y + ySymbol, symbolSize);
    Blt_Ps_DrawText(ps, elemOps->label, &ops->style, 
		    x + xLabel, y + ops->entryBW + ops->iyPad);
    count++;
    if ((count % legendPtr->nRows) > 0) {
      y += legendPtr->entryHeight;
    } else {
      x += legendPtr->entryWidth;
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
    elemPtr = legendPtr->selAnchorPtr;
  else if ((c == 'c') && (strcmp(string, "current") == 0))
    elemPtr = (Element *)Blt_GetCurrentItem(legendPtr->bindTable);
  else if ((c == 'f') && (strcmp(string, "first") == 0))
    elemPtr = GetFirstElement(graphPtr);
  else if ((c == 'f') && (strcmp(string, "focus") == 0))
    elemPtr = legendPtr->focusPtr;
  else if ((c == 'l') && (strcmp(string, "last") == 0))
    elemPtr = GetLastElement(graphPtr);
  else if ((c == 'e') && (strcmp(string, "end") == 0))
    elemPtr = GetLastElement(graphPtr);
  else if ((c == 'n') && (strcmp(string, "next.row") == 0))
    elemPtr = GetNextRow(graphPtr, legendPtr->focusPtr);
  else if ((c == 'n') && (strcmp(string, "next.column") == 0))
    elemPtr = GetNextColumn(graphPtr, legendPtr->focusPtr);
  else if ((c == 'p') && (strcmp(string, "previous.row") == 0))
    elemPtr = GetPreviousRow(graphPtr, legendPtr->focusPtr);
  else if ((c == 'p') && (strcmp(string, "previous.column") == 0))
    elemPtr = GetPreviousColumn(graphPtr, legendPtr->focusPtr);
  else if ((c == 's') && (strcmp(string, "sel.first") == 0))
    elemPtr = legendPtr->selFirstPtr;
  else if ((c == 's') && (strcmp(string, "sel.last") == 0))
    elemPtr = legendPtr->selLastPtr;
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
  return graphPtr->legend->site;
}

int Blt_Legend_Width(Graph* graphPtr)
{
  return graphPtr->legend->width;
}

int Blt_Legend_Height(Graph* graphPtr)
{
  return graphPtr->legend->height;
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
  return graphPtr->legend->x;
}

int Blt_Legend_Y(Graph* graphPtr)
{
  return graphPtr->legend->y;
}

void Blt_Legend_RemoveElement(Graph* graphPtr, Element* elemPtr)
{
  Blt_DeleteBindings(graphPtr->legend->bindTable, elemPtr);
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

    for (link = Blt_Chain_FirstLink(legendPtr->selected); 
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

