/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrPen.c --
 *
 * This module implements pens for the BLT graph widget.
 *
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

#include <assert.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bltInt.h"
#include "bltGraph.h"
#include "bltGrElem.h"
#include "bltOp.h"

static Tk_CustomOptionSetProc PenSetProc;
static Tk_CustomOptionGetProc PenGetProc;
Tk_ObjCustomOption barPenObjOption =
  {
    "barPen", PenSetProc, PenGetProc, NULL, NULL, (ClientData)CID_ELEM_BAR
  };
Tk_ObjCustomOption linePenObjOption =
  {
    "linePen", PenSetProc, PenGetProc, NULL, NULL, (ClientData)CID_ELEM_LINE
  };

static int PenSetProc(ClientData clientData, Tcl_Interp *interp,
		      Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		      int offset, char* save, int flags)
{
  Pen** penPtrPtr = (Pen**)(widgRec + offset);
  const char* string = Tcl_GetString(*objPtr);
  if ((string[0] == '\0') && (flags & TK_OPTION_NULL_OK)) {
    Blt_FreePen(*penPtrPtr);
    *penPtrPtr = NULL;
  } 
  else {
    Pen *penPtr;
    Graph *graphPtr = Blt_GetGraphFromWindowData(tkwin);
    ClassId classId = (ClassId)clientData; /* Element type. */

    if (classId == CID_NONE)
      classId = graphPtr->classId;

    if (Blt_GetPenFromObj(interp, graphPtr, *objPtr, classId, &penPtr) 
	!= TCL_OK)
      return TCL_ERROR;
    
    Blt_FreePen(*penPtrPtr);
    *penPtrPtr = penPtr;
  }

  return TCL_OK;
};

static Tcl_Obj* PenGetProc(ClientData clientData, Tk_Window tkwin, 
			   char *widgRec, int offset)
{
  Pen* penPtr = *(Pen**)(widgRec + offset);

  if (penPtr == NULL)
    return Tcl_NewStringObj("", -1);
  else
    return Tcl_NewStringObj(penPtr->name, -1);
};

static int PenObjConfigure(Tcl_Interp *interp, Graph* graphPtr,
			   Pen* penPtr, 
			   int objc, Tcl_Obj* const objv[]);
typedef int (GraphPenProc)(Tcl_Interp *interp, Graph *graphPtr, int objc, 
			   Tcl_Obj *const *objv);

