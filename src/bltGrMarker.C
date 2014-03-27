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

#include "bltC.h"
#include "bltMath.h"

extern "C" {
#include "bltGraph.h"
#include "bltOp.h"
};

#include "bltConfig.h"
#include "bltGrElem.h"
#include "bltGrMarker.h"

using namespace Blt;

extern MarkerCreateProc Blt_CreateBitmapProc;
extern MarkerCreateProc Blt_CreateLineProc;
extern MarkerCreateProc Blt_CreatePolygonProc;
extern MarkerCreateProc Blt_CreateTextProc;
extern MarkerCreateProc Blt_CreateWindowProc;

#define NORMALIZE(A,x) 	(((x) - (A)->axisRange.min) * (A)->axisRange.scale)

Marker::Marker(Graph* graphPtr)
{
  obj.classId =CID_NONE;
  obj.name =NULL;
  obj.className =NULL;
  obj.graphPtr =graphPtr;
  obj.tags = NULL;

  classPtr =NULL;
  optionTable =NULL;
  hashPtr =NULL;
  link =NULL;
  clipped =0;
  flags =0;
}

Marker::~Marker()
{
  Graph* graphPtr = obj.graphPtr;

  // If the marker to be deleted is currently displayed below the
  // elements, then backing store needs to be repaired.
  if (((MarkerOptions*)ops)->drawUnder)
    graphPtr->flags |= CACHE_DIRTY;

  Blt_DeleteBindings(graphPtr->bindTable, this);

  if (obj.name)
    free((void*)obj.name);

  if (hashPtr)
    Tcl_DeleteHashEntry(hashPtr);

  if (link)
    Blt_Chain_DeleteLink(graphPtr->markers.displayList, link);

  Tk_FreeConfigOptions((char*)ops, optionTable, graphPtr->tkwin);

  if (ops)
    free(ops);
}

// Defs

static int GetCoordinate(Tcl_Interp* interp, Tcl_Obj *objPtr, double *valuePtr);
static Tcl_Obj* PrintCoordinate(double x);

typedef int (GraphMarkerProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			      Tcl_Obj* const objv[]);

static int MarkerObjConfigure( Tcl_Interp* interp, Graph* graphPtr,
			       Marker* markerPtr,
			       int objc, Tcl_Obj* const objv[]);
static void DestroyMarker(Marker* markerPtr);
static int GetMarkerFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			    Tcl_Obj* objPtr, Marker** markerPtrPtr);
static int IsElementHidden(Marker* markerPtr);

extern "C" {
  void Blt_DestroyMarkers(Graph* graphPtr);
  void Blt_DrawMarkers(Graph* graphPtr, Drawable drawable, int under);
  ClientData Blt_MakeMarkerTag(Graph* graphPtr, const char* tagName);
  void Blt_MapMarkers(Graph* graphPtr);
  int Blt_MarkerOp(Graph* graphPtr, Tcl_Interp* interp, 
			 int objc, Tcl_Obj* const objv[]);
  void Blt_MarkersToPostScript(Graph* graphPtr, Blt_Ps ps, int under);
  void* Blt_NearestMarker(Graph* graphPtr, int x, int y, int under);
};

// OptionSpecs

static Tk_CustomOptionSetProc CoordsSetProc;
static Tk_CustomOptionGetProc CoordsGetProc;
static Tk_CustomOptionFreeProc CoordsFreeProc;
Tk_ObjCustomOption coordsObjOption =
  {
    "coords", CoordsSetProc, CoordsGetProc, RestoreProc, CoordsFreeProc, NULL
  };

