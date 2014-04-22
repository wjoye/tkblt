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
#include "bltInt.h"
#include "bltOp.h"
#include "bltBind.h"
};

#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrAxisOp.h"
#include "bltGrElem.h"
#include "bltGrElemBar.h"
#include "bltGrElemLine.h"
#include "bltGrElemOp.h"
#include "bltGrLegd.h"

using namespace Blt;

static int GetIndex(Tcl_Interp* interp, Element* elemPtr, 
		    Tcl_Obj *objPtr, int *indexPtr);
static Tcl_Obj *DisplayListObj(Graph* graphPtr);

int ElementObjConfigure(Tcl_Interp* interp, Element* elemPtr,
			int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = elemPtr->graphPtr_;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)elemPtr->ops(), elemPtr->optionTable(), 
			objc, objv, graphPtr->tkwin_, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    elemPtr->flags |= mask;
    elemPtr->flags |= MAP_ITEM;
    graphPtr->flags |= RESET_WORLD | CACHE_DIRTY;
    if (elemPtr->configure() != TCL_OK)
      return TCL_ERROR;
    graphPtr->eventuallyRedraw();

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

// Configure

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc != 5) {
    Tcl_WrongNumArgs(interp, 3, objv, "cget option");
    return TCL_ERROR;
  }

  Element* elemPtr;
  if (graphPtr->getElement(objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)elemPtr->ops(), 
				      elemPtr->optionTable(),
				      objv[4], graphPtr->tkwin_);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp,
		       int objc, Tcl_Obj* const objv[])
{
  Element* elemPtr;
  if (graphPtr->getElement(objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  if (objc <= 5) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(interp, (char*)elemPtr->ops(), 
				       elemPtr->optionTable(), 
				       (objc == 5) ? objv[4] : NULL, 
				       graphPtr->tkwin_);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return ElementObjConfigure(interp, elemPtr, objc-4, objv+4);
}

// Ops

static int ActivateOp(Graph* graphPtr, Tcl_Interp* interp,
		      int objc, Tcl_Obj* const objv[])
{
  // List all the currently active elements
  if (objc == 3) {
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements_.table, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      if (elemPtr->flags & ACTIVE)
	Tcl_ListObjAppendElement(interp, listObjPtr,
				 Tcl_NewStringObj(elemPtr->name_, -1));
    }

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }

  Element* elemPtr;
  if (graphPtr->getElement(objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  int* indices = NULL;
  int nIndices = -1;
  if (objc > 4) {
    nIndices = objc - 4;
    indices = (int*)malloc(sizeof(int) * nIndices);

    int *activePtr = indices;
    for (int ii=4; ii<objc; ii++) {
      if (GetIndex(interp, elemPtr, objv[ii], activePtr) != TCL_OK)
	return TCL_ERROR;
      activePtr++;
    }
  }

  if (elemPtr->activeIndices_)
    free(elemPtr->activeIndices_);
  elemPtr->nActiveIndices_ = nIndices;
  elemPtr->activeIndices_ = indices;

  elemPtr->flags |= ACTIVE | ACTIVE_PENDING;
  graphPtr->eventuallyRedraw();

  return TCL_OK;
}

static int BindOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements_.tagTable, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      char *tagName = (char*)Tcl_GetHashKey(&graphPtr->elements_.tagTable, hPtr);
      Tcl_ListObjAppendElement(interp, listObjPtr, 
			       Tcl_NewStringObj(tagName, -1));
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }

  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable_, graphPtr->elementTag(Tcl_GetString(objv[3])), objc - 4, objv + 4);
}

