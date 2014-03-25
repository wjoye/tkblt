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

#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "bltC.h"

extern "C" {
#include "bltInt.h"
#include "bltGraph.h"
#include "bltOp.h"
};

#include "bltGrElem.h"

// Defs

static void DestroyElement(Element* elemPtr);
static int ElementObjConfigure(Tcl_Interp* interp, Graph* graphPtr,
			       Element* elemPtr, 
			       int objc, Tcl_Obj* const objv[]);
static void FreeDataValues(ElemValues *valuesPtr);
static void FreeVectorSource(ElemValues *valuesPtr);
static void FreeElement(char* data);
static void FindRange(ElemValues *valuesPtr);
static int GetIndex(Tcl_Interp* interp, Element* elemPtr, 
		    Tcl_Obj *objPtr, int *indexPtr);
static int GetPenStyleFromObj(Tcl_Interp* interp, Graph* graphPtr,
			      Tcl_Obj *objPtr, ClassId classId,
			      PenStyle *stylePtr);
static int GetVectorData(Tcl_Interp* interp, ElemValues *valuesPtr, 
			 const char *vecName);
static int ParseValues(Tcl_Interp* interp, Tcl_Obj *objPtr, int *nValuesPtr,
		       double **arrayPtr);
typedef int (GraphElementProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			       Tcl_Obj *const *objv);

// OptionSpecs

const char* fillObjOption[] = {"none", "x", "y", "both", NULL};

static Tk_CustomOptionSetProc ValuesSetProc;
static Tk_CustomOptionGetProc ValuesGetProc;
static Tk_CustomOptionFreeProc ValuesFreeProc;
Tk_ObjCustomOption valuesObjOption =
  {
    "values", ValuesSetProc, ValuesGetProc, RestoreProc, ValuesFreeProc, NULL
  };

static int ValuesSetProc(ClientData clientData, Tcl_Interp* interp,
		       Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		       int offset, char* savePtr, int flags)
{
  ElemValues** valuesPtrPtr = (ElemValues**)(widgRec + offset);
  *(double*)savePtr = *(double*)valuesPtrPtr;
  Element* elemPtr = (Element*)widgRec;

  if (!valuesPtrPtr)
    return TCL_OK;

  Tcl_Obj** objv;
  int objc;
  if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  if (objc == 0) {
    *valuesPtrPtr = NULL;
    return TCL_OK;
  }

  ElemValues* valuesPtr = (ElemValues*)calloc(1, sizeof(ElemValues));
  valuesPtr->elemPtr = elemPtr;

  const char *string = Tcl_GetString(objv[0]);
  if ((objc == 1) && (Blt_VectorExists2(interp, string))) {
    valuesPtr->type = ELEM_SOURCE_VECTOR;

    if (GetVectorData(interp, valuesPtr, string) != TCL_OK)
      return TCL_ERROR;
  }
  else {
    valuesPtr->type = ELEM_SOURCE_VALUES;

    double *values;
    int nValues;
    if (ParseValues(interp, *objPtr, &nValues, &values) != TCL_OK)
      return TCL_ERROR;
    valuesPtr->values = values;
    valuesPtr->nValues = nValues;
    FindRange(valuesPtr);
  }

  *valuesPtrPtr = valuesPtr;
  return TCL_OK;
}

static Tcl_Obj* ValuesGetProc(ClientData clientData, Tk_Window tkwin, 
			    char *widgRec, int offset)
{
  ElemValues* valuesPtr = *(ElemValues**)(widgRec + offset);

  if (!valuesPtr)
   return Tcl_NewStringObj("", -1);
    
  switch (valuesPtr->type) {
  case ELEM_SOURCE_VECTOR:
    {
      const char* vecName = 
	Blt_NameOfVectorId(valuesPtr->vectorSource.vector);
      return Tcl_NewStringObj(vecName, -1);
    }
  case ELEM_SOURCE_VALUES:
    {
      int cnt = valuesPtr->nValues;
      if (!cnt)
	return Tcl_NewListObj(0, (Tcl_Obj**)NULL);

      Tcl_Obj** ll = (Tcl_Obj**)calloc(cnt, sizeof(Tcl_Obj*));
      for (int ii=0; ii<cnt; ii++)
	ll[ii] = Tcl_NewDoubleObj(valuesPtr->values[ii]);
      Tcl_Obj* listObjPtr = Tcl_NewListObj(cnt, ll);
      free(ll);

      return listObjPtr;
    }
 default:
   return Tcl_NewStringObj("", -1);
  }
}

