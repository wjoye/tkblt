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

#include "bltGrElem.h"
#include "bltGrElemBar.h"
#include "bltGrElemLine.h"
#include "bltGrElemOp.h"

// Defs

extern int Blt_GetPenFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			     Tcl_Obj *objPtr, ClassId classId, Pen **penPtrPtr);

static Tcl_Obj *DisplayListObj(Graph* graphPtr);
static void DestroyElement(Element* elemPtr);
static int ElementObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			       Element* elemPtr, 
			       int objc, Tcl_Obj* const objv[]);
static void FreeElement(char* data);
static int GetIndex(Tcl_Interp* interp, Element* elemPtr, 
		    Tcl_Obj *objPtr, int *indexPtr);
typedef int (GraphElementProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			       Tcl_Obj *const *objv);

// Create

static int CreateElement(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			 Tcl_Obj *const *objv, ClassId classId)
{
  char *name = Tcl_GetString(objv[3]);
  if (name[0] == '-') {
    Tcl_AppendResult(graphPtr->interp, "name of element \"", name, 
		     "\" can't start with a '-'", NULL);
    return TCL_ERROR;
  }

  int isNew;
  Tcl_HashEntry* hPtr = 
    Tcl_CreateHashEntry(&graphPtr->elements.table, name, &isNew);
  if (!isNew) {
    Tcl_AppendResult(interp, "element \"", name, 
		     "\" already exists in \"", Tcl_GetString(objv[0]), 
		     "\"", NULL);
    return TCL_ERROR;
  }

  Element* elemPtr;
  switch (classId) {
  case CID_ELEM_BAR:
    elemPtr = new BarElement(graphPtr,name,hPtr);
    break;
  case CID_ELEM_LINE:
    elemPtr = new LineElement(graphPtr,name,hPtr);
    break;
  default:
    return TCL_ERROR;
  }
  if (!elemPtr)
    return TCL_ERROR;

  Tcl_SetHashValue(hPtr, elemPtr);

  if ((Tk_InitOptions(graphPtr->interp, (char*)elemPtr->ops(), elemPtr->optionTable(), graphPtr->tkwin) != TCL_OK) || (ElementObjConfigure(interp, graphPtr, elemPtr, objc-4, objv+4) != TCL_OK)) {
    DestroyElement(elemPtr);
    return TCL_ERROR;
  }

  elemPtr->link = Blt_Chain_Append(graphPtr->elements.displayList, elemPtr);

  return TCL_OK;
}

static void DestroyElement(Element* elemPtr)
{
  Graph* graphPtr = elemPtr->obj.graphPtr;

  Blt_DeleteBindings(graphPtr->bindTable, elemPtr);
  Blt_Legend_RemoveElement(graphPtr, elemPtr);

  if (elemPtr->link)
    Blt_Chain_DeleteLink(graphPtr->elements.displayList, elemPtr->link);

  delete elemPtr;
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
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)elemPtr, 
				      elemPtr->optionTable(),
				      objv[4], graphPtr->tkwin);
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
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  if (objc <= 5) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, (char*)elemPtr, 
				       elemPtr->optionTable(), 
				       (objc == 5) ? objv[4] : NULL, 
				       graphPtr->tkwin);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return ElementObjConfigure(interp, graphPtr, elemPtr, objc-4, objv+4);
}

static int ElementObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			       Element* elemPtr, 
			       int objc, Tcl_Obj* const objv[])
{
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)elemPtr->ops(), elemPtr->optionTable(), 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
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

// Ops

static int ActivateOp(Graph* graphPtr, Tcl_Interp* interp,
		      int objc, Tcl_Obj* const objv[])
{
  // List all the currently active elements
  if (objc == 3) {
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      if (elemPtr->flags & ACTIVE)
	Tcl_ListObjAppendElement(interp, listObjPtr,
				 Tcl_NewStringObj(elemPtr->obj.name, -1));
    }

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }

  Element* elemPtr;
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK)
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
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static int BindOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements.tagTable, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      char *tagName = (char*)Tcl_GetHashKey(&graphPtr->elements.tagTable, hPtr);
      Tcl_ListObjAppendElement(interp, listObjPtr, 
			       Tcl_NewStringObj(tagName, -1));
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }

  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}