static int CoordsSetProc(ClientData clientData, Tcl_Interp* interp,
			 Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			 int offset, char* savePtr, int flags)
{
  Coords** coordsPtrPtr = (Coords**)(widgRec + offset);
  *(double*)savePtr = *(double*)coordsPtrPtr;

  if (!coordsPtrPtr)
    return TCL_OK;

  int objc;
  Tcl_Obj** objv;
  if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  if (objc == 0) {
    *coordsPtrPtr = NULL;
    return TCL_OK;
  }

  if (objc & 1) {
    Tcl_AppendResult(interp, "odd number of marker coordinates specified",NULL);
    return TCL_ERROR;
  }

  Coords* coordsPtr = (Coords*)calloc(1,sizeof(Coords));
  coordsPtr->num = objc/2;
  coordsPtr->points = (Point2d*)calloc(coordsPtr->num, sizeof(Point2d));

  Point2d* pp = coordsPtr->points;
  for (int ii=0; ii<objc; ii+=2) {
    double x, y;
    if ((GetCoordinate(interp, objv[ii], &x) != TCL_OK) ||
	(GetCoordinate(interp, objv[ii+1], &y) != TCL_OK))
      return TCL_ERROR;
    pp->x = x;
    pp->y = y;
    pp++;
  }

  *coordsPtrPtr = coordsPtr;
  return TCL_OK;
}

static Tcl_Obj* CoordsGetProc(ClientData clientData, Tk_Window tkwin, 
			      char *widgRec, int offset)
{
  Coords* coordsPtr = *(Coords**)(widgRec + offset);

  if (!coordsPtr)
    return Tcl_NewListObj(0, NULL);

  int cnt = coordsPtr->num*2;
  Tcl_Obj** ll = (Tcl_Obj**)calloc(cnt, sizeof(Tcl_Obj*));

  Point2d* pp = coordsPtr->points;
  for (int ii=0; ii<cnt; pp++) {
    ll[ii++] = PrintCoordinate(pp->x);
    ll[ii++] = PrintCoordinate(pp->y);
  }

  Tcl_Obj* listObjPtr = Tcl_NewListObj(cnt, ll);
  free(ll);
  return listObjPtr;
}

static void CoordsFreeProc(ClientData clientData, Tk_Window tkwin,
			   char *ptr)
{
  Coords* coordsPtr = *(Coords**)ptr;
  if (coordsPtr) {
    if (coordsPtr->points)
      free(coordsPtr->points);
    free(coordsPtr);
  }
}

static Tk_CustomOptionSetProc CapStyleSetProc;
static Tk_CustomOptionGetProc CapStyleGetProc;
Tk_ObjCustomOption capStyleObjOption =
  {
    "capStyle", CapStyleSetProc, CapStyleGetProc, NULL, NULL, NULL
  };

static int CapStyleSetProc(ClientData clientData, Tcl_Interp* interp,
			   Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			   int offset, char* save, int flags)
{
  int* ptr = (int*)(widgRec + offset);

  Tk_Uid uid = Tk_GetUid(Tcl_GetString(*objPtr));
  int cap;
  if (Tk_GetCapStyle(interp, uid, &cap) != TCL_OK)
    return TCL_ERROR;
  *ptr = cap;

  return TCL_OK;
}

static Tcl_Obj* CapStyleGetProc(ClientData clientData, Tk_Window tkwin, 
			      char *widgRec, int offset)
{
  int* ptr = (int*)(widgRec + offset);
  return Tcl_NewStringObj(Tk_NameOfCapStyle(*ptr), -1);
}

static Tk_CustomOptionSetProc JoinStyleSetProc;
static Tk_CustomOptionGetProc JoinStyleGetProc;
Tk_ObjCustomOption joinStyleObjOption =
  {
    "joinStyle", JoinStyleSetProc, JoinStyleGetProc, NULL, NULL, NULL
  };

static int JoinStyleSetProc(ClientData clientData, Tcl_Interp* interp,
			   Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			   int offset, char* save, int flags)
{
  int* ptr = (int*)(widgRec + offset);

  Tk_Uid uid = Tk_GetUid(Tcl_GetString(*objPtr));
  int join;
  if (Tk_GetJoinStyle(interp, uid, &join) != TCL_OK)
    return TCL_ERROR;
  *ptr = join;

  return TCL_OK;
}

static Tcl_Obj* JoinStyleGetProc(ClientData clientData, Tk_Window tkwin, 
			      char *widgRec, int offset)
{
  int* ptr = (int*)(widgRec + offset);
  return Tcl_NewStringObj(Tk_NameOfJoinStyle(*ptr), -1);
}

// Create