static void ValuesFreeProc(ClientData clientData, Tk_Window tkwin,
			   char *ptr)
{
  ElemValues* valuesPtr = *(ElemValues**)ptr;
  if (valuesPtr) {
    switch (valuesPtr->type) {
    case ELEM_SOURCE_VECTOR: 
      FreeVectorSource(valuesPtr);
      break;
    case ELEM_SOURCE_VALUES:
      break;
    }

    FreeDataValues(valuesPtr);
    free(valuesPtr);
  }
}

static Tk_CustomOptionSetProc PairsSetProc;
static Tk_CustomOptionGetProc PairsGetProc;
static Tk_CustomOptionRestoreProc PairsRestoreProc;
static Tk_CustomOptionFreeProc PairsFreeProc;
Tk_ObjCustomOption pairsObjOption =
  {
    "pairs", PairsSetProc, PairsGetProc, PairsRestoreProc, PairsFreeProc, NULL
  };

static int PairsSetProc(ClientData clientData, Tcl_Interp* interp,
		       Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		       int offset, char* savePtr, int flags)
{
  ElemCoords* coordsPtr = (ElemCoords*)(widgRec + offset);
  *(double*)savePtr = (double)NULL;

  double *values;
  int nValues;
  if (ParseValues(interp, *objPtr, &nValues, &values) != TCL_OK)
    return TCL_ERROR;

  if (nValues & 1) {
    Tcl_AppendResult(interp, "odd number of data points", NULL);
    free(values);
    return TCL_ERROR;
  }

  nValues /= 2;
  size_t newSize = nValues * sizeof(double);
  FreeDataValues(coordsPtr->x);
  FreeDataValues(coordsPtr->y);
  coordsPtr->x = NULL;
  coordsPtr->y = NULL;

  if (newSize==0)
    return TCL_OK;

  coordsPtr->x = (ElemValues*)calloc(1,sizeof(ElemValues));
  coordsPtr->y = (ElemValues*)calloc(1,sizeof(ElemValues));

  coordsPtr->x->values = (double*)malloc(newSize);
  coordsPtr->x->nValues = nValues;
  coordsPtr->y->values = (double*)malloc(newSize);
  coordsPtr->x->nValues = nValues;

  int ii=0;
  for (double* p = values; ii<nValues; ii++) {
    coordsPtr->x->values[ii] = *p++;
    coordsPtr->y->values[ii] = *p++;
  }
  free(values);

  FindRange(coordsPtr->x);
  FindRange(coordsPtr->y);

  return TCL_OK;
};

static Tcl_Obj* PairsGetProc(ClientData clientData, Tk_Window tkwin, 
			    char *widgRec, int offset)
{
  ElemCoords* coordsPtr = (ElemCoords*)(widgRec + offset);

  if (!coordsPtr || 
      !coordsPtr->x || !coordsPtr->y || 
      !coordsPtr->x->nValues || !coordsPtr->y->nValues)
    return Tcl_NewListObj(0, (Tcl_Obj**)NULL);

  int cnt = MIN(coordsPtr->x->nValues, coordsPtr->y->nValues);
  Tcl_Obj** ll = (Tcl_Obj**)calloc(2*cnt,sizeof(Tcl_Obj*));
  for (int ii=0, jj=0; ii<cnt; ii++) {
    ll[jj++] = Tcl_NewDoubleObj(coordsPtr->x->values[ii]);
    ll[jj++] = Tcl_NewDoubleObj(coordsPtr->y->values[ii]);
  }
  Tcl_Obj* listObjPtr = Tcl_NewListObj(2*cnt, ll);
  free(ll);

  return listObjPtr;
};

