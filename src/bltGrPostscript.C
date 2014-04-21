/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1991-2004 George A Howlett.
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
};

#include "bltGrPageSetup.h"
#include "bltConfig.h"

static Tk_OptionSpec optionSpecs[] = {
  {TK_OPTION_BOOLEAN, "-center", "center", "Center", 
   "yes", -1, Tk_Offset(PageSetup, center), 0, NULL, 0},
  {TK_OPTION_CUSTOM, "-comments", "comments", "Comments",
   NULL, -1, Tk_Offset(PageSetup, comments), 
   TK_OPTION_NULL_OK, &listObjOption, 0},
  {TK_OPTION_BOOLEAN, "-decorations", "decorations", "Decorations",
   "no", -1, Tk_Offset(PageSetup, decorations), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-footer", "footer", "Footer",
   "no", -1, Tk_Offset(PageSetup, footer), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-greyscale", "greyscale", "Greyscale",
   "no", -1, Tk_Offset(PageSetup, greyscale), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-height", "height", "Height", 
   "0", -1, Tk_Offset(PageSetup, reqHeight), 0, NULL, 0},
  {TK_OPTION_BOOLEAN, "-landscape", "landscape", "Landscape",
   "no", -1, Tk_Offset(PageSetup, landscape), 0, NULL, 0},
  {TK_OPTION_INT, "-level", "level", "Level", 
   "2", -1, Tk_Offset(PageSetup, level), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-padx", "padX", "PadX", 
   "1.0i", -1, Tk_Offset(PageSetup, xPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-pady", "padY", "PadY", 
   "1.0i", -1, Tk_Offset(PageSetup, yPad), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-paperheight", "paperHeight", "PaperHeight",
   "11.0i", -1, Tk_Offset(PageSetup, reqPaperHeight), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-paperwidth", "paperWidth", "PaperWidth",
   "8.5i", -1, Tk_Offset(PageSetup, reqPaperWidth), 0, NULL, 0},
  {TK_OPTION_PIXELS, "-width", "width", "Width", 
   "0", -1, Tk_Offset(PageSetup, reqWidth), 0, NULL, 0},
  {TK_OPTION_END, NULL, NULL, NULL, NULL, -1, 0, 0, NULL, 0}
};

int Blt_CreatePageSetup(Graph* graphPtr)
{
  PageSetup* setupPtr = (PageSetup*)calloc(1, sizeof(PageSetup));
  graphPtr->pageSetup_ = setupPtr;

  setupPtr->optionTable =Tk_CreateOptionTable(graphPtr->interp_, optionSpecs);
  return Tk_InitOptions(graphPtr->interp_, (char*)setupPtr, 
			setupPtr->optionTable, graphPtr->tkwin_);
}

void Blt_DestroyPageSetup(Graph* graphPtr)
{
  PageSetup* setupPtr = graphPtr->pageSetup_;
  Tk_FreeConfigOptions((char*)setupPtr, setupPtr->optionTable, graphPtr->tkwin_);
  free(setupPtr);
}

int PageSetupObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			  int objc, Tcl_Obj* const objv[])
{
  PageSetup* setupPtr = graphPtr->pageSetup_;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)setupPtr, setupPtr->optionTable, 
			objc, objv, graphPtr->tkwin_, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    break; 
  }

  if (!error) {
    Tk_FreeSavedOptions(&savedOptions);
    return TCL_OK;
  }
  else {
    Tcl_SetObjResult(interp, errorResult);
    Tcl_DecrRefCount(errorResult);
    return TCL_ERROR;
  }
}