static int ClosestOp(Graph* graphPtr, Tcl_Interp* interp,
		     int objc, Tcl_Obj* const objv[])
{
  GraphOptions* gops = (GraphOptions*)graphPtr->ops_;
  ClosestSearch* searchPtr = &gops->search;

  if (graphPtr->flags & RESET_AXES)
    graphPtr->resetAxes();

  int x;
  if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
    Tcl_AppendResult(interp, ": bad window x-coordinate", NULL);
    return TCL_ERROR;
  }
  int y;
  if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
    Tcl_AppendResult(interp, ": bad window y-coordinate", NULL);
    return TCL_ERROR;
  }

  searchPtr->x = x;
  searchPtr->y = y;
  searchPtr->index = -1;
  searchPtr->dist = (double)(searchPtr->halo + 1);

  if (objc>5) {
    for (int ii=5; ii<objc; ii++) {
      Element* elemPtr;
      if (graphPtr->getElement(objv[ii], &elemPtr) != TCL_OK)
	return TCL_ERROR;

      if (elemPtr && !elemPtr->hide_ && !(elemPtr->flags & MAP_ITEM))
	elemPtr->closest();
    }
  }
  else {
    // Find the closest point from the set of displayed elements,
    // searching the display list from back to front.  That way if
    // the points from two different elements overlay each other
    // exactly, the last one picked will be the topmost.  

    for (Blt_ChainLink link=Blt_Chain_LastLink(graphPtr->elements_.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      if (elemPtr && !elemPtr->hide_ && !(elemPtr->flags & MAP_ITEM))
	elemPtr->closest();
    }
  }

  if (searchPtr->dist < (double)searchPtr->halo) {
    Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("name", -1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(searchPtr->elemPtr->name_, -1)); 
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("index", -1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(searchPtr->index));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("x", -1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(searchPtr->point.x));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("y", -1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(searchPtr->point.y));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("dist", -1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(searchPtr->dist));
    Tcl_SetObjResult(interp, listObjPtr);
  }

  return TCL_OK;
}

static int CreateOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  if (graphPtr->createElement(objc, objv) != TCL_OK)
    return TCL_ERROR;
  Tcl_SetObjResult(interp, objv[3]);

  return TCL_OK;
}

static int DeactivateOp(Graph* graphPtr, Tcl_Interp* interp,
			int objc, Tcl_Obj* const objv[])
{
  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;
    if (graphPtr->getElement(objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    if (elemPtr->activeIndices_) {
      free(elemPtr->activeIndices_);
      elemPtr->activeIndices_ = NULL;
    }
    elemPtr->nActiveIndices_ = 0;
    elemPtr->flags &= ~(ACTIVE | ACTIVE_PENDING);
  }
  graphPtr->eventuallyRedraw();

  return TCL_OK;
}

static int DeleteOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;
    if (graphPtr->getElement(objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;
    graphPtr->destroyElement(elemPtr);
  }

  graphPtr->flags |= RESET_WORLD;
  graphPtr->eventuallyRedraw();

  return TCL_OK;
}

static int ExistsOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry *hPtr = 
    Tcl_FindHashEntry(&graphPtr->elements_.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp,
		 int objc, Tcl_Obj* const objv[])
{
  char *string = Tcl_GetString(objv[3]);
  if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
    Element* elemPtr = (Element*)Blt_GetCurrentItem(graphPtr->bindTable_);
    if (elemPtr)
      Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->name_,-1);
  }
  return TCL_OK;
}

static int LowerOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  // Move the links of lowered elements out of the display list into
  // a temporary list
  Blt_Chain chain = Blt_Chain_Create();

  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;
    if (graphPtr->getElement(objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    Blt_Chain_UnlinkLink(graphPtr->elements_.displayList, elemPtr->link); 
    Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
  }

  // Append the links to end of the display list
  Blt_ChainLink link, next;
  for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
    next = Blt_Chain_NextLink(link);
    Blt_Chain_UnlinkLink(chain, link); 
    Blt_Chain_LinkAfter(graphPtr->elements_.displayList, link, NULL); 
  }	
  Blt_Chain_Destroy(chain);

  graphPtr->flags |= RESET_WORLD;
  graphPtr->eventuallyRedraw();

  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  return TCL_OK;
}

static int NamesOp(Graph* graphPtr, Tcl_Interp* interp,
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

  if (objc == 3) {
    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements_.table, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->name_, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
  }
  else {
    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements_.table, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);

      for (int ii=3; ii<objc; ii++) {
	if (Tcl_StringMatch(elemPtr->name_,Tcl_GetString(objv[ii]))) {
	  Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->name_, -1);
	  Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	  break;
	}
      }
    }
  }

  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int RaiseOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Blt_Chain chain = Blt_Chain_Create();

  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;
    if (graphPtr->getElement(objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    Blt_Chain_UnlinkLink(graphPtr->elements_.displayList, elemPtr->link); 
    Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
  }

  // Prepend the links to beginning of the display list in reverse order
  Blt_ChainLink link, prev;
  for (link = Blt_Chain_LastLink(chain); link != NULL; link = prev) {
    prev = Blt_Chain_PrevLink(link);
    Blt_Chain_UnlinkLink(chain, link); 
    Blt_Chain_LinkBefore(graphPtr->elements_.displayList, link, NULL); 
  }	
  Blt_Chain_Destroy(chain);

  graphPtr->flags |= RESET_WORLD;
  graphPtr->eventuallyRedraw();

  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  return TCL_OK;
}

