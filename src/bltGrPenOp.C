/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1996-2004 George A Howlett.
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

#include "bltGrPen.h"
#include "bltGrPenOp.h"
#include "bltGrPenLine.h"
#include "bltGrPenBar.h"

// Defs

static int GetPenFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			 Pen **penPtrPtr);
static int PenObjConfigure(Tcl_Interp* interp, Graph* graphPtr, Pen* penPtr, 
			   int objc, Tcl_Obj* const objv[]);
typedef int (GraphPenProc)(Tcl_Interp* interp, Graph* graphPtr, int objc, 
			   Tcl_Obj* const objv[]);

// Create

int Blt_CreatePen(Graph* graphPtr, Tcl_Interp* interp, 
		  const char* penName, ClassId classId,
		  int objc, Tcl_Obj* const objv[])
{
  int isNew;
  Tcl_HashEntry *hPtr = 
    Tcl_CreateHashEntry(&graphPtr->penTable, penName, &isNew);
  if (!isNew) {
    Tcl_AppendResult(interp, "pen \"", penName, 
		     "\" already exists in \"", Tk_PathName(graphPtr->tkwin),
		     "\"", (char *)NULL);
    return TCL_ERROR;
  }

  Pen* penPtr;
  switch (classId) {
  case CID_ELEM_BAR:
    penPtr = new BarPen(graphPtr, penName, hPtr);
    break;
  case CID_ELEM_LINE:
    penPtr = new LinePen(graphPtr, penName, hPtr);
    break;
  default:
    return TCL_ERROR;
  }
  if (!penPtr)
    return TCL_ERROR;

  Tcl_SetHashValue(hPtr, penPtr);

  if ((Tk_InitOptions(graphPtr->interp, (char*)penPtr->ops(), penPtr->optionTable(), graphPtr->tkwin) != TCL_OK) || (PenObjConfigure(interp, graphPtr, penPtr, objc-4, objv+4) != TCL_OK)) {
    delete penPtr;
    return TCL_ERROR;
  }

  graphPtr->flags |= CACHE_DIRTY;
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

// Configure

static int CgetOp(Tcl_Interp* interp, Graph* graphPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc != 5) {
    Tcl_WrongNumArgs(interp, 3, objv, "cget option");
    return TCL_ERROR;
  }

  Pen* penPtr;
  if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK)
    return TCL_ERROR;


  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)penPtr->ops(), 
				      penPtr->optionTable(),
				      objv[4], graphPtr->tkwin);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Tcl_Interp* interp, Graph* graphPtr, 
		       int objc, Tcl_Obj* const objv[])
{
  Pen* penPtr;
  if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK)
    return TCL_ERROR;

  if (objc <= 5) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, 
				       (char*)penPtr->ops(), 
				       penPtr->optionTable(), 
				       (objc == 5) ? objv[4] : NULL, 
				       graphPtr->tkwin);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return PenObjConfigure(interp, graphPtr, penPtr, objc-4, objv+4);
}

static int PenObjConfigure(Tcl_Interp* interp, Graph* graphPtr, Pen* penPtr, 
			   int objc, Tcl_Obj* const objv[])
{
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)penPtr->ops(), penPtr->optionTable(), 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    graphPtr->flags |= mask;
    graphPtr->flags |= CACHE_DIRTY;
    if (penPtr->configure() != TCL_OK)
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

static int CreateOp(Tcl_Interp* interp, Graph* graphPtr, 
		    int objc, Tcl_Obj* const objv[])
{
  if (Blt_CreatePen(graphPtr, interp, Tcl_GetString(objv[3]),
		    graphPtr->classId, objc, objv) != TCL_OK)
    return TCL_ERROR;
  Tcl_SetObjResult(interp, objv[3]);

  return TCL_OK;
}

static int DeleteOp(Tcl_Interp* interp, Graph* graphPtr, 
		    int objc, Tcl_Obj* const objv[])
{
  if (objc<4)
    return TCL_ERROR;
    
  Pen* penPtr;
  if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK) {
    Tcl_AppendResult(interp, "can't find pen \"", 
		     Tcl_GetString(objv[3]), "\" in \"", 
		     Tk_PathName(graphPtr->tkwin), "\"", NULL);
    return TCL_ERROR;
  }

  penPtr->flags |= DELETE_PENDING;
  if (penPtr->refCount == 0)
    delete penPtr;

  return TCL_OK;
}