static int CreateMarker(Graph* graphPtr, Tcl_Interp* interp, 
			int objc, Tcl_Obj* const objv[])
{
  int offset = 5;
  char* name =NULL;
  char ident[128];
  if (objc == 4) {
    offset = 4;
    sprintf_s(ident, 128, "marker%d", graphPtr->nextMarkerId++);
    name = ident;
  }
  else {
    name = Tcl_GetString(objv[4]);
    if (name[0] == '-') {
      offset = 4;
      sprintf_s(ident, 128, "marker%d", graphPtr->nextMarkerId++);
      name = ident;
    }
  }

  int isNew;
  Tcl_HashEntry* hPtr =
    Tcl_CreateHashEntry(&graphPtr->markers.table, name, &isNew);
  if (!isNew) {
    Tcl_AppendResult(graphPtr->interp, "marker \"", name,
		     "\" already exists in \"", Tcl_GetString(objv[0]),
		     "\"", NULL);
    return TCL_ERROR;
  }

  const char* type = Tcl_GetString(objv[3]);
  Marker* markerPtr;
  if (!strcmp(type, "bitmap")) {
    markerPtr = Blt_CreateBitmapProc(graphPtr);
    Blt_GraphSetObjectClass(&markerPtr->obj, CID_MARKER_BITMAP);
  }
  else if (!strcmp(type, "line")) {
    markerPtr = Blt_CreateLineProc(graphPtr);
    Blt_GraphSetObjectClass(&markerPtr->obj, CID_MARKER_LINE);
  }
  else if (!strcmp(type, "polygon")) {
    markerPtr = Blt_CreatePolygonProc(graphPtr);
    Blt_GraphSetObjectClass(&markerPtr->obj, CID_MARKER_POLYGON);
  }
  else if (!strcmp(type, "text")) {
    markerPtr = Blt_CreateTextProc(graphPtr);
    Blt_GraphSetObjectClass(&markerPtr->obj, CID_MARKER_TEXT);
  }
  else if (!strcmp(type, "window")) {
    markerPtr = Blt_CreateWindowProc(graphPtr);
    Blt_GraphSetObjectClass(&markerPtr->obj, CID_MARKER_WINDOW);
  }
  else {
    Tcl_AppendResult(interp, "unknown marker type ", type, NULL);
    return TCL_ERROR;
  }

  markerPtr->obj.name = Blt_Strdup(name);
  markerPtr->obj.graphPtr = graphPtr;

  markerPtr->hashPtr = hPtr;
  Tcl_SetHashValue(hPtr, markerPtr);

  if ((Tk_InitOptions(graphPtr->interp, (char*)markerPtr->ops, markerPtr->optionTable, graphPtr->tkwin) != TCL_OK) || (MarkerObjConfigure(interp, graphPtr, markerPtr, objc-offset, objv+offset) != TCL_OK)) {
    DestroyMarker(markerPtr);
    return TCL_ERROR;
  }

  // Unlike elements, new markers are drawn on top of old markers
  markerPtr->link = Blt_Chain_Prepend(graphPtr->markers.displayList, markerPtr);

  Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
  return TCL_OK;
}

static void DestroyMarker(Marker* markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;

  // If the marker to be deleted is currently displayed below the
  // elements, then backing store needs to be repaired.
  if (ops->drawUnder)
    graphPtr->flags |= CACHE_DIRTY;

  Blt_DeleteBindings(graphPtr->bindTable, markerPtr);

  if (markerPtr->obj.name)
    free((void*)markerPtr->obj.name);

  if (markerPtr->hashPtr)
    Tcl_DeleteHashEntry(markerPtr->hashPtr);

  if (markerPtr->link)
    Blt_Chain_DeleteLink(graphPtr->markers.displayList, markerPtr->link);

  Tk_FreeConfigOptions((char*)markerPtr->ops, markerPtr->optionTable,
		       graphPtr->tkwin);

  (*markerPtr->classPtr->freeProc)(markerPtr);
  free(markerPtr);
}

// Configure

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Marker* markerPtr;
  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK)
    return TCL_ERROR;

  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, (char*)markerPtr->ops, 
				      markerPtr->optionTable,
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
  Marker* markerPtr;
  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK)
    return TCL_ERROR;

  if (objc <= 5) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(graphPtr->interp, (char*)markerPtr->ops, 
				       markerPtr->optionTable, 
				       (objc == 5) ? objv[4] : NULL, 
				       graphPtr->tkwin);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return MarkerObjConfigure(interp, graphPtr, markerPtr, objc-4, objv+4);
}

