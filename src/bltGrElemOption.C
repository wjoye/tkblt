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
};

#include "bltGrElem.h"
#include "bltGrElemOption.h"

#define ELEM_SOURCE_VALUES	0
#define ELEM_SOURCE_VECTOR	1

#define SETRANGE(l) ((l).range = ((l).max > (l).min) ? ((l).max - (l).min) : DBL_EPSILON)
#define SETWEIGHT(l, lo, hi) ((l).min = (lo), (l).max = (hi), SETRANGE(l))

// Defs

extern int Blt_GetPenFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			     Tcl_Obj *objPtr, ClassId classId, Pen **penPtrPtr);

static void FreeDataValues(ElemValues* valuesPtr);
static void FreeVectorSource(ElemValues* valuesPtr);
static void FindRange(ElemValues* valuesPtr);
static int GetPenStyleFromObj(Tcl_Interp* interp, Graph* graphPtr,
			      Tcl_Obj *objPtr, ClassId classId,
			      PenStyle *stylePtr);
static int GetVectorData(Tcl_Interp* interp, ElemValues* valuesPtr, 
			 const char *vecName);
static int ParseValues(Tcl_Interp* interp, Tcl_Obj *objPtr, int *nValuesPtr,
		       double **arrayPtr);

// OptionSpecs

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
  if (!valuesPtr)
    return;

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

  double* values;
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

  if (newSize == 0)
    return TCL_OK;

  coordsPtr->x = (ElemValues*)calloc(1,sizeof(ElemValues));
  coordsPtr->y = (ElemValues*)calloc(1,sizeof(ElemValues));

  coordsPtr->x->values = (double*)malloc(newSize);
  coordsPtr->x->nValues = nValues;
  coordsPtr->y->values = (double*)malloc(newSize);
  coordsPtr->y->nValues = nValues;

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

  int objc;
  Tcl_Obj** objv;
  if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  // Reserve the first entry for the "normal" pen. We'll set the style later
  Blt_FreeStylePalette(stylePalette);
  Blt_ChainLink link = Blt_Chain_FirstLink(stylePalette);
  if (!link) {
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
			   (PenStyle*)stylePtr) != TCL_OK) {
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

void StyleRestoreProc(ClientData clientData, Tk_Window tkwin,
		      char *ptr, char *savePtr)
{
  // do nothing
}

void StyleFreeProc(ClientData clientData, Tk_Window tkwin, char *ptr)
{
  // do nothing
}

// Support

static int GetPenStyleFromObj(Tcl_Interp* interp, Graph* graphPtr,
			      Tcl_Obj *objPtr, ClassId classId,
			      PenStyle *stylePtr)
{
  int objc;
  Tcl_Obj **objv;
  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  if ((objc != 1) && (objc != 3)) {
    Tcl_AppendResult(interp, "bad style entry \"", 
		     Tcl_GetString(objPtr), 
		     "\": should be \"penName\" or \"penName min max\"", 
		     NULL);
    return TCL_ERROR;
  }

  Pen* penPtr;
  if (Blt_GetPenFromObj(interp, graphPtr, objv[0], classId, &penPtr) != TCL_OK)
    return TCL_ERROR;

  if (objc == 3) {
    double min, max;
    if ((Tcl_GetDoubleFromObj(interp, objv[1], &min) != TCL_OK) ||
	(Tcl_GetDoubleFromObj(interp, objv[2], &max) != TCL_OK))
      return TCL_ERROR;

    SETWEIGHT(stylePtr->weight, min, max);
  }

  stylePtr->penPtr = penPtr;
  return TCL_OK;
}

static void FreeVectorSource(ElemValues* valuesPtr)
{
  if (!valuesPtr)
    return;

  if (valuesPtr->vectorSource.vector) { 
    Blt_SetVectorChangedProc(valuesPtr->vectorSource.vector, NULL, NULL);
    Blt_FreeVectorId(valuesPtr->vectorSource.vector); 
    valuesPtr->vectorSource.vector = NULL;
  }
}

static int FetchVectorValues(Tcl_Interp* interp, ElemValues* valuesPtr, 
			     Blt_Vector *vector)
{
  if (!valuesPtr)
    return TCL_ERROR;

  double *array = !valuesPtr->values ?
    (double*)malloc(Blt_VecLength(vector) * sizeof(double)) :
    (double*)realloc(valuesPtr->values, Blt_VecLength(vector)*sizeof(double));

  if (!array) {
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
  if (!valuesPtr)
    return;

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

static int GetVectorData(Tcl_Interp* interp, ElemValues* valuesPtr, 
			 const char *vecName)
{
  if (!valuesPtr)
    return TCL_ERROR;

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

static void FreeDataValues(ElemValues* valuesPtr)
{
  if (!valuesPtr)
    return;

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

static void FindRange(ElemValues* valuesPtr)
{
  if (!valuesPtr || (valuesPtr->nValues < 1) || !valuesPtr->values)
    return;

  double* x = valuesPtr->values;
  double min = DBL_MAX;
  double max = -DBL_MAX;
  int ii;
  for(ii=0; ii<valuesPtr->nValues; ii++) {
    if (isfinite(x[ii])) {
      min = max = x[ii];
      break;
    }
  }

  //  Initialize values to track the vector range
  for (/* empty */; ii<valuesPtr->nValues; ii++) {
    if (isfinite(x[ii])) {
      if (x[ii] < min)
	min = x[ii];
      else if (x[ii] > max)
	max = x[ii];
    }
  }
  valuesPtr->min = min;
  valuesPtr->max = max;
}