static int NamesOp(Tcl_Interp* interp, Graph* graphPtr, 
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->penTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Pen* penPtr = (Pen*)Tcl_GetHashValue(hPtr);
      if ((penPtr->flags & DELETE_PENDING) == 0)
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewStringObj(penPtr->name(), -1));
    }
  } 
  else {
    Tcl_HashSearch iter;
    for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->penTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Pen* penPtr = (Pen*)Tcl_GetHashValue(hPtr);
      if ((penPtr->flags & DELETE_PENDING) == 0) {
	for (int ii=3; ii<objc; ii++) {
	  char *pattern = Tcl_GetString(objv[ii]);
	  if (Tcl_StringMatch(penPtr->name(), pattern)) {
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
				     Tcl_NewStringObj(penPtr->name(), -1));
	    break;
	  }
	}
      }
    }
  }
  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int TypeOp(Tcl_Interp* interp, Graph* graphPtr, 
		  int objc, Tcl_Obj* const objv[])
{
  Pen* penPtr;
  if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK)
    return TCL_ERROR;

  Tcl_SetStringObj(Tcl_GetObjResult(interp), penPtr->typeName(), -1);
  return TCL_OK;
}

static Blt_OpSpec penOps[] =
  {
    {"cget", 2, (void*)CgetOp, 5, 5, "penName option",},
    {"configure", 2, (void*)ConfigureOp, 4, 0, "penName ?penName?... ?option value?...",},
    {"create", 2, (void*)CreateOp, 4, 0, "penName ?option value?...",},
    {"delete", 2, (void*)DeleteOp, 3, 0, "?penName?...",},
    {"names", 1, (void*)NamesOp, 3, 0, "?pattern?...",},
    {"type", 1, (void*)TypeOp, 4, 4, "penName",},
  };
static int nPenOps = sizeof(penOps) / sizeof(Blt_OpSpec);

int Blt_PenOp(Graph* graphPtr, Tcl_Interp* interp, 
	      int objc, Tcl_Obj* const objv[])
{
  GraphPenProc *proc = (GraphPenProc*)Blt_GetOpFromObj(interp, nPenOps, penOps, BLT_OP_ARG2, objc, objv, 0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc)(interp, graphPtr, objc, objv);
}

// Support

void Blt_DestroyPens(Graph* graphPtr)
{
  Tcl_HashSearch iter;
  for (Tcl_HashEntry *hPtr = Tcl_FirstHashEntry(&graphPtr->penTable, &iter);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
    Pen* penPtr = (Pen*)Tcl_GetHashValue(hPtr);
    delete penPtr;
  }
  Tcl_DeleteHashTable(&graphPtr->penTable);
}

void Blt_FreePen(Pen* penPtr)
{
  if (penPtr != NULL) {
    penPtr->refCount--;
    if ((penPtr->refCount == 0) && (penPtr->flags & DELETE_PENDING))
      delete penPtr;
  }
}

int Blt_GetPenFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr,
		      ClassId classId, Pen **penPtrPtr)
{
  Pen* penPtr = NULL;
  const char *name = Tcl_GetString(objPtr);
  Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&graphPtr->penTable, name);
  if (hPtr != NULL) {
    penPtr = (Pen*)Tcl_GetHashValue(hPtr);
    if (penPtr->flags & DELETE_PENDING)
      penPtr = NULL;
  }

  if (!penPtr) {
    Tcl_AppendResult(interp, "can't find pen \"", name, "\" in \"", 
		     Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
    return TCL_ERROR;
  }

  if (penPtr->classId() != classId) {
    Tcl_AppendResult(interp, "pen \"", name, 
		     "\" is the wrong type (is \"", penPtr->className(),
		     "\"", ", wanted \"", 
		     Blt_GraphClassName(classId), "\")", (char *)NULL);
    return TCL_ERROR;
  }

  penPtr->refCount++;
  *penPtrPtr = penPtr;

  return TCL_OK;
}

// Support

static int GetPenFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr, 
			 Pen **penPtrPtr)
{
  Pen* penPtr = NULL;
  const char *name = Tcl_GetString(objPtr);
  Tcl_HashEntry *hPtr = Tcl_FindHashEntry(&graphPtr->penTable, name);
  if (hPtr != NULL) {
    penPtr = (Pen*)Tcl_GetHashValue(hPtr);
    if (penPtr->flags & DELETE_PENDING)
      penPtr = NULL;
  }
  if (penPtr == NULL) {
    if (interp != NULL) {
      Tcl_AppendResult(interp, "can't find pen \"", name, "\" in \"", 
		       Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
    }
    return TCL_ERROR;
  }
  *penPtrPtr = penPtr;
  return TCL_OK;
}