static void PairsRestoreProc(ClientData clientData, Tk_Window tkwin,
			     char *ptr, char *savePtr)
{
  // do nothing
}

static void PairsFreeProc(ClientData clientData, Tk_Window tkwin, char *ptr)
{
  // do nothing
}

int StyleSetProc(ClientData clientData, Tcl_Interp* interp,
		 Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		 int offset, char* save, int flags)
{
  Blt_Chain stylePalette = *(Blt_Chain*)(widgRec + offset);
  Element* elemPtr = (Element*)(widgRec);
  size_t size = (size_t)clientData;

  Tcl_Obj** objv;
  int objc;
  if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  /* Reserve the first entry for the "normal" pen. We'll set the
   * style later */
  Blt_FreeStylePalette(stylePalette);
  Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette);
  if (link == NULL) {
    link = Blt_Chain_AllocLink(size);
    Blt_Chain_LinkAfter(stylePalette, link, NULL);
  }
  PenStyle* stylePtr = (PenStyle*)Blt_Chain_GetValue(link);
  stylePtr->penPtr = NORMALPEN(elemPtr);
  for (int ii = 0; ii<objc; ii++) {
    link = Blt_Chain_AllocLink(size);
    stylePtr = (PenStyle*)Blt_Chain_GetValue(link);
    stylePtr->weight.min = (double)ii;
    stylePtr->weight.max = (double)ii + 1.0;
    stylePtr->weight.range = 1.0;
    if (GetPenStyleFromObj(interp, elemPtr->obj.graphPtr, objv[ii], 
			   elemPtr->obj.classId, 
			   (PenStyle *)stylePtr) != TCL_OK) {
      Blt_FreeStylePalette(stylePalette);
      return TCL_ERROR;
    }
    Blt_Chain_LinkAfter(stylePalette, link, NULL);
  }

  return TCL_OK;
}