static int ClosestOp(Graph* graphPtr, Tcl_Interp* interp,
		     int objc, Tcl_Obj* const objv[])
{
  ClosestSearch* searchPtr = &graphPtr->search;

  if (graphPtr->flags & RESET_AXES)
    Blt_ResetAxes(graphPtr);

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
      if (Blt_GetElement(interp, graphPtr, objv[ii], &elemPtr) != TCL_OK)
	return TCL_ERROR;

      if (elemPtr && !elemPtr->hide_ && 
	  !(elemPtr->flags & (MAP_ITEM|DELETE_PENDING)))
	elemPtr->closest();
    }
  }
  else {
    // Find the closest point from the set of displayed elements,
    // searching the display list from back to front.  That way if
    // the points from two different elements overlay each other
    // exactly, the last one picked will be the topmost.  

    for (Blt_ChainLink link=Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      if (elemPtr && !elemPtr->hide_ && 
	  !(elemPtr->flags & (MAP_ITEM|DELETE_PENDING)))
	elemPtr->closest();
    }
  }

  if (searchPtr->dist < (double)searchPtr->halo) {
    Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("name", -1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(searchPtr->elemPtr->obj.name, -1)); 
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
		    int objc, Tcl_Obj* const objv[], ClassId classId)
{
  if (CreateElement(graphPtr, interp, objc, objv, classId) != TCL_OK)
    return TCL_ERROR;
  Tcl_SetObjResult(interp, objv[3]);

  return TCL_OK;
}

static int DeactivateOp(Graph* graphPtr, Tcl_Interp* interp,
			int objc, Tcl_Obj* const objv[])
{
  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;
    if (Blt_GetElement(interp, graphPtr, objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    if (elemPtr->activeIndices_) {
      free(elemPtr->activeIndices_);
      elemPtr->activeIndices_ = NULL;
    }
    elemPtr->nActiveIndices_ = 0;
    elemPtr->flags &= ~(ACTIVE | ACTIVE_PENDING);
  }

  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static int DeleteOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;
    if (Blt_GetElement(interp, graphPtr, objv[ii], &elemPtr) != TCL_OK) {
      Tcl_AppendResult(interp, "can't find element \"", 
		       Tcl_GetString(objv[ii]), "\" in \"", 
		       Tk_PathName(graphPtr->tkwin), "\"", NULL);
      return TCL_ERROR;
    }
    elemPtr->flags |= DELETE_PENDING;
    Tcl_EventuallyFree(elemPtr, FreeElement);
  }

  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static int ExistsOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry *hPtr = 
    Tcl_FindHashEntry(&graphPtr->elements.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp,
		 int objc, Tcl_Obj* const objv[])
{
  char *string = Tcl_GetString(objv[3]);
  if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
    Element* elemPtr = (Element*)Blt_GetCurrentItem(graphPtr->bindTable);
    /* Report only on elements. */
    if ((elemPtr) && ((elemPtr->flags & DELETE_PENDING) == 0) &&
	(elemPtr->obj.classId >= CID_ELEM_BAR) &&
	(elemPtr->obj.classId <= CID_ELEM_LINE)) {
      Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name,-1);
    }
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
    if (Blt_GetElement(interp, graphPtr, objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;
    Blt_Chain_UnlinkLink(graphPtr->elements.displayList, elemPtr->link); 
    Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
  }

  // Append the links to end of the display list
  Blt_ChainLink link, next;
  for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
    next = Blt_Chain_NextLink(link);
    Blt_Chain_UnlinkLink(chain, link); 
    Blt_Chain_LinkAfter(graphPtr->elements.displayList, link, NULL); 
  }	
  Blt_Chain_Destroy(chain);

  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);

  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  return TCL_OK;
}

static int NamesOp(Graph* graphPtr, Tcl_Interp* interp,
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

  if (objc == 3) {
    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
  }
  else {
    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter); hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);

      for (int ii=3; ii<objc; ii++) {
	if (Tcl_StringMatch(elemPtr->obj.name,Tcl_GetString(objv[ii]))) {
	  Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
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
    if (Blt_GetElement(interp, graphPtr, objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    Blt_Chain_UnlinkLink(graphPtr->elements.displayList, elemPtr->link); 
    Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
  }

  // Prepend the links to beginning of the display list in reverse order
  Blt_ChainLink link, prev;
  for (link = Blt_Chain_LastLink(chain); link != NULL; link = prev) {
    prev = Blt_Chain_PrevLink(link);
    Blt_Chain_UnlinkLink(chain, link); 
    Blt_Chain_LinkBefore(graphPtr->elements.displayList, link, NULL); 
  }	
  Blt_Chain_Destroy(chain);

  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);

  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  return TCL_OK;
}

static int ShowOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  int n;
  Tcl_Obj **elem;
  if (Tcl_ListObjGetElements(interp, objv[3], &n, &elem) != TCL_OK)
    return TCL_ERROR;

  // Collect the named elements into a list
  Blt_Chain chain = Blt_Chain_Create();
  for (int ii=0; ii<n; ii++) {
    Element* elemPtr;
    if (Blt_GetElement(interp, graphPtr, elem[ii], &elemPtr) != TCL_OK) {
      Blt_Chain_Destroy(chain);
      return TCL_ERROR;
    }
    Blt_Chain_Append(chain, elemPtr);
  }

  // Clear the links from the currently displayed elements
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->link = NULL;
  }
  Blt_Chain_Destroy(graphPtr->elements.displayList);
  graphPtr->elements.displayList = chain;

  // Set links on all the displayed elements
  for (Blt_ChainLink link = Blt_Chain_FirstLink(chain); link != NULL; 
       link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->link = link;
  }

  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);

  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  return TCL_OK;
}