static int MarkerObjConfigure( Tcl_Interp* interp, Graph* graphPtr,
			       Marker* markerPtr,
			       int objc, Tcl_Obj* const objv[])
{
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)markerPtr->ops, markerPtr->optionTable, 
			objc, objv, graphPtr->tkwin, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    markerPtr->flags |= mask;
    markerPtr->flags |= MAP_ITEM;
    graphPtr->flags |= CACHE_DIRTY;
    if ((*markerPtr->classPtr->configProc)(markerPtr) != TCL_OK)
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

static int BindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_HashSearch iter;
    for (Tcl_HashEntry* hp = 
	   Tcl_FirstHashEntry(&graphPtr->markers.tagTable, &iter); 
	 hp; hp = Tcl_NextHashEntry(&iter)) {

      const char* tag = 
	(const char*)Tcl_GetHashKey(&graphPtr->markers.tagTable, hp);
      Tcl_Obj* objPtr = Tcl_NewStringObj(tag, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }

  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, Blt_MakeMarkerTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}

static int CreateOp(Graph* graphPtr, Tcl_Interp* interp,
		    int objc, Tcl_Obj* const objv[])
{
  if (CreateMarker(graphPtr, interp, objc, objv) != TCL_OK)
    return TCL_ERROR;
  // set in CreateMarker
  // Tcl_SetObjResult(interp, objv[3]);

  return TCL_OK;
}

static int DeleteOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  for (int ii=3; ii<objc; ii++) {
    Marker* markerPtr;
    if (GetMarkerFromObj(NULL, graphPtr, objv[ii], &markerPtr) != TCL_OK) {
      Tcl_AppendResult(interp, "can't find marker \"", 
		       Tcl_GetString(objv[ii]), "\" in \"", 
		       Tk_PathName(graphPtr->tkwin), "\"", NULL);
      return TCL_ERROR;
    }
    markerPtr->flags |= DELETE_PENDING;
    Tcl_EventuallyFree(markerPtr, Blt_FreeMarker);
  }

  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int ExistsOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry* hPtr =
    Tcl_FindHashEntry(&graphPtr->markers.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr));

  return TCL_OK;
}

static int FindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
#define FIND_ENCLOSED	 (1<<0)
#define FIND_OVERLAPPING (1<<1)
  const char* string = Tcl_GetString(objv[3]);
  int mode;
  if (strcmp(string, "enclosed") == 0)
    mode = FIND_ENCLOSED;
  else if (strcmp(string, "overlapping") == 0)
    mode = FIND_OVERLAPPING;
  else {
    Tcl_AppendResult(interp, "bad search type \"", string, 
		     ": should be \"enclosed\", or \"overlapping\"",
		     NULL);
    return TCL_ERROR;
  }

  int left, right, top, bottom;
  if ((Tcl_GetIntFromObj(interp, objv[4], &left) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[5], &top) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[6], &right) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[7], &bottom) != TCL_OK)) {
    return TCL_ERROR;
  }

  Region2d extents;
  if (left < right) {
    extents.left = (double)left;
    extents.right = (double)right;
  }
  else {
    extents.left = (double)right;
    extents.right = (double)left;
  }
  if (top < bottom) {
    extents.top = (double)top;
    extents.bottom = (double)bottom;
  }
  else {
    extents.top = (double)bottom;
    extents.bottom = (double)top;
  }

  int enclosed = (mode == FIND_ENCLOSED);
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;
    if ((markerPtr->flags & DELETE_PENDING) || ops->hide) {
      continue;
    }
    if (IsElementHidden(markerPtr))
      continue;

    if ((*markerPtr->classPtr->regionProc)(markerPtr, &extents, enclosed)) {
      Tcl_Obj* objPtr = Tcl_GetObjResult(interp);
      Tcl_SetStringObj(objPtr, markerPtr->obj.name, -1);
      return TCL_OK;
    }
  }

  Tcl_SetStringObj(Tcl_GetObjResult(interp), "", -1);
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  const char* string = Tcl_GetString(objv[3]);
  if (!strcmp(string, "current")) {
    Marker* markerPtr = (Marker*)Blt_GetCurrentItem(graphPtr->bindTable);

    if (markerPtr == NULL)
      return TCL_OK;

    // Report only on markers
    if ((markerPtr->obj.classId >= CID_MARKER_BITMAP) &&
	(markerPtr->obj.classId <= CID_MARKER_WINDOW))
      Tcl_SetStringObj(Tcl_GetObjResult(interp), markerPtr->obj.name, -1);
  }

  return TCL_OK;
}