Tcl_Obj* StyleGetProc(ClientData clientData, Tk_Window tkwin, 
		      char *widgRec, int offset)
{
  Blt_Chain stylePalette = *(Blt_Chain*)(widgRec + offset);

  // count how many
  int cnt =0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette); !link; 
       link = Blt_Chain_NextLink(link), cnt++) {}
  if (!cnt)
    return Tcl_NewListObj(0, (Tcl_Obj**)NULL);

  Tcl_Obj** ll = (Tcl_Obj**)calloc(3*cnt, sizeof(Tcl_Obj*));
  int ii=0;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette); !link; 
       link = Blt_Chain_NextLink(link)) {
    PenStyle *stylePtr = (PenStyle*)Blt_Chain_GetValue(link);
    ll[ii++] = Tcl_NewStringObj(stylePtr->penPtr->name, -1);
    ll[ii++] = Tcl_NewDoubleObj(stylePtr->weight.min);
    ll[ii++] = Tcl_NewDoubleObj(stylePtr->weight.max);
  }
  Tcl_Obj *listObjPtr = Tcl_NewListObj(3*cnt,ll);
  free(ll);

  return listObjPtr;
}

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
    elemPtr = Blt_BarElement(graphPtr);
    break;
  case CID_ELEM_LINE:
    elemPtr = Blt_LineElement(graphPtr);
    break;
  default:
    return TCL_ERROR;
  }
  if (!elemPtr)
    return TCL_ERROR;

  elemPtr->obj.graphPtr = graphPtr;
  elemPtr->obj.name = Blt_Strdup(name);

  // this is an option and will be freed via Tk_FreeConfigOptions
  // By default an element's name and label are the same
  elemPtr->label = Tcl_Alloc(strlen(name)+1);
  if (name)
    strcpy((char*)elemPtr->label,(char*)name);
  Blt_GraphSetObjectClass(&elemPtr->obj, classId);
  elemPtr->stylePalette = Blt_Chain_Create();

  elemPtr->hashPtr = hPtr;
  Tcl_SetHashValue(hPtr, elemPtr);

  if ((Tk_InitOptions(graphPtr->interp, (char*)elemPtr, elemPtr->optionTable, graphPtr->tkwin) != TCL_OK) || (ElementObjConfigure(interp,graphPtr, elemPtr, objc-4, objv+4) != TCL_OK)) {
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

  if (elemPtr->link) {
    Blt_Chain_DeleteLink(graphPtr->elements.displayList, elemPtr->link);
    if (!(elemPtr->flags & DELETE_PENDING)) {
      graphPtr->flags |= RESET_WORLD;
      Blt_EventuallyRedrawGraph(graphPtr);
    }
  }

  if (elemPtr->hashPtr)
    Tcl_DeleteHashEntry(elemPtr->hashPtr);

  if (elemPtr->obj.name)
    free((void*)(elemPtr->obj.name));

  Tk_FreeConfigOptions((char*)elemPtr, elemPtr->optionTable, graphPtr->tkwin);

  (*elemPtr->procsPtr->destroyProc) (graphPtr, elemPtr);
  free(elemPtr);
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
				      elemPtr->optionTable,
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
				       elemPtr->optionTable, 
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
      if (Tk_SetOptions(interp, (char*)elemPtr, elemPtr->optionTable, 
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
    if ((*elemPtr->procsPtr->configProc)(graphPtr, elemPtr) != TCL_OK)
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
  Element* elemPtr;
  int i;
  int *indices;
  int nIndices;

  if (objc == 3) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    /* List all the currently active elements */
    for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      if (elemPtr->flags & ACTIVE) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewStringObj(elemPtr->obj.name, -1));
      }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
    return TCL_ERROR;	/* Can't find named element */
  }
  elemPtr->flags |= ACTIVE | ACTIVE_PENDING;

  indices = NULL;
  nIndices = -1;
  if (objc > 4) {
    int *activePtr;

    nIndices = objc - 4;
    activePtr = indices = (int*)malloc(sizeof(int) * nIndices);
    for (i = 4; i < objc; i++) {
      if (GetIndex(interp, elemPtr, objv[i], activePtr) != TCL_OK) {
	return TCL_ERROR;
      }
      activePtr++;
    }
  }
  if (elemPtr->activeIndices) {
    free(elemPtr->activeIndices);
  }
  elemPtr->nActiveIndices = nIndices;
  elemPtr->activeIndices = indices;
  Blt_EventuallyRedrawGraph(graphPtr);
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
    for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.tagTable, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
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
    Element* elemPtr;
    if (Blt_GetElement(interp, graphPtr, objv[5], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    if (elemPtr && !elemPtr->hide && 
	!(elemPtr->flags & (MAP_ITEM|DELETE_PENDING)))
      (*elemPtr->procsPtr->closestProc) (graphPtr, elemPtr);
  }
  else {
    // Find the closest point from the set of displayed elements,
    // searching the display list from back to front.  That way if
    // the points from two different elements overlay each other
    // exactly, the last one picked will be the topmost.  

    for (Blt_ChainLink link=Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
      Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
      if (elemPtr && !elemPtr->hide && 
	  !(elemPtr->flags & (MAP_ITEM|DELETE_PENDING)))
	(*elemPtr->procsPtr->closestProc) (graphPtr, elemPtr);
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

static int DeactivateOp(Graph* graphPtr, Tcl_Interp* interp,
			int objc, Tcl_Obj* const objv[])
{
  Element* elemPtr;
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK)
    return TCL_ERROR;

  elemPtr->flags &= ~(ACTIVE | ACTIVE_PENDING);
  if (elemPtr->activeIndices) {
    free(elemPtr->activeIndices);
    elemPtr->activeIndices = NULL;
  }
  elemPtr->nActiveIndices = 0;
  Blt_EventuallyRedrawGraph(graphPtr);

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

static int DeleteOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  if (objc<4)
    return TCL_ERROR;
    
  Element* elemPtr;
  if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
    Tcl_AppendResult(interp, "can't find element \"", 
		     Tcl_GetString(objv[3]), "\" in \"", 
		     Tk_PathName(graphPtr->tkwin), "\"", NULL);
      return TCL_ERROR;
  }

  elemPtr->flags |= DELETE_PENDING;
  Tcl_EventuallyFree(elemPtr, FreeElement);
  Blt_EventuallyRedrawGraph(graphPtr);

  return TCL_OK;
}

static int ExistsOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_FindHashEntry(&graphPtr->elements.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp,
		 int objc, Tcl_Obj* const objv[])
{
  char *string;

  string = Tcl_GetString(objv[3]);
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

static Tcl_Obj *DisplayListObj(Graph* graphPtr)
{
  Tcl_Obj *listObjPtr;
  Blt_ChainLink link;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
    Tcl_ListObjAppendElement(graphPtr->interp, listObjPtr, objPtr);
  }
  return listObjPtr;
}

static int LowerOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Blt_Chain chain;
  Blt_ChainLink link, next;

  // Move the links of lowered elements out of the display list into
  // a temporary list
  chain = Blt_Chain_Create();
  for (int ii=3; ii<objc; ii++) {
    Element* elemPtr;

    if (Blt_GetElement(interp, graphPtr, objv[ii], &elemPtr) != TCL_OK)
      return TCL_ERROR;

    Blt_Chain_UnlinkLink(graphPtr->elements.displayList, elemPtr->link); 
    Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
  }

  // Append the links to end of the display list
  for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
    next = Blt_Chain_NextLink(link);
    Blt_Chain_UnlinkLink(chain, link); 
    Blt_Chain_LinkAfter(graphPtr->elements.displayList, link, NULL); 
  }	
  Blt_Chain_Destroy(chain);
  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int NamesOp(Graph* graphPtr, Tcl_Interp* interp,
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      Tcl_Obj *objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
  }
  else {
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;

    for (hPtr = Tcl_FirstHashEntry(&graphPtr->elements.table, &iter);
	 hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
      int i;

      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      for (i = 3; i < objc; i++) {
	if (Tcl_StringMatch(elemPtr->obj.name,Tcl_GetString(objv[i]))) {
	  Tcl_Obj *objPtr;

	  objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
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
  Blt_Chain chain;
  Blt_ChainLink link, prev;
  int i;

  /* Move the links of lowered elements out of the display list into
   * a temporary list. */
  chain = Blt_Chain_Create();
  for (i = 3; i < objc; i++) {
    Element* elemPtr;

    if (Blt_GetElement(interp, graphPtr, objv[i], &elemPtr) != TCL_OK) {
      return TCL_ERROR;	/* Can't find named element */
    }
    Blt_Chain_UnlinkLink(graphPtr->elements.displayList, elemPtr->link); 
    Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
  }
  /* Prepend the links to beginning of the display list in reverse order. */
  for (link = Blt_Chain_LastLink(chain); link != NULL; link = prev) {
    prev = Blt_Chain_PrevLink(link);
    Blt_Chain_UnlinkLink(chain, link); 
    Blt_Chain_LinkBefore(graphPtr->elements.displayList, link, NULL); 
  }	
  Blt_Chain_Destroy(chain);
  Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int ShowOp(Graph* graphPtr, Tcl_Interp* interp,
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 4) {
    Tcl_Obj **elem;
    int n;
    if (Tcl_ListObjGetElements(interp, objv[3], &n, &elem) != TCL_OK)
      return TCL_ERROR;

    // Collect the named elements into a list
    Blt_Chain chain = Blt_Chain_Create();
    for (int ii = 0; ii<n; ii++) {
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
  }

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
  {"closest",    2, (void*)ClosestOp,    5, 0,
   "x y ?option value?... ?elemName?...",},
  {"configure",  2, (void*)ConfigureOp,  4, 0,
   "elemName ?elemName?... ?option value?...",},
  {"create",     2, (void*)CreateOp,     4, 0, "elemName ?option value?...",},
  {"deactivate", 3, (void*)DeactivateOp, 3, 0, "?elemName?...",},
  {"delete",     3, (void*)DeleteOp,     3, 0, "?elemName?...",},
  {"exists",     1, (void*)ExistsOp,     4, 4, "elemName",},
  {"get",        1, (void*)GetOp,        4, 4, "name",},
  {"lower",      1, (void*)LowerOp,      3, 0, "?elemName?...",},
  {"names",      1, (void*)NamesOp,      3, 0, "?pattern?...",},
  {"raise",      1, (void*)RaiseOp,      3, 0, "?elemName?...",},
  {"show",       1, (void*)ShowOp,       3, 4, "?elemList?",},
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

static void FreeElement(char* data)
{
  Element* elemPtr = (Element *)data;
  DestroyElement(elemPtr);
}

static int GetPenStyleFromObj(Tcl_Interp* interp, Graph* graphPtr,
			      Tcl_Obj *objPtr, ClassId classId,
			      PenStyle *stylePtr)
{
  Pen* penPtr;
  Tcl_Obj **objv;
  int objc;

  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
    return TCL_ERROR;
  }
  if ((objc != 1) && (objc != 3)) {
    if (interp) {
      Tcl_AppendResult(interp, "bad style entry \"", 
		       Tcl_GetString(objPtr), 
		       "\": should be \"penName\" or \"penName min max\"", 
		       NULL);
    }
    return TCL_ERROR;
  }
  if (Blt_GetPenFromObj(interp, graphPtr, objv[0], classId, &penPtr) != TCL_OK)
    return TCL_ERROR;

  if (objc == 3) {
    double min, max;

    if ((Tcl_GetDoubleFromObj(interp, objv[1], &min) != TCL_OK) ||
	(Tcl_GetDoubleFromObj(interp, objv[2], &max) != TCL_OK)) {
      return TCL_ERROR;
    }
    SetWeight(stylePtr->weight, min, max);
  }
  stylePtr->penPtr = penPtr;
  return TCL_OK;
}

static void FreeVectorSource(ElemValues *valuesPtr)
{
  if (valuesPtr->vectorSource.vector) { 
    Blt_SetVectorChangedProc(valuesPtr->vectorSource.vector, NULL, NULL);
    Blt_FreeVectorId(valuesPtr->vectorSource.vector); 
    valuesPtr->vectorSource.vector = NULL;
  }
}

static int FetchVectorValues(Tcl_Interp* interp, ElemValues *valuesPtr, 
			     Blt_Vector *vector)
{
  double *array;
  if (!valuesPtr->values)
    array = (double*)malloc(Blt_VecLength(vector) * sizeof(double));
  else
    array = (double*)realloc(valuesPtr->values, Blt_VecLength(vector) * sizeof(double));

  if (!array) {
    if (interp)
      Tcl_AppendResult(interp, "can't allocate new vector", NULL);
    return TCL_ERROR;
  }

  memcpy(array, Blt_VecData(vector), sizeof(double) * Blt_VecLength(vector));
  valuesPtr->min = Blt_VecMin(vector);
  valuesPtr->max = Blt_VecMax(vector);
  valuesPtr->values = array;
  valuesPtr->nValues = Blt_VecLength(vector);

  return TCL_OK;
}

static void VectorChangedProc(Tcl_Interp* interp, ClientData clientData, 
			      Blt_VectorNotify notify)
{
  ElemValues* valuesPtr = (ElemValues*)clientData;

  if (notify == BLT_VECTOR_NOTIFY_DESTROY)
    FreeDataValues(valuesPtr);
  else {
    Blt_Vector* vector;
    Blt_GetVectorById(interp, valuesPtr->vectorSource.vector, &vector);
    if (FetchVectorValues(NULL, valuesPtr, vector) != TCL_OK)
      return;
  }

  Element* elemPtr = valuesPtr->elemPtr;
  Graph* graphPtr = elemPtr->obj.graphPtr;
  graphPtr->flags |= RESET_AXES;
  elemPtr->flags |= MAP_ITEM;
  if (elemPtr->link && !(elemPtr->flags & DELETE_PENDING)) {
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
  }
}

static int GetVectorData(Tcl_Interp* interp, ElemValues *valuesPtr, 
			 const char *vecName)
{
  VectorDataSource *srcPtr = &valuesPtr->vectorSource;
  srcPtr->vector = Blt_AllocVectorId(interp, vecName);

  Blt_Vector *vecPtr;
  if (Blt_GetVectorById(interp, srcPtr->vector, &vecPtr) != TCL_OK)
    return TCL_ERROR;

  if (FetchVectorValues(interp, valuesPtr, vecPtr) != TCL_OK) {
    FreeVectorSource(valuesPtr);
    return TCL_ERROR;
  }

  Blt_SetVectorChangedProc(srcPtr->vector, VectorChangedProc, valuesPtr);
  return TCL_OK;
}

static int ParseValues(Tcl_Interp* interp, Tcl_Obj *objPtr, int *nValuesPtr,
		       double **arrayPtr)
{
  int objc;
  Tcl_Obj **objv;
  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  *arrayPtr = NULL;
  *nValuesPtr = 0;
  if (objc > 0) {
    double *p;
    int i;

    double *array = (double*)malloc(sizeof(double) * objc);
    if (!array) {
      Tcl_AppendResult(interp, "can't allocate new vector", NULL);
      return TCL_ERROR;
    }
    for (p = array, i = 0; i < objc; i++, p++) {
      if (Blt_ExprDoubleFromObj(interp, objv[i], p) != TCL_OK) {
	free(array);
	return TCL_ERROR;
      }
    }
    *arrayPtr = array;
    *nValuesPtr = objc;
  }
  return TCL_OK;
}

static void FreeDataValues(ElemValues *valuesPtr)
{
  switch (valuesPtr->type) {
  case ELEM_SOURCE_VECTOR: 
    FreeVectorSource(valuesPtr);
    break;
  case ELEM_SOURCE_VALUES:
    break;
  }
  if (valuesPtr->values)
    free(valuesPtr->values);
  valuesPtr->values = NULL;
  valuesPtr->nValues =0;
}

static void FindRange(ElemValues *valuesPtr)
{
  int i;
  double *x;
  double min, max;

  if (!valuesPtr || 
      (valuesPtr->nValues < 1) || 
      (valuesPtr->values == NULL))
    return;
  x = valuesPtr->values;

  min = DBL_MAX, max = -DBL_MAX;
  for(i = 0; i < valuesPtr->nValues; i++) {
    if (isfinite(x[i])) {
      min = max = x[i];
      break;
    }
  }
  /*  Initialize values to track the vector range */
  for (/* empty */; i < valuesPtr->nValues; i++) {
    if (isfinite(x[i])) {
      if (x[i] < min) {
	min = x[i];
      } else if (x[i] > max) {
	max = x[i];
      }
    }
  }
  valuesPtr->min = min, valuesPtr->max = max;
}

double Blt_FindElemValuesMinimum(ElemValues *valuesPtr, double minLimit)
{
  double min = DBL_MAX;
  if (!valuesPtr)
    return min;

  for (int ii=0; ii<valuesPtr->nValues; ii++) {
    double x = valuesPtr->values[ii];
    if (x < 0.0) {
      /* What do you do about negative values when using log
       * scale values seems like a grey area.  Mirror. */
      x = -x;
    }
    if ((x > minLimit) && (min > x)) {
      min = x;
    }
  }
  if (min == DBL_MAX)
    min = minLimit;

  return min;
}

void Blt_FreeStylePalette(Blt_Chain stylePalette)
{
  Blt_ChainLink link;

  /* Skip the first slot. It contains the built-in "normal" pen of
   * the element.  */
  link = Blt_Chain_FirstLink(stylePalette);
  if (link) {
    Blt_ChainLink next;

    for (link = Blt_Chain_NextLink(link); link != NULL; link = next) {

      next = Blt_Chain_NextLink(link);
      PenStyle *stylePtr = (PenStyle*)Blt_Chain_GetValue(link);
      Blt_FreePen(stylePtr->penPtr);
      Blt_Chain_DeleteLink(stylePalette, link);
    }
  }
}

PenStyle** Blt_StyleMap(Element* elemPtr)
{
  int i;
  int nWeights;		/* Number of weights to be examined.
			 * If there are more data points than
			 * weights, they will default to the
			 * normal pen. */
  Blt_ChainLink link;
  double *w;			/* Weight vector */
  int nPoints;

  nPoints = NUMBEROFPOINTS(elemPtr);
  nWeights = MIN(elemPtr->w ? elemPtr->w->nValues : 0, nPoints);
  w = elemPtr->w ? elemPtr->w->values : NULL;
  link = Blt_Chain_FirstLink(elemPtr->stylePalette);
  PenStyle *stylePtr = (PenStyle*)Blt_Chain_GetValue(link);

  /* 
   * Create a style mapping array (data point index to style), 
   * initialized to the default style.
   */
  PenStyle **dataToStyle = (PenStyle**)malloc(nPoints * sizeof(PenStyle *));
  for (i = 0; i < nPoints; i++)
    dataToStyle[i] = stylePtr;

  for (i = 0; i < nWeights; i++) {
    for (link = Blt_Chain_LastLink(elemPtr->stylePalette); link != NULL; 
	 link = Blt_Chain_PrevLink(link)) {
      stylePtr = (PenStyle*)Blt_Chain_GetValue(link);

      if (stylePtr->weight.range > 0.0) {
	double norm;

	norm = (w[i] - stylePtr->weight.min) / stylePtr->weight.range;
	if (((norm - 1.0) <= DBL_EPSILON) && 
	    (((1.0 - norm) - 1.0) <= DBL_EPSILON)) {
	  dataToStyle[i] = stylePtr;
	  break;		/* Done: found range that matches. */
	}
      }
    }
  }
  return dataToStyle;
}


static int GetIndex(Tcl_Interp* interp, Element* elemPtr, 
		    Tcl_Obj *objPtr, int *indexPtr)
{
  char *string = Tcl_GetString(objPtr);
  if ((*string == 'e') && (strcmp("end", string) == 0))
    *indexPtr = NUMBEROFPOINTS(elemPtr);
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
    (*elemPtr->procsPtr->configProc) (graphPtr, elemPtr);
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
      (*elemPtr->procsPtr->mapProc) (graphPtr, elemPtr);
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
    if (!(elemPtr->flags & DELETE_PENDING) && !elemPtr->hide) {
      (*elemPtr->procsPtr->drawNormalProc)(graphPtr, drawable, elemPtr);
    }
  }
}

void Blt_DrawActiveElements(Graph* graphPtr, Drawable drawable)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!(elemPtr->flags & DELETE_PENDING) && 
	(elemPtr->flags & ACTIVE) && 
	!elemPtr->hide) {
      (*elemPtr->procsPtr->drawActiveProc)(graphPtr, drawable, elemPtr);
    }
  }
}

void Blt_ElementsToPostScript(Graph* graphPtr, Blt_Ps ps)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Element* elemPtr = (Element*)Blt_Chain_GetValue(link);
    if (!(elemPtr->flags & DELETE_PENDING) && !elemPtr->hide) {
      continue;
    }
    /* Comment the PostScript to indicate the start of the element */
    Blt_Ps_Format(ps, "\n%% Element \"%s\"\n\n", elemPtr->obj.name);
    (*elemPtr->procsPtr->printNormalProc) (graphPtr, ps, elemPtr);
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
	!elemPtr->hide) {
      Blt_Ps_Format(ps, "\n%% Active Element \"%s\"\n\n", 
		    elemPtr->obj.name);
      (*elemPtr->procsPtr->printActiveProc)(graphPtr, ps, elemPtr);
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
