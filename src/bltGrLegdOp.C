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
#include "bltGrElem.h"

static Tk_LostSelProc LostSelectionProc;
static int SelectionOp(Graph* graphPtr, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[]);

static int LegendObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			      int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)legendPtr->ops(),
			legendPtr->optionTable(), 
			objc, objv, graphPtr->tkwin_, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    graphPtr->flags |= mask;
    graphPtr->flags |= (RESET_WORLD | CACHE_DIRTY);
    legendPtr->configure();
    Blt_EventuallyRedrawGraph(graphPtr);

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

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  Legend* legendPtr = graphPtr->legend_;
  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)legendPtr->ops(), 
				      legendPtr->optionTable(),
				      objv[3], graphPtr->tkwin_);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp,
		       int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(interp, (char*)legendPtr->ops(), 
				       legendPtr->optionTable(), 
				       (objc == 4) ? objv[3] : NULL, 
				       graphPtr->tkwin_);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return LegendObjConfigure(interp, graphPtr, objc-3, objv+3);
}

// Ops

static int ActivateOp(Graph* graphPtr, Tcl_Interp* interp, 
		      int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops();

  unsigned int active, redraw;
  const char *string;
  int i;

  string = Tcl_GetString(objv[2]);
  active = (string[0] == 'a') ? LABEL_ACTIVE : 0;
  redraw = 0;
  for (i = 3; i < objc; i++) {
    Blt_ChainLink link;
    const char *pattern;

    pattern = Tcl_GetString(objv[i]);
    for (link = Blt_Chain_FirstLink(graphPtr->elements_.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      if (Tcl_StringMatch(elemPtr->name(), pattern)) {
	fprintf(stderr, "legend %s(%s) %s is currently %d\n",
		string, pattern, elemPtr->name(), 
		(elemPtr->flags & LABEL_ACTIVE));
	if (active) {
	  if ((elemPtr->flags & LABEL_ACTIVE) == 0) {
	    elemPtr->flags |= LABEL_ACTIVE;
	    redraw = 1;
	  }
	} else {
	  if (elemPtr->flags & LABEL_ACTIVE) {
	    elemPtr->flags &= ~LABEL_ACTIVE;
	    redraw = 1;
	  }
	}
	fprintf(stderr, "legend %s(%s) %s is now %d\n",
		string, pattern, elemPtr->name(), 
		(elemPtr->flags & LABEL_ACTIVE));
      }
    }
  }
  if ((redraw) && ((ops->hide) == 0)) {
    /*
     * See if how much we need to draw. If the graph is already scheduled
     * for a redraw, just make sure the right flags are set.  Otherwise
     * redraw only the legend: it's either in an external window or it's
     * the only thing that need updating.
     */
    if (graphPtr->flags & REDRAW_PENDING) {
      graphPtr->flags |= CACHE_DIRTY;
      graphPtr->flags |= REDRAW_WORLD; /* Redraw entire graph. */
    }
  }
  {
    Blt_ChainLink link;
    Tcl_Obj *listObjPtr;
	
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    /* List active elements in stacking order. */
    for (link = Blt_Chain_FirstLink(graphPtr->elements_.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      if (elemPtr->flags & LABEL_ACTIVE) {
	Tcl_Obj *objPtr;

	objPtr = Tcl_NewStringObj(elemPtr->name(), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
      }
    }
    Tcl_SetObjResult(interp, listObjPtr);
  }
  return TCL_OK;
}

static int BindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements_.tagTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      const char *tagName = 
	(const char*)Tcl_GetHashKey(&graphPtr->elements_.tagTable, hPtr);
      Tcl_Obj *objPtr = Tcl_NewStringObj(tagName, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  return Blt_ConfigureBindingsFromObj(interp, graphPtr->legend_->bindTable_, Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}

static int CurselectionOp(Graph* graphPtr, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (legendPtr->flags & SELECT_SORTED) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(legendPtr->selected_); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->name(), -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
  }
  else {
    // List of selected entries is in stacking order
    for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements_.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);

      if (legendPtr->entryIsSelected(elemPtr)) {
	Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->name(), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
      }
    }
  }
  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int FocusOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;

  legendPtr->focusPtr_ = NULL;
  if (objc == 4) {
    Element* elemPtr;
    if (legendPtr->getElementFromObj(objv[3], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    if (elemPtr)
      legendPtr->focusPtr_ = elemPtr;
  }

  Blt_SetFocusItem(legendPtr->bindTable_,legendPtr->focusPtr_,CID_LEGEND_ENTRY);
  graphPtr->legend_->eventuallyRedraw();

  if (legendPtr->focusPtr_)
    Tcl_SetStringObj(Tcl_GetObjResult(interp),legendPtr->focusPtr_->name(),-1);

  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops();

  if (((ops->hide) == 0) && (legendPtr->nEntries_ > 0)) {
    Element* elemPtr;

    if (legendPtr->getElementFromObj(objv[3], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    if (elemPtr)
      Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->name(),-1);
  }
  return TCL_OK;
}

static Blt_OpSpec legendOps[] =
  {
    {"activate",     1, (void*)ActivateOp,      3, 0, "?pattern?...",},
    {"bind",         1, (void*)BindOp,          3, 6, "elem sequence command",},
    {"cget",         2, (void*)CgetOp,          4, 4, "option",},
    {"configure",    2, (void*)ConfigureOp,     3, 0, "?option value?...",},
    {"curselection", 2, (void*)CurselectionOp,  3, 3, "",},
    {"deactivate",   1, (void*)ActivateOp,      3, 0, "?pattern?...",},
    {"focus",        1, (void*)FocusOp,         3, 4, "?elem?",},
    {"get",          1, (void*)GetOp,           4, 4, "elem",},
    {"selection",    1, (void*)SelectionOp,     3, 0, "args"},
  };
static int nLegendOps = sizeof(legendOps) / sizeof(Blt_OpSpec);

typedef int (GraphLegendProc)(Graph* graphPtr, Tcl_Interp* interp, 
			      int objc, Tcl_Obj* const objv[]);

int Blt_LegendOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  GraphLegendProc *proc = (GraphLegendProc*)Blt_GetOpFromObj(interp, nLegendOps, legendOps, BLT_OP_ARG2, objc, objv,0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(graphPtr, interp, objc, objv);
}

// Selection Widget Ops

static int SelectionAnchorOp(Graph* graphPtr, Tcl_Interp* interp, 
			     int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  Element* elemPtr;

  if (legendPtr->getElementFromObj(objv[4], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  // Set both the anchor and the mark. Indicates that a single entry
  // is selected
  legendPtr->selAnchorPtr_ = elemPtr;
  legendPtr->selMarkPtr_ = NULL;
  if (elemPtr)
    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->name(), -1);

  graphPtr->legend_->eventuallyRedraw();

  return TCL_OK;
}

static int SelectionClearallOp(Graph* graphPtr, Tcl_Interp* interp, 
			       int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  legendPtr->clearSelection();
  legendPtr->eventuallyRedraw();

  return TCL_OK;
}

static int SelectionIncludesOp(Graph* graphPtr, Tcl_Interp* interp, 
			       int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  Element* elemPtr;
  if (legendPtr->getElementFromObj(objv[4], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  int boo = legendPtr->entryIsSelected(elemPtr);
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), boo);
  return TCL_OK;
}

static int SelectionMarkOp(Graph* graphPtr, Tcl_Interp* interp, 
			   int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops();
  Element* elemPtr;

  if (legendPtr->getElementFromObj(objv[4], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  if (legendPtr->selAnchorPtr_ == NULL) {
    Tcl_AppendResult(interp, "selection anchor must be set first", NULL);
    return TCL_ERROR;
  }

  if (legendPtr->selMarkPtr_ != elemPtr) {
    // Deselect entry from the list all the way back to the anchor
    Blt_ChainLink link, next;
    for (link = Blt_Chain_LastLink(legendPtr->selected_); link != NULL; 
	 link = next) {
      next = Blt_Chain_PrevLink(link);
      Element *selectPtr = (Element*)Blt_Chain_GetValue(link);
      if (selectPtr == legendPtr->selAnchorPtr_)
	break;

      legendPtr->deselectElement(selectPtr);
    }

    legendPtr->flags &= ~SELECT_TOGGLE;
    legendPtr->flags |= SELECT_SET;
    legendPtr->selectRange(legendPtr->selAnchorPtr_, elemPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->name(), -1);
    legendPtr->selMarkPtr_ = elemPtr;

    graphPtr->legend_->eventuallyRedraw();

    if (ops->selectCmd)
      legendPtr->eventuallyInvokeSelectCmd();
  }
  return TCL_OK;
}

static int SelectionPresentOp(Graph* graphPtr, Tcl_Interp* interp, 
			      int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  int boo = (Blt_Chain_GetLength(legendPtr->selected_) > 0);
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), boo);
  return TCL_OK;
}

static int SelectionSetOp(Graph* graphPtr, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  Legend* legendPtr = graphPtr->legend_;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops();

  legendPtr->flags &= ~SELECT_TOGGLE;
  const char* string = Tcl_GetString(objv[3]);
  switch (string[0]) {
  case 's':
    legendPtr->flags |= SELECT_SET;
    break;
  case 'c':
    legendPtr->flags |= SELECT_CLEAR;
    break;
  case 't':
    legendPtr->flags |= SELECT_TOGGLE;
    break;
  }

  Element *firstPtr;
  if (legendPtr->getElementFromObj(objv[4], &firstPtr) != TCL_OK)
    return TCL_ERROR;

  if ((firstPtr->hide()) && ((legendPtr->flags & SELECT_CLEAR)==0)) {
    Tcl_AppendResult(interp, "can't select hidden node \"", 
		     Tcl_GetString(objv[4]), "\"", (char *)NULL);
    return TCL_ERROR;
  }

  Element* lastPtr = firstPtr;
  if (objc > 5) {
    if (legendPtr->getElementFromObj(objv[5], &lastPtr) != TCL_OK)
      return TCL_ERROR;

    if (lastPtr->hide() && ((legendPtr->flags & SELECT_CLEAR) == 0)) {
      Tcl_AppendResult(interp, "can't select hidden node \"", 
		       Tcl_GetString(objv[5]), "\"", (char *)NULL);
      return TCL_ERROR;
    }
  }

  if (firstPtr == lastPtr)
    legendPtr->selectEntry(firstPtr);
  else
    legendPtr->selectRange(firstPtr, lastPtr);

  // Set both the anchor and the mark. Indicates that a single entry is
  // selected
  if (legendPtr->selAnchorPtr_ == NULL)
    legendPtr->selAnchorPtr_ = firstPtr;

  if (ops->exportSelection)
    Tk_OwnSelection(graphPtr->tkwin_, XA_PRIMARY, LostSelectionProc, legendPtr);

  graphPtr->legend_->eventuallyRedraw();

  if (ops->selectCmd)
    legendPtr->eventuallyInvokeSelectCmd();

  return TCL_OK;
}

static Blt_OpSpec selectionOps[] =
  {
    {"anchor",   1, (void*)SelectionAnchorOp,   5, 5, "elem",},
    {"clear",    5, (void*)SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"clearall", 6, (void*)SelectionClearallOp, 4, 4, "",},
    {"includes", 1, (void*)SelectionIncludesOp, 5, 5, "elem",},
    {"mark",     1, (void*)SelectionMarkOp,     5, 5, "elem",},
    {"present",  1, (void*)SelectionPresentOp,  4, 4, "",},
    {"set",      1, (void*)SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"toggle",   1, (void*)SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
  };
static int nSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int SelectionOp(Graph* graphPtr, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  GraphLegendProc* proc = (GraphLegendProc*)Blt_GetOpFromObj(interp, nSelectionOps, selectionOps, BLT_OP_ARG3, objc, objv, 0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(graphPtr, interp, objc, objv);
}

// Support

static void LostSelectionProc(ClientData clientData)
{
  Legend* legendPtr = (Legend*)clientData;
  LegendOptions* ops = (LegendOptions*)legendPtr->ops();

  if (ops->exportSelection)
    legendPtr->clearSelection();
  legendPtr->eventuallyRedraw();
}