static int NamesOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj* listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    for (Blt_ChainLink link=Blt_Chain_FirstLink(graphPtr->markers.displayList); 
	 link; link = Blt_Chain_NextLink(link)) {
      Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
      Tcl_ListObjAppendElement(interp, listObjPtr,
			       Tcl_NewStringObj(markerPtr->obj.name, -1));
    }
  } 
  else {
    for (Blt_ChainLink link=Blt_Chain_FirstLink(graphPtr->markers.displayList); 
	 link; link = Blt_Chain_NextLink(link)) {
      Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
      for (int ii = 3; ii<objc; ii++) {
	const char* pattern = (const char*)Tcl_GetString(objv[ii]);
	if (Tcl_StringMatch(markerPtr->obj.name, pattern)) {
	  Tcl_ListObjAppendElement(interp, listObjPtr,
				   Tcl_NewStringObj(markerPtr->obj.name, -1));
	  break;
	}
      }
    }
  }

  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int RelinkOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Marker* markerPtr;
  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK)
    return TCL_ERROR;
  MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;

  Marker* placePtr =NULL;
  if (objc == 5)
    if (GetMarkerFromObj(interp, graphPtr, objv[4], &placePtr) != TCL_OK)
      return TCL_ERROR;

  Blt_ChainLink link = markerPtr->link;
  Blt_Chain_UnlinkLink(graphPtr->markers.displayList, markerPtr->link);

  Blt_ChainLink place = placePtr ? placePtr->link : NULL;

  const char* string = Tcl_GetString(objv[2]);
  if (string[0] == 'l')
    Blt_Chain_LinkAfter(graphPtr->markers.displayList, link, place);
  else
    Blt_Chain_LinkBefore(graphPtr->markers.displayList, link, place);

  if (ops->drawUnder)
    graphPtr->flags |= CACHE_DIRTY;

  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int TypeOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Marker* markerPtr;
  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK)
    return TCL_ERROR;

  switch (markerPtr->obj.classId) {
  case CID_MARKER_BITMAP:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "bitmap", -1);
    return TCL_OK;
  case CID_MARKER_LINE:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "line", -1);
    return TCL_OK;
  case CID_MARKER_POLYGON:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "polygon", -1);
    return TCL_OK;
  case CID_MARKER_TEXT:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "text", -1);
    return TCL_OK;
  case CID_MARKER_WINDOW:
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "window", -1);
    return TCL_OK;
  default:
  Tcl_SetStringObj(Tcl_GetObjResult(interp), "unknown", -1);
    return TCL_OK;
  }
}

static Blt_OpSpec markerOps[] =
  {
    {"bind",      1, (void*)BindOp,   3, 6, "marker sequence command",},
    {"cget",      2, (void*)CgetOp,   5, 5, "marker option",},
    {"configure", 2, (void*)ConfigureOp, 4, 0,"marker ?option value?...",},
    {"create",    2, (void*)CreateOp, 4, 0, "type ?option value?...",},
    {"delete",    1, (void*)DeleteOp, 3, 0, "?marker?...",},
    {"exists",    1, (void*)ExistsOp, 4, 4, "marker",},
    {"find",      1, (void*)FindOp,   8, 8, "option x1 y1 x2 y2",},
    {"get",       1, (void*)GetOp,    5, 5, "current",},
    {"lower",     1, (void*)RelinkOp, 4, 5, "marker ?afterMarker?",},
    {"names",     1, (void*)NamesOp,  3, 0, "?pattern?...",},
    {"raise",     1, (void*)RelinkOp, 4, 5, "marker ?beforeMarker?",},
    {"type",      1, (void*)TypeOp,   4, 4, "marker",},
  };
static int nMarkerOps = sizeof(markerOps) / sizeof(Blt_OpSpec);