static int GetPenFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
			 Pen **penPtrPtr)
{
  Tcl_HashEntry *hPtr;
  Pen *penPtr;
  const char *name;

  penPtr = NULL;
  name = Tcl_GetString(objPtr);
  hPtr = Tcl_FindHashEntry(&graphPtr->penTable, name);
  if (hPtr != NULL) {
    penPtr = Tcl_GetHashValue(hPtr);
    if (penPtr->flags & DELETE_PENDING) {
      penPtr = NULL;
    }
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

static void DestroyPen(Pen* penPtr)
{
  Graph *graphPtr = penPtr->graphPtr;

  (*penPtr->destroyProc) (graphPtr, penPtr);
  if ((penPtr->name != NULL) && (penPtr->name[0] != '\0')) {
    free((void*)(penPtr->name));
  }
  if (penPtr->hashPtr != NULL) {
    Tcl_DeleteHashEntry(penPtr->hashPtr);
  }
  free(penPtr);
}

void Blt_FreePen(Pen *penPtr)
{
  if (penPtr != NULL) {
    penPtr->refCount--;
    if ((penPtr->refCount == 0) && (penPtr->flags & DELETE_PENDING)) {
      DestroyPen(penPtr);
    }
  }
}

Pen* Blt_CreatePen(Graph* graphPtr, const char* penName, ClassId classId,
		   int objc, Tcl_Obj* const objv[])
{
  Pen *penPtr;
  Tcl_HashEntry *hPtr;
  int isNew;
  int i;

  /*
   * Scan the option list for a "-type" entry.  This will indicate what type
   * of pen we are creating. Otherwise we'll default to the suggested type.
   * Last -type option wins.
   */
  for (i = 0; i < objc; i += 2) {
    char *string;
    int length;

    string = Tcl_GetStringFromObj(objv[i],  &length);
    if ((length > 2) && (strncmp(string, "-type", length) == 0)) {
      char *arg;

      arg = Tcl_GetString(objv[i + 1]);
      if (strcmp(arg, "bar") == 0) {
	classId = CID_ELEM_BAR;
      } else if (strcmp(arg, "line") == 0) {
	classId = CID_ELEM_LINE;
      } else {
	Tcl_AppendResult(graphPtr->interp, "unknown pen type \"",
			 arg, "\" specified", (char *)NULL);
	return NULL;
      }
    }
  }
  classId = CID_ELEM_LINE;
  hPtr = Tcl_CreateHashEntry(&graphPtr->penTable, penName, &isNew);
  if (!isNew) {
    penPtr = Tcl_GetHashValue(hPtr);
    if ((penPtr->flags & DELETE_PENDING) == 0) {
      Tcl_AppendResult(graphPtr->interp, "pen \"", penName,
		       "\" already exists in \"", Tk_PathName(graphPtr->tkwin), "\"",
		       (char *)NULL);
      return NULL;
    }
    if (penPtr->classId != classId) {
      Tcl_AppendResult(graphPtr->interp, "pen \"", penName,
		       "\" in-use: can't change pen type from \"", 
		       Blt_GraphClassName(penPtr->classId), "\" to \"", 
		       Blt_GraphClassName(classId), "\"", (char *)NULL);
      return NULL;
    }
    penPtr->flags &= ~DELETE_PENDING; /* Undelete the pen. */
  } else {
    if (classId == CID_ELEM_BAR)
      penPtr = Blt_BarPen(graphPtr, penName);
    else
      penPtr = Blt_LinePen(graphPtr, penName);
    penPtr->classId = classId;
    penPtr->hashPtr = hPtr;
    penPtr->graphPtr = graphPtr;
    Tcl_SetHashValue(hPtr, penPtr);
  }

  /*
  if ((Tk_InitOptions(graphPtr->interp, (char*)penPtr, penPtr->optionTable, graphPtr->tkwin) != TCL_OK) || (PenObjConfigure(interp, graphPtr, penPtr, objc-4, objv+4) != TCL_OK)) {
    if (isNew)
      DestroyElement(penPtr);
    return TCL_ERROR;
  }
  */

  //  (*penPtr->configProc) (graphPtr, penPtr);
  return penPtr;
}

int Blt_GetPenFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
		      ClassId classId, Pen **penPtrPtr)
{
  Tcl_HashEntry *hPtr;
  Pen *penPtr;
  const char *name;
    
  penPtr = NULL;
  name = Tcl_GetString(objPtr);
  hPtr = Tcl_FindHashEntry(&graphPtr->penTable, name);
  if (hPtr != NULL) {
    penPtr = Tcl_GetHashValue(hPtr);
    if (penPtr->flags & DELETE_PENDING) {
      penPtr = NULL;
    }
  }
  if (penPtr == NULL) {
    if (interp != NULL) {
      Tcl_AppendResult(interp, "can't find pen \"", name, "\" in \"", 
		       Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
    }
    return TCL_ERROR;
  }
  classId = CID_ELEM_LINE;
  if (penPtr->classId != classId) {
    if (interp != NULL) {
      Tcl_AppendResult(interp, "pen \"", name, 
		       "\" is the wrong type (is \"", 
		       Blt_GraphClassName(penPtr->classId), "\"", ", wanted \"", 
		       Blt_GraphClassName(classId), "\")", (char *)NULL);
    }
    return TCL_ERROR;
  }
  penPtr->refCount++;
  *penPtrPtr = penPtr;
  return TCL_OK;
}

void Blt_DestroyPens(Graph *graphPtr)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch iter;

  for (hPtr = Tcl_FirstHashEntry(&graphPtr->penTable, &iter);
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
    Pen *penPtr;

    penPtr = Tcl_GetHashValue(hPtr);
    penPtr->hashPtr = NULL;
    DestroyPen(penPtr);
  }
  Tcl_DeleteHashTable(&graphPtr->penTable);
}

static int CgetOp(Tcl_Interp *interp, Graph *graphPtr, 
		  int objc, Tcl_Obj *const *objv)
{
  if (objc != 5) {
    Tcl_WrongNumArgs(interp, 3, objv, "cget option");
    return TCL_ERROR;
  }

  Pen *penPtr;
  if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK)
    return TCL_ERROR;


  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)penPtr, 
				      penPtr->optionTable,
				      objv[4], graphPtr->tkwin);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(Tcl_Interp *interp, Graph *graphPtr, 
		       int objc, Tcl_Obj *const *objv)
{
  Pen *penPtr;
  int nNames, nOpts;
  int redraw;
  Tcl_Obj *const *options;
  int i;

  /* Figure out where the option value pairs begin */
  objc -= 3;
  objv += 3;
  for (i = 0; i < objc; i++) {
    char *string;

    string = Tcl_GetString(objv[i]);
    if (string[0] == '-') {
      break;
    }
    if (GetPenFromObj(interp, graphPtr, objv[i], &penPtr) != TCL_OK) {
      return TCL_ERROR;
    }
  }
  nNames = i;			/* Number of pen names specified */
  nOpts = objc - i;		/* Number of options specified */
  options = objv + i;		/* Start of options in objv  */

  redraw = 0;
  for (i = 0; i < nNames; i++) {
    if (GetPenFromObj(interp, graphPtr, objv[i], &penPtr) != TCL_OK) {
      return TCL_ERROR;
    }
    /*
      // waj
    flags = BLT_CONFIG_OBJV_ONLY | (penPtr->flags&(ACTIVE_PEN|NORMAL_PEN));
    if (nOpts == 0) {
      return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
				      penPtr->configSpecs, (char *)penPtr, (Tcl_Obj *)NULL, flags);
    } else if (nOpts == 1) {
      return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
				      penPtr->configSpecs, (char *)penPtr, options[0], flags);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, 
				   penPtr->configSpecs, nOpts, options, (char *)penPtr, flags) 
	!= TCL_OK) {
      break;
    }
    */
    (*penPtr->configProc) (graphPtr, penPtr);
    if (penPtr->refCount > 0) {
      redraw++;
    }
  }
  if (redraw) {
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
  }
  if (i < nNames) {
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int CreateOp(Tcl_Interp *interp, Graph *graphPtr, 
		    int objc, Tcl_Obj *const *objv)
{
  Pen *penPtr;

  penPtr = Blt_CreatePen(graphPtr, Tcl_GetString(objv[3]), graphPtr->classId,
			 objc - 4, objv + 4);
  if (penPtr == NULL)
    return TCL_ERROR;

  Tcl_SetObjResult(interp, objv[3]);
  return TCL_OK;
}

static int DeleteOp(Tcl_Interp *interp, Graph *graphPtr, 
		    int objc, Tcl_Obj *const *objv)
{
  int i;

  for (i = 3; i < objc; i++) {
    Pen *penPtr;

    if (GetPenFromObj(interp, graphPtr, objv[i], &penPtr) != TCL_OK) {
      return TCL_ERROR;
    }
    if (penPtr->flags & DELETE_PENDING) {
      Tcl_AppendResult(interp, "can't find pen \"", 
		       Tcl_GetString(objv[i]), "\" in \"", 
		       Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
      return TCL_ERROR;
    }
    penPtr->flags |= DELETE_PENDING;
    if (penPtr->refCount == 0) {
      DestroyPen(penPtr);
    }
  }
  return TCL_OK;
}

static int NamesOp(Tcl_Interp *interp, Graph *graphPtr, 
		   int objc, Tcl_Obj *const *objv)
{
  Tcl_Obj *listObjPtr;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    for (hPtr = Tcl_FirstHashEntry(&graphPtr->penTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Pen *penPtr;

      penPtr = Tcl_GetHashValue(hPtr);
      if ((penPtr->flags & DELETE_PENDING) == 0) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewStringObj(penPtr->name, -1));
      }
    }
  } else {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    for (hPtr = Tcl_FirstHashEntry(&graphPtr->penTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Pen *penPtr;

      penPtr = Tcl_GetHashValue(hPtr);
      if ((penPtr->flags & DELETE_PENDING) == 0) {
	int i;

	for (i = 3; i < objc; i++) {
	  char *pattern;

	  pattern = Tcl_GetString(objv[i]);
	  if (Tcl_StringMatch(penPtr->name, pattern)) {
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
				     Tcl_NewStringObj(penPtr->name, -1));
	    break;
	  }
	}
      }
    }
  }
  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int TypeOp(Tcl_Interp *interp, Graph *graphPtr, 
		  int objc, Tcl_Obj *const *objv)
{
  Pen *penPtr;

  if (GetPenFromObj(interp, graphPtr, objv[3], &penPtr) != TCL_OK)
    return TCL_ERROR;

  Tcl_SetStringObj(Tcl_GetObjResult(interp), Blt_GraphClassName(penPtr->classId), -1);
  return TCL_OK;
}

static Blt_OpSpec penOps[] =
  {
    {"cget", 2, CgetOp, 5, 5, "penName option",},
    {"configure", 2, ConfigureOp, 4, 0,
     "penName ?penName?... ?option value?...",},
    {"create", 2, CreateOp, 4, 0, "penName ?option value?...",},
    {"delete", 2, DeleteOp, 3, 0, "?penName?...",},
    {"names", 1, NamesOp, 3, 0, "?pattern?...",},
    {"type", 1, TypeOp, 4, 4, "penName",},
  };
static int nPenOps = sizeof(penOps) / sizeof(Blt_OpSpec);

int Blt_PenOp(Graph *graphPtr, Tcl_Interp *interp, 
	      int objc, Tcl_Obj *const *objv)
{
  GraphPenProc *proc = Blt_GetOpFromObj(interp, nPenOps, penOps, BLT_OP_ARG2, 
					objc, objv, 0);
  if (proc == NULL) {
    return TCL_ERROR;
  }
  return (*proc) (interp, graphPtr, objc, objv);
}