static int ShowOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  int elemObjc;
  Tcl_Obj** elemObjv;
  if (Tcl_ListObjGetElements(interp, objv[3], &elemObjc, &elemObjv) != TCL_OK)
    return TCL_ERROR;

  // Collect the named elements into a list
  Blt_Chain chain = Blt_Chain_Create();
  for (int ii=0; ii<elemObjc; ii++) {
    Element* elemPtr;
    if (graphPtr->getElement(elemObjv[ii], &elemPtr) != TCL_OK) {
      Blt_Chain_Destroy(chain);
      return TCL_ERROR;
    }

    Blt_Chain_Append(chain, elemPtr);
  }

  // Clear the links from the currently displayed elements
  for (Blt_ChainLink link=Blt_Chain_FirstLink(graphPtr->elements_.displayList);
       link; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->link = NULL;
  }
  Blt_Chain_Destroy(graphPtr->elements_.displayList);
  graphPtr->elements_.displayList = chain;

  // Set links on all the displayed elements
  for (Blt_ChainLink link = Blt_Chain_FirstLink(chain); link; 
       link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->link = link;
  }

  graphPtr->flags |= RESET_WORLD;
  graphPtr->eventuallyRedraw();

  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  return TCL_OK;
}

static int TypeOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  Element* elemPtr;
  if (graphPtr->getElement(objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->typeName(), -1);
  return TCL_OK;
}

static Blt_OpSpec elemOps[] = {
  {"activate",   1, (void*)ActivateOp,   3, 0, "?elemName? ?index...?",},
  {"bind",       1, (void*)BindOp,       3, 6, "elemName sequence command",},
  {"cget",       2, (void*)CgetOp,       5, 5, "elemName option",},
  {"closest",    2, (void*)ClosestOp,    5, 0, "x y ?elemName?...",},
  {"configure",  2, (void*)ConfigureOp,  4, 0, "elemName ?option value?...",},
  {"create",     2, (void*)CreateOp,     4, 0, "elemName ?option value?...",},
  {"deactivate", 3, (void*)DeactivateOp, 3, 0, "?elemName?...",},
  {"delete",     3, (void*)DeleteOp,     3, 0, "?elemName?...",},
  {"exists",     1, (void*)ExistsOp,     4, 4, "elemName",},
  {"get",        1, (void*)GetOp,        4, 4, "current",},
  {"lower",      1, (void*)LowerOp,      3, 0, "?elemName?...",},
  {"names",      1, (void*)NamesOp,      3, 0, "?pattern?...",},
  {"raise",      1, (void*)RaiseOp,      3, 0, "?elemName?...",},
  {"show",       1, (void*)ShowOp,       4, 4, "elemList",},
  {"type",       1, (void*)TypeOp,       4, 4, "elemName",},
};
static int numElemOps = sizeof(elemOps) / sizeof(Blt_OpSpec);

typedef int (GraphElementProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			       Tcl_Obj *const *objv);

int Blt_ElementOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  void *ptr = Blt_GetOpFromObj(interp, numElemOps, elemOps, BLT_OP_ARG2, 
			       objc, objv, 0);
  if (!ptr)
    return TCL_ERROR;

  if (ptr == CreateOp)
    return CreateOp(graphPtr, interp, objc, objv);
  else {
    GraphElementProc* proc = (GraphElementProc*)ptr;
    return (*proc)(graphPtr, interp, objc, objv);
  }
}

// Support

static Tcl_Obj *DisplayListObj(Graph* graphPtr)
{
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements_.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->name_, -1);
    Tcl_ListObjAppendElement(graphPtr->interp_, listObjPtr, objPtr);
  }

  return listObjPtr;
}

static int GetIndex(Tcl_Interp* interp, Element* elemPtr, 
		    Tcl_Obj *objPtr, int *indexPtr)
{
  ElementOptions* ops = (ElementOptions*)elemPtr->ops();

  char *string = Tcl_GetString(objPtr);
  if ((*string == 'e') && (strcmp("end", string) == 0))
    *indexPtr = NUMBEROFPOINTS(ops);
  else if (Blt_ExprIntFromObj(interp, objPtr, indexPtr) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}