int Blt_MarkerOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  GraphMarkerProc* proc = (GraphMarkerProc*)Blt_GetOpFromObj(interp, nMarkerOps, markerOps, BLT_OP_ARG2, objc, objv,0);
  if (proc == NULL)
    return TCL_ERROR;

  return (*proc) (graphPtr, interp, objc, objv);
}

// Support

static int IsElementHidden(Marker* markerPtr)
{
  Tcl_HashEntry *hPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;
  MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;

  if (ops->elemName) {
    hPtr = Tcl_FindHashEntry(&graphPtr->elements.table, ops->elemName);
    if (hPtr) {
      Element* elemPtr = (Element*)Tcl_GetHashValue(hPtr);
      if (!elemPtr->link || elemPtr->hide)
	return TRUE;
    }
  }
  return FALSE;
}

static double HMap(Axis *axisPtr, double x)
{
  if (x == DBL_MAX)
    x = 1.0;
  else if (x == -DBL_MAX)
    x = 0.0;
  else {
    if (axisPtr->logScale) {
      if (x > 0.0)
	x = log10(x);
      else if (x < 0.0)
	x = 0.0;
    }
    x = NORMALIZE(axisPtr, x);
  }
  if (axisPtr->descending)
    x = 1.0 - x;

  // Horizontal transformation
  return (x * axisPtr->screenRange + axisPtr->screenMin);
}

static double VMap(Axis *axisPtr, double y)
{
  if (y == DBL_MAX)
    y = 1.0;
  else if (y == -DBL_MAX)
    y = 0.0;
  else {
    if (axisPtr->logScale) {
      if (y > 0.0)
	y = log10(y);
      else if (y < 0.0)
	y = 0.0;
    }
    y = NORMALIZE(axisPtr, y);
  }
  if (axisPtr->descending)
    y = 1.0 - y;

  // Vertical transformation
  return (((1.0 - y) * axisPtr->screenRange) + axisPtr->screenMin);
}

Point2d Blt_MapPoint(Point2d *pointPtr, Axis2d *axesPtr)
{
  Graph* graphPtr = axesPtr->y->obj.graphPtr;

  Point2d result;
  if (graphPtr->inverted) {
    result.x = HMap(axesPtr->y, pointPtr->y);
    result.y = VMap(axesPtr->x, pointPtr->x);
  }
  else {
    result.x = HMap(axesPtr->x, pointPtr->x);
    result.y = VMap(axesPtr->y, pointPtr->y);
  }

  return result;
}

static int GetMarkerFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			    Tcl_Obj *objPtr, Marker** markerPtrPtr)
{
  const char* string = Tcl_GetString(objPtr);
  Tcl_HashEntry* hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, string);
  if (hPtr) {
    *markerPtrPtr = (Marker*)Tcl_GetHashValue(hPtr);
    return TCL_OK;
  }
  if (interp) {
    Tcl_AppendResult(interp, "can't find marker \"", string, 
		     "\" in \"", Tk_PathName(graphPtr->tkwin), NULL);
  }

  return TCL_ERROR;
}

void Blt_MarkersToPostScript(Graph* graphPtr, Blt_Ps ps, int under)
{
  for (Blt_ChainLink link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;
    if (markerPtr->classPtr->postscriptProc == NULL)
      continue;

    if (ops->drawUnder != under)
      continue;

    if ((markerPtr->flags & DELETE_PENDING) || ops->hide)
      continue;

    if (IsElementHidden(markerPtr))
      continue;

    Blt_Ps_VarAppend(ps, "\n% Marker \"", markerPtr->obj.name, 
		     "\" is a ", markerPtr->obj.className, ".\n", (char*)NULL);
    (*markerPtr->classPtr->postscriptProc) (markerPtr, ps);
  }
}

void Blt_DrawMarkers(Graph* graphPtr, Drawable drawable, int under)
{
  for (Blt_ChainLink link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
       link; link = Blt_Chain_PrevLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;

    if ((ops->drawUnder != under) ||
	(markerPtr->clipped) ||
	(markerPtr->flags & DELETE_PENDING) ||
	(ops->hide))
      continue;

    if (IsElementHidden(markerPtr))
      continue;

    (*markerPtr->classPtr->drawProc) (markerPtr, drawable);
  }
}