static int TypeOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  Element* elemPtr;
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  switch (elemPtr->obj.classId) {
  case CID_ELEM_BAR:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "bar", -1);
    return TCL_OK;
  case CID_ELEM_LINE:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "line", -1);
    return TCL_OK;
  default:
    return TCL_ERROR;
  }
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

int Blt_ElementOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[], ClassId classId)
{
  void *ptr = Blt_GetOpFromObj(interp, numElemOps, elemOps, BLT_OP_ARG2, 
			       objc, objv, 0);
  if (!ptr)
    return TCL_ERROR;

  if (ptr == CreateOp)
    return CreateOp(graphPtr, interp, objc, objv, classId);
  else {
    GraphElementProc* proc = (GraphElementProc*)ptr;
    return (*proc)(graphPtr, interp, objc, objv);
  }
}

// Support

static Tcl_Obj *DisplayListObj(Graph* graphPtr)
{
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->elements.displayList); link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
    Tcl_ListObjAppendElement(graphPtr->interp, listObjPtr, objPtr);
  }

  return listObjPtr;
}

static void FreeElement(char* data)
{
  Element* elemPtr = (Element *)data;
  DestroyElement(elemPtr);
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

int Blt_GetElement(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
		   Element **elemPtrPtr)
{
  Tcl_HashEntry *hPtr;
  char *name;

  name = Tcl_GetString(objPtr);
  if (!name || !name[0])
    return TCL_ERROR;
  hPtr = Tcl_FindHashEntry(&graphPtr->elements.table, name);
  if (!hPtr) {
    if (interp)
      Tcl_AppendResult(interp, "can't find element \"", name,
		       "\" in \"", Tk_PathName(graphPtr->tkwin), "\"", 
		       NULL);
    return TCL_ERROR;
  }
  *elemPtrPtr = (Element*)Tcl_GetHashValue(hPtr);
  return TCL_OK;
}

void Blt_DestroyElements(Graph* graphPtr)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch iter;

  for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
    Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
    if (elemPtr) {
      elemPtr->hashPtr = NULL;
      DestroyElement(elemPtr);
    }
  }
  Tcl_DeleteHashTable(&graphPtr->elements.table);
  Tcl_DeleteHashTable(&graphPtr->elements.tagTable);
  Blt_Chain_Destroy(graphPtr->elements.displayList);
}

void Blt_ConfigureElements(Graph* graphPtr)
{
  for (Blt_ChainLink link =Blt_Chain_FirstLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    elemPtr->configure();
  }
}

void Blt_MapElements(Graph* graphPtr)
{
  if (graphPtr->barMode != BARS_INFRONT)
    Blt_ResetBarGroups(graphPtr);

  for (Blt_ChainLink link =Blt_Chain_FirstLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!elemPtr->link || (elemPtr->flags & DELETE_PENDING))
      continue;

    if ((graphPtr->flags & MAP_ALL) || (elemPtr->flags & MAP_ITEM)) {
      elemPtr->map();
      elemPtr->flags &= ~MAP_ITEM;
    }
  }
}

void Blt_DrawElements(Graph* graphPtr, Drawable drawable)
{
  Blt_ChainLink link;

  /* Draw with respect to the stacking order. */
  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!(elemPtr->flags & DELETE_PENDING) && !elemPtr->hide_)
      elemPtr->drawNormal(drawable);
  }
}

void Blt_DrawActiveElements(Graph* graphPtr, Drawable drawable)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!(elemPtr->flags & DELETE_PENDING) && (elemPtr->flags & ACTIVE) && 
	!elemPtr->hide_)
      elemPtr->drawActive(drawable);
  }
}

void Blt_ElementsToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!(elemPtr->flags & DELETE_PENDING) && !elemPtr->hide_) {
      continue;
    }
    /* Comment the PostScript to indicate the start of the element */
    Blt_Ps_Format(ps, "\n%% Element \"%s\"\n\n", elemPtr->obj.name);
    elemPtr->printNormal(ps);
  }
}

void Blt_ActiveElementsToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!(elemPtr->flags & DELETE_PENDING) && 
	(elemPtr->flags & ACTIVE) && 
	!elemPtr->hide_) {
      Blt_Ps_Format(ps, "\n%% Active Element \"%s\"\n\n", 
		    elemPtr->obj.name);
      elemPtr->printActive(ps);
    }
  }
}

ClientData Blt_MakeElementTag(Graph* graphPtr, const char *tagName)
{
  Tcl_HashEntry *hPtr;
  int isNew;

  hPtr = Tcl_CreateHashEntry(&graphPtr->elements.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->elements.tagTable, hPtr);
}