void Blt_ConfigureMarkers(Graph* graphPtr)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    (*markerPtr->classPtr->configProc)(markerPtr);
  }
}

void Blt_MapMarkers(Graph* graphPtr)
{
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;

    if ((markerPtr->flags & DELETE_PENDING) || ops->hide)
      continue;

    if ((graphPtr->flags & MAP_ALL) || (markerPtr->flags & MAP_ITEM)) {
      (*markerPtr->classPtr->mapProc) (markerPtr);
      markerPtr->flags &= ~MAP_ITEM;
    }
  }
}

void Blt_DestroyMarkers(Graph* graphPtr)
{
  Tcl_HashSearch iter;
  for (Tcl_HashEntry* hPtr=Tcl_FirstHashEntry(&graphPtr->markers.table, &iter); 
       hPtr; hPtr = Tcl_NextHashEntry(&iter)) {
    Marker* markerPtr = (Marker*)Tcl_GetHashValue(hPtr);

    // Dereferencing the pointer to the hash table prevents the hash table
    // entry from being automatically deleted.
    markerPtr->hashPtr = NULL;
    DestroyMarker(markerPtr);
  }
  Tcl_DeleteHashTable(&graphPtr->markers.table);
  Tcl_DeleteHashTable(&graphPtr->markers.tagTable);
  Blt_Chain_Destroy(graphPtr->markers.displayList);
}

void* Blt_NearestMarker(Graph* graphPtr, int x, int y, int under)
{
  Point2d point;
  point.x = (double)x;
  point.y = (double)y;
  for (Blt_ChainLink link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
       link; link = Blt_Chain_NextLink(link)) {
    Marker* markerPtr = (Marker*)Blt_Chain_GetValue(link);
    MarkerOptions* ops = (MarkerOptions*)markerPtr->ops;

    if ((markerPtr->flags & (DELETE_PENDING|MAP_ITEM)) || 
	(ops->hide))
      continue;

    if (IsElementHidden(markerPtr))
      continue;

    if ((ops->drawUnder == under) && (ops->state == BLT_STATE_NORMAL))
      if ((*markerPtr->classPtr->pointProc) (markerPtr, &point))
	return markerPtr;
  }
  return NULL;
}

ClientData Blt_MakeMarkerTag(Graph* graphPtr, const char* tagName)
{
  int isNew;
  Tcl_HashEntry *hPtr =
    Tcl_CreateHashEntry(&graphPtr->markers.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->markers.tagTable, hPtr);
}

void Blt_FreeMarker(char* dataPtr) 
{
  Marker* markerPtr = (Marker*)dataPtr;
  DestroyMarker(markerPtr);
}

int Blt_BoxesDontOverlap(Graph* graphPtr, Region2d *extsPtr)
{
  return (((double)graphPtr->right < extsPtr->left) ||
	  ((double)graphPtr->bottom < extsPtr->top) ||
	  (extsPtr->right < (double)graphPtr->left) ||
	  (extsPtr->bottom < (double)graphPtr->top));
}

static Tcl_Obj* PrintCoordinate(double x)
{
  if (x == DBL_MAX)
    return Tcl_NewStringObj("+Inf", -1);
  else if (x == -DBL_MAX)
    return Tcl_NewStringObj("-Inf", -1);
  else
    return Tcl_NewDoubleObj(x);
}

static int GetCoordinate(Tcl_Interp* interp, Tcl_Obj *objPtr, double *valuePtr)
{
  const char* expr = Tcl_GetString(objPtr);
  char c = expr[0];
  if ((c == 'I') && (strcmp(expr, "Inf") == 0))
    *valuePtr = DBL_MAX;		/* Elastic upper bound */
  else if ((c == '-') && (expr[1] == 'I') && (strcmp(expr, "-Inf") == 0))
    *valuePtr = -DBL_MAX;		/* Elastic lower bound */
  else if ((c == '+') && (expr[1] == 'I') && (strcmp(expr, "+Inf") == 0))
    *valuePtr = DBL_MAX;		/* Elastic upper bound */
  else if (Blt_ExprDoubleFromObj(interp, objPtr, valuePtr) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}
