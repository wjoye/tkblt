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

#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltGrElem.h"
#include "bltConfig.h"
#include "bltGrMarker.h"
#include "bltGrMarkerBitmap.h"
#include "bltGrMarkerLine.h"
#include "bltGrMarkerPolygon.h"
#include "bltGrMarkerText.h"
#include "bltGrMarkerWindow.h"

#define NORMALIZE(A,x) 	(((x) - (A)->axisRange.min) * (A)->axisRange.scale)

// Defs

typedef int (GraphMarkerProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			      Tcl_Obj* const objv[]);

static int ParseCoordinates(Tcl_Interp* interp, Marker *markerPtr,
			    int objc, Tcl_Obj* const objv[]);
static Tcl_Obj* PrintCoordinate(double x);

// OptionSpecs

static Tk_CustomOptionSetProc CoordsSetProc;
static Tk_CustomOptionGetProc CoordsGetProc;
Tk_ObjCustomOption coordsObjOption =
  {
    "coords", CoordsSetProc, CoordsGetProc, NULL, NULL, NULL
  };

static int CoordsSetProc(ClientData clientData, Tcl_Interp* interp,
			 Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			 int offset, char* save, int flags)
{
  Marker *markerPtr = (Marker *)widgRec;

  int objc;
  Tcl_Obj **objv;
  if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
    return TCL_ERROR;

  if (objc == 0)
    return TCL_OK;

  return ParseCoordinates(interp, markerPtr, objc, objv);
}

static Tcl_Obj* CoordsGetProc(ClientData clientData, Tk_Window tkwin, 
			      char *widgRec, int offset)
{
  Marker* markerPtr = (Marker*)widgRec;

  int cnt = markerPtr->nWorldPts*2;
  Tcl_Obj** ll = calloc(cnt, sizeof(Tcl_Obj*));

  Point2d* pp = markerPtr->worldPts;
  for (int ii=0; ii < markerPtr->nWorldPts*2; pp++) {
    ll[ii++] = PrintCoordinate(pp->x);
    ll[ii++] = PrintCoordinate(pp->y);
  }

  Tcl_Obj* listObjPtr = Tcl_NewListObj(cnt, ll);
  free(ll);
  return listObjPtr;
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

static Blt_OptionParseProc ObjToCoordsProc;
static Blt_OptionPrintProc CoordsToObjProc;
static Blt_OptionFreeProc FreeCoordsProc;
Blt_CustomOption coordsOption =
  {
    ObjToCoordsProc, CoordsToObjProc, FreeCoordsProc, NULL
  };

static Blt_OptionFreeProc FreeColorPairProc;
static Blt_OptionParseProc ObjToColorPairProc;
static Blt_OptionPrintProc ColorPairToObjProc;
Blt_CustomOption colorPairOption =
  {
    ObjToColorPairProc, ColorPairToObjProc, FreeColorPairProc, (ClientData)0
  };

static int GetCoordinate(Tcl_Interp* interp, Tcl_Obj *objPtr, double *valuePtr)
{
  char c;
  const char* expr;
    
  expr = Tcl_GetString(objPtr);
  c = expr[0];
  if ((c == 'I') && (strcmp(expr, "Inf") == 0)) {
    *valuePtr = DBL_MAX;		/* Elastic upper bound */
  } else if ((c == '-') && (expr[1] == 'I') && (strcmp(expr, "-Inf") == 0)) {
    *valuePtr = -DBL_MAX;		/* Elastic lower bound */
  } else if ((c == '+') && (expr[1] == 'I') && (strcmp(expr, "+Inf") == 0)) {
    *valuePtr = DBL_MAX;		/* Elastic upper bound */
  } else if (Blt_ExprDoubleFromObj(interp, objPtr, valuePtr) != TCL_OK) {
    return TCL_ERROR;
  }
  return TCL_OK;
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

static int ParseCoordinates(Tcl_Interp* interp, Marker *markerPtr,
			    int objc, Tcl_Obj* const objv[])
{
  int nWorldPts;
  int minArgs, maxArgs;
  Point2d *worldPts;
  int i;

  if (objc == 0) {
    return TCL_OK;
  }
  if (objc & 1) {
    Tcl_AppendResult(interp, "odd number of marker coordinates specified",
		     (char*)NULL);
    return TCL_ERROR;
  }
  switch (markerPtr->obj.classId) {
  case CID_MARKER_LINE:
    minArgs = 4, maxArgs = 0;
    break;
  case CID_MARKER_POLYGON:
    minArgs = 6, maxArgs = 0;
    break;
  case CID_MARKER_WINDOW:
  case CID_MARKER_TEXT:
    minArgs = 2, maxArgs = 2;
    break;
  case CID_MARKER_IMAGE:
  case CID_MARKER_BITMAP:
    minArgs = 2, maxArgs = 4;
    break;
  default:
    Tcl_AppendResult(interp, "unknown marker type", (char*)NULL);
    return TCL_ERROR;
  }

  if (objc < minArgs) {
    Tcl_AppendResult(interp, "too few marker coordinates specified",
		     (char*)NULL);
    return TCL_ERROR;
  }
  if ((maxArgs > 0) && (objc > maxArgs)) {
    Tcl_AppendResult(interp, "too many marker coordinates specified",
		     (char*)NULL);
    return TCL_ERROR;
  }
  nWorldPts = objc / 2;
  worldPts = malloc(nWorldPts * sizeof(Point2d));
  if (worldPts == NULL) {
    Tcl_AppendResult(interp, "can't allocate new coordinate array",
		     (char*)NULL);
    return TCL_ERROR;
  }

  {
    Point2d *pp;

    pp = worldPts;
    for (i = 0; i < objc; i += 2) {
      double x, y;
	    
      if ((GetCoordinate(interp, objv[i], &x) != TCL_OK) ||
	  (GetCoordinate(interp, objv[i + 1], &y) != TCL_OK)) {
	free(worldPts);
	return TCL_ERROR;
      }
      pp->x = x, pp->y = y, pp++;
    }
  }
  /* Don't free the old coordinate array until we've parsed the new
   * coordinates without errors.  */
  if (markerPtr->worldPts != NULL) {
    free(markerPtr->worldPts);
  }
  markerPtr->worldPts = worldPts;
  markerPtr->nWorldPts = nWorldPts;
  markerPtr->flags |= MAP_ITEM;
  return TCL_OK;
}

static void FreeCoordsProc(ClientData clientData, Display *display,
			   char* widgRec, int offset)
{
  Marker *markerPtr = (Marker *)widgRec;
  Point2d **pointsPtr = (Point2d **)(widgRec + offset);

  if (*pointsPtr != NULL) {
    free(*pointsPtr);
    *pointsPtr = NULL;
  }
  markerPtr->nWorldPts = 0;
}

static int ObjToCoordsProc(ClientData clientData, Tcl_Interp* interp,
			   Tk_Window tkwin, Tcl_Obj *objPtr,
			   char* widgRec, int offset, int flags)
{
  Marker *markerPtr = (Marker *)widgRec;
  int objc;
  Tcl_Obj **objv;

  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
    return TCL_ERROR;
  }
  if (objc == 0) {
    return TCL_OK;
  }
  return ParseCoordinates(interp, markerPtr, objc, objv);
}

static Tcl_Obj* CoordsToObjProc(ClientData clientData, Tcl_Interp* interp, 
				Tk_Window tkwin, char* widgRec, 
				int offset, int flags)
{
  Marker *markerPtr = (Marker *)widgRec;
  Tcl_Obj *listObjPtr;
  Point2d *pp, *pend;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  for (pp = markerPtr->worldPts, pend = pp + markerPtr->nWorldPts; pp < pend;
       pp++) {
    Tcl_ListObjAppendElement(interp, listObjPtr, PrintCoordinate(pp->x));
    Tcl_ListObjAppendElement(interp, listObjPtr, PrintCoordinate(pp->y));
  }
  return listObjPtr;
}

static int GetColorPair(Tcl_Interp* interp, Tk_Window tkwin,
			Tcl_Obj *fgObjPtr, Tcl_Obj *bgObjPtr,
			ColorPair *pairPtr, int allowDefault)
{
  XColor* fgColor, *bgColor;
  const char* string;

  fgColor = bgColor = NULL;
  if (fgObjPtr != NULL) {
    int length;

    string = Tcl_GetStringFromObj(fgObjPtr, &length);
    if (string[0] == '\0') {
      fgColor = NULL;
    } else if ((allowDefault) && (string[0] == 'd') &&
	       (strncmp(string, "defcolor", length) == 0)) {
      fgColor = COLOR_DEFAULT;
    } else {
      fgColor = Tk_AllocColorFromObj(interp, tkwin, fgObjPtr);
      if (fgColor == NULL) {
	return TCL_ERROR;
      }
    }
  }
  if (bgObjPtr != NULL) {
    int length;

    string = Tcl_GetStringFromObj(bgObjPtr, &length);
    if (string[0] == '\0') {
      bgColor = NULL;
    } else if ((allowDefault) && (string[0] == 'd') &&
	       (strncmp(string, "defcolor", length) == 0)) {
      bgColor = COLOR_DEFAULT;
    } else {
      bgColor = Tk_AllocColorFromObj(interp, tkwin, bgObjPtr);
      if (bgColor == NULL) {
	return TCL_ERROR;
      }
    }
  }
  if (pairPtr->fgColor != NULL) {
    Tk_FreeColor(pairPtr->fgColor);
  }
  if (pairPtr->bgColor != NULL) {
    Tk_FreeColor(pairPtr->bgColor);
  }
  pairPtr->fgColor = fgColor;
  pairPtr->bgColor = bgColor;
  return TCL_OK;
}

void Blt_FreeColorPair(ColorPair *pairPtr)
{
  if ((pairPtr->bgColor != NULL) && (pairPtr->bgColor != COLOR_DEFAULT)) {
    Tk_FreeColor(pairPtr->bgColor);
  }
  if ((pairPtr->fgColor != NULL) && (pairPtr->fgColor != COLOR_DEFAULT)) {
    Tk_FreeColor(pairPtr->fgColor);
  }
  pairPtr->bgColor = pairPtr->fgColor = NULL;
}

static void FreeColorPairProc(ClientData clientData, Display *display,
			      char* widgRec, int offset)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
  Blt_FreeColorPair(pairPtr);
}

static int ObjToColorPairProc(ClientData clientData, Tcl_Interp* interp,
			      Tk_Window tkwin, Tcl_Obj *objPtr, char* widgRec,
			      int offset, int flags)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
  long longValue = (long)clientData;
  int bool;
  int objc;
  Tcl_Obj **objv;

  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
    return TCL_ERROR;
  }
  if (objc > 2) {
    Tcl_AppendResult(interp, "too many names in colors list", 
		     (char*)NULL);
    return TCL_ERROR;
  }
  if (objc == 0) {
    Blt_FreeColorPair(pairPtr);
    return TCL_OK;
  }
  bool = (int)longValue;
  if (objc == 1) {
    if (GetColorPair(interp, tkwin, objv[0], NULL, pairPtr, bool) 
	!= TCL_OK) {
      return TCL_ERROR;
    }
  } else {
    if (GetColorPair(interp, tkwin, objv[0], objv[1], pairPtr, bool) 
	!= TCL_OK) {
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}

static const char* NameOfColor(XColor* colorPtr)
{
  if (colorPtr == NULL) {
    return "";
  } else if (colorPtr == COLOR_DEFAULT) {
    return "defcolor";
  } else {
    return Tk_NameOfColor(colorPtr);
  }
}

static Tcl_Obj* ColorPairToObjProc(ClientData clientData, Tcl_Interp* interp,
				   Tk_Window tkwin, char* widgRec,
				   int offset, int flags)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
  Tcl_Obj *listObjPtr;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, 
			   Tcl_NewStringObj(NameOfColor(pairPtr->fgColor), -1));
  Tcl_ListObjAppendElement(interp, listObjPtr,
			   Tcl_NewStringObj(NameOfColor(pairPtr->bgColor), -1));
  return listObjPtr;
}

static int IsElementHidden(Marker *markerPtr)
{
  Tcl_HashEntry *hPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  /* Look up the named element and see if it's hidden */
  hPtr = Tcl_FindHashEntry(&graphPtr->elements.table, markerPtr->elemName);
  if (hPtr != NULL) {
    Element* elemPtr;
	
    elemPtr = Tcl_GetHashValue(hPtr);
    if ((elemPtr->link == NULL) || (elemPtr->flags & HIDE)) {
      return TRUE;
    }
  }
  return FALSE;
}

static double HMap(Axis *axisPtr, double x)
{
  if (x == DBL_MAX) {
    x = 1.0;
  } else if (x == -DBL_MAX) {
    x = 0.0;
  } else {
    if (axisPtr->logScale) {
      if (x > 0.0) {
	x = log10(x);
      } else if (x < 0.0) {
	x = 0.0;
      }
    }
    x = NORMALIZE(axisPtr, x);
  }
  if (axisPtr->descending) {
    x = 1.0 - x;
  }
  /* Horizontal transformation */
  return (x * axisPtr->screenRange + axisPtr->screenMin);
}

static double VMap(Axis *axisPtr, double y)
{
  if (y == DBL_MAX) {
    y = 1.0;
  } else if (y == -DBL_MAX) {
    y = 0.0;
  } else {
    if (axisPtr->logScale) {
      if (y > 0.0) {
	y = log10(y);
      } else if (y < 0.0) {
	y = 0.0;
      }
    }
    y = NORMALIZE(axisPtr, y);
  }
  if (axisPtr->descending) {
    y = 1.0 - y;
  }
  /* Vertical transformation. */
  return (((1.0 - y) * axisPtr->screenRange) + axisPtr->screenMin);
}

Point2d Blt_MapPoint(Point2d *pointPtr, Axis2d *axesPtr)
{
  Point2d result;
  Graph* graphPtr = axesPtr->y->obj.graphPtr;

  if (graphPtr->inverted) {
    result.x = HMap(axesPtr->y, pointPtr->y);
    result.y = VMap(axesPtr->x, pointPtr->x);
  } else {
    result.x = HMap(axesPtr->x, pointPtr->x);
    result.y = VMap(axesPtr->y, pointPtr->y);
  }
  return result;
}

static Marker* CreateMarker(Graph* graphPtr, const char* name, ClassId classId)
{    
  Marker *markerPtr;
  switch (classId) {
  case CID_MARKER_BITMAP:
    markerPtr = Blt_CreateBitmapProc(); /* bitmap */
    break;
  case CID_MARKER_LINE:
    markerPtr = Blt_CreateLineProc();	/* line */
    break;
  case CID_MARKER_IMAGE:
    return NULL; /* not supported */
    break;
  case CID_MARKER_TEXT:
    markerPtr = Blt_CreateTextProc();	/* text */
    break;
  case CID_MARKER_POLYGON:
    markerPtr = Blt_CreatePolygonProc(); /* polygon */
    break;
  case CID_MARKER_WINDOW:
    markerPtr = Blt_CreateWindowProc(); /* window */
    break;
  default:
    return NULL;
  }
  markerPtr->obj.graphPtr = graphPtr;
  markerPtr->drawUnder = FALSE;
  markerPtr->flags |= MAP_ITEM;
  markerPtr->obj.name = Blt_Strdup(name);
  Blt_GraphSetObjectClass(&markerPtr->obj, classId);
  return markerPtr;
}

static void DestroyMarker(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (markerPtr->drawUnder) {
    /* If the marker to be deleted is currently displayed below the
     * elements, then backing store needs to be repaired. */
    graphPtr->flags |= CACHE_DIRTY;
  }
  /* 
   * Call the marker's type-specific deallocation routine. We do it first
   * while all the marker fields are still valid.
   */
  (*markerPtr->classPtr->freeProc)(markerPtr);

  /* Dump any bindings that might be registered for the marker. */
  Blt_DeleteBindings(graphPtr->bindTable, markerPtr);

  /* Release all the X resources associated with the marker. */
  Blt_FreeOptions(markerPtr->classPtr->configSpecs, (char*)markerPtr,
		  graphPtr->display, 0);

  if (markerPtr->hashPtr != NULL) {
    Tcl_DeleteHashEntry(markerPtr->hashPtr);
  }
  if (markerPtr->link != NULL) {
    Blt_Chain_DeleteLink(graphPtr->markers.displayList, markerPtr->link);
  }
  if (markerPtr->obj.name != NULL) {
    free((void*)(markerPtr->obj.name));
  }
  free(markerPtr);
}

static int GetMarkerFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			    Tcl_Obj *objPtr, Marker **markerPtrPtr)
{
  Tcl_HashEntry *hPtr;
  const char* string;

  string = Tcl_GetString(objPtr);
  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, string);
  if (hPtr != NULL) {
    *markerPtrPtr = Tcl_GetHashValue(hPtr);
    return TCL_OK;
  }
  if (interp != NULL) {
    Tcl_AppendResult(interp, "can't find marker \"", string, 
		     "\" in \"", Tk_PathName(graphPtr->tkwin), (char*)NULL);
  }
  return TCL_ERROR;
}

static int RenameMarker(Graph* graphPtr, Marker *markerPtr, 
			const char* oldName, const char* newName)
{
  int isNew;
  Tcl_HashEntry *hPtr;

  /* Rename the marker only if no marker already exists by that name */
  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.table, newName, &isNew);
  if (!isNew) {
    Tcl_AppendResult(graphPtr->interp, "can't rename marker: \"", newName,
		     "\" already exists", (char*)NULL);
    return TCL_ERROR;
  }
  markerPtr->obj.name = Blt_Strdup(newName);
  markerPtr->hashPtr = hPtr;
  Tcl_SetHashValue(hPtr, (char*)markerPtr);

  /* Delete the old hash entry */
  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, oldName);
  Tcl_DeleteHashEntry(hPtr);
  if (oldName != NULL) {
    free((void*)oldName);
  }
  return TCL_OK;
}

static int NamesOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Marker *markerPtr;

      markerPtr = Blt_Chain_GetValue(link);
      Tcl_ListObjAppendElement(interp, listObjPtr,
			       Tcl_NewStringObj(markerPtr->obj.name, -1));
    }
  } else {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Marker *markerPtr;
      int i;

      markerPtr = Blt_Chain_GetValue(link);
      for (i = 3; i < objc; i++) {
	const char* pattern;

	pattern = Tcl_GetString(objv[i]);
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

static int BindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_HashEntry *hp;
    Tcl_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hp = Tcl_FirstHashEntry(&graphPtr->markers.tagTable, &iter);
	 hp != NULL; hp = Tcl_NextHashEntry(&iter)) {
      const char* tag;
      Tcl_Obj *objPtr;

      tag = Tcl_GetHashKey(&graphPtr->markers.tagTable, hp);
      objPtr = Tcl_NewStringObj(tag, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
				      Blt_MakeMarkerTag(graphPtr, Tcl_GetString(objv[3])),
				      objc - 4, objv + 4);
}

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;

  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, 
				markerPtr->classPtr->configSpecs, (char*)markerPtr, objv[4], 0) 
      != TCL_OK) {
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  Tcl_Obj *const *options;
  const char* oldName;
  const char* string;
  int flags = BLT_CONFIG_OBJV_ONLY;
  int nNames, nOpts;
  int i;
  int under;

  markerPtr = NULL;			/* Suppress compiler warning. */

  /* Figure out where the option value pairs begin */
  objc -= 3;
  objv += 3;
  for (i = 0; i < objc; i++) {
    string = Tcl_GetString(objv[i]);
    if (string[0] == '-') {
      break;
    }
    if (GetMarkerFromObj(interp, graphPtr, objv[i], &markerPtr) != TCL_OK) {
      return TCL_ERROR;
    }
  }
  nNames = i;				/* # of element names specified */
  nOpts = objc - i;			/* # of options specified */
  options = objv + nNames;		/* Start of options in objv  */
    
  for (i = 0; i < nNames; i++) {
    GetMarkerFromObj(interp, graphPtr, objv[i], &markerPtr);
    if (nOpts == 0) {
      return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
				      markerPtr->classPtr->configSpecs, (char*)markerPtr, 
				      (Tcl_Obj *)NULL, flags);
    } else if (nOpts == 1) {
      return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin,
				      markerPtr->classPtr->configSpecs, (char*)markerPtr, 
				      options[0], flags);
    }
    /* Save the old marker name. */
    oldName = markerPtr->obj.name;
    under = markerPtr->drawUnder;
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, 
				   markerPtr->classPtr->configSpecs, nOpts, options, 
				   (char*)markerPtr, flags) != TCL_OK) {
      return TCL_ERROR;
    }
    if (oldName != markerPtr->obj.name) {
      if (RenameMarker(graphPtr, markerPtr, oldName, markerPtr->obj.name)
	  != TCL_OK) {
	markerPtr->obj.name = oldName;
	return TCL_ERROR;
      }
    }
    if ((*markerPtr->classPtr->configProc) (markerPtr) != TCL_OK) {

      return TCL_ERROR;
    }
    if (markerPtr->drawUnder != under) {
      graphPtr->flags |= CACHE_DIRTY;
    }
  }
  return TCL_OK;
}

static int CreateOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  Tcl_HashEntry *hPtr;
  int isNew;
  ClassId classId;
  int i;
  const char* name;
  char ident[200];
  const char* string;
  char c;

  string = Tcl_GetString(objv[3]);
  c = string[0];
  /* Create the new marker based upon the given type */
  if ((c == 't') && (strcmp(string, "text") == 0)) {
    classId = CID_MARKER_TEXT;
  } else if ((c == 'l') && (strcmp(string, "line") == 0)) {
    classId = CID_MARKER_LINE;
  } else if ((c == 'p') && (strcmp(string, "polygon") == 0)) {
    classId = CID_MARKER_POLYGON;
  } else if ((c == 'i') && (strcmp(string, "image") == 0)) {
    classId = CID_MARKER_IMAGE;
  } else if ((c == 'b') && (strcmp(string, "bitmap") == 0)) {
    classId = CID_MARKER_BITMAP;
  } else if ((c == 'w') && (strcmp(string, "window") == 0)) {
    classId = CID_MARKER_WINDOW;
  } else {
    Tcl_AppendResult(interp, "unknown marker type \"", string,
		     "\": should be \"text\", \"line\", \"polygon\", \"bitmap\", \"image\", or \
\"window\"", (char*)NULL);
    return TCL_ERROR;
  }
  /* Scan for "-name" option. We need it for the component name */
  name = NULL;
  for (i = 4; i < objc; i += 2) {
    int length;

    string = Tcl_GetStringFromObj(objv[i], &length);
    if ((length > 1) && (strncmp(string, "-name", length) == 0)) {
      name = Tcl_GetString(objv[i + 1]);
      break;
    }
  }
  /* If no name was given for the marker, make up one. */
  if (name == NULL) {
    sprintf_s(ident, 200, "marker%d", graphPtr->nextMarkerId++);
    name = ident;
  } else if (name[0] == '-') {
    Tcl_AppendResult(interp, "name of marker \"", name, 
		     "\" can't start with a '-'", (char*)NULL);
    return TCL_ERROR;
  }
  markerPtr = CreateMarker(graphPtr, name, classId);
  if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, name, 
				    markerPtr->obj.className, markerPtr->classPtr->configSpecs, 
				    objc - 4, objv + 4, (char*)markerPtr, 0) != TCL_OK) {
    DestroyMarker(markerPtr);
    return TCL_ERROR;
  }
  if ((*markerPtr->classPtr->configProc) (markerPtr) != TCL_OK) {
    DestroyMarker(markerPtr);
    return TCL_ERROR;
  }
  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.table, name, &isNew);
  if (!isNew) {
    Marker *oldPtr;
    /*
     * Marker by the same name already exists.  Delete the old marker and
     * it's list entry.  But save the hash entry.
     */
    oldPtr = Tcl_GetHashValue(hPtr);
    oldPtr->hashPtr = NULL;
    DestroyMarker(oldPtr);
  }
  Tcl_SetHashValue(hPtr, markerPtr);
  markerPtr->hashPtr = hPtr;
  /* Unlike elements, new markers are drawn on top of old markers. */
  markerPtr->link = Blt_Chain_Prepend(graphPtr->markers.displayList,markerPtr);
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
  return TCL_OK;
}

static int DeleteOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  int i;

  for (i = 3; i < objc; i++) {
    Marker *markerPtr;

    if (GetMarkerFromObj(NULL, graphPtr, objv[i], &markerPtr) == TCL_OK) {
      markerPtr->flags |= DELETE_PENDING;
      Tcl_EventuallyFree(markerPtr, Blt_FreeMarker);
    }
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  const char* string;

  string = Tcl_GetString(objv[3]);
  if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
    markerPtr = (Marker *)Blt_GetCurrentItem(graphPtr->bindTable);
    if (markerPtr == NULL) {
      return TCL_OK;		/* Report only on markers. */

    }
    if ((markerPtr->obj.classId >= CID_MARKER_BITMAP) &&
	(markerPtr->obj.classId <= CID_MARKER_WINDOW))	    {
      Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		       markerPtr->obj.name, -1);
    }
  }
  return TCL_OK;
}

static int RelinkOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Blt_ChainLink link, place;
  Marker *markerPtr;
  const char* string;

  /* Find the marker to be raised or lowered. */
  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  /* Right now it's assumed that all markers are always in the display
     list. */
  link = markerPtr->link;
  Blt_Chain_UnlinkLink(graphPtr->markers.displayList, markerPtr->link);

  place = NULL;
  if (objc == 5) {
    if (GetMarkerFromObj(interp, graphPtr, objv[4], &markerPtr) != TCL_OK) {
      return TCL_ERROR;
    }
    place = markerPtr->link;
  }

  /* Link the marker at its new position. */
  string = Tcl_GetString(objv[2]);
  if (string[0] == 'l') {
    Blt_Chain_LinkAfter(graphPtr->markers.displayList, link, place);
  } else {
    Blt_Chain_LinkBefore(graphPtr->markers.displayList, link, place);
  }
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int FindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Blt_ChainLink link;
  Region2d extents;
  const char* string;
  int enclosed;
  int left, right, top, bottom;
  int mode;

#define FIND_ENCLOSED	 (1<<0)
#define FIND_OVERLAPPING (1<<1)
  string = Tcl_GetString(objv[3]);
  if (strcmp(string, "enclosed") == 0) {
    mode = FIND_ENCLOSED;
  } else if (strcmp(string, "overlapping") == 0) {
    mode = FIND_OVERLAPPING;
  } else {
    Tcl_AppendResult(interp, "bad search type \"", string, 
		     ": should be \"enclosed\", or \"overlapping\"", (char*)NULL);
    return TCL_ERROR;
  }

  if ((Tcl_GetIntFromObj(interp, objv[4], &left) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[5], &top) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[6], &right) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[7], &bottom) != TCL_OK)) {
    return TCL_ERROR;
  }
  if (left < right) {
    extents.left = (double)left;
    extents.right = (double)right;
  } else {
    extents.left = (double)right;
    extents.right = (double)left;
  }
  if (top < bottom) {
    extents.top = (double)top;
    extents.bottom = (double)bottom;
  } else {
    extents.top = (double)bottom;
    extents.bottom = (double)top;
  }
  enclosed = (mode == FIND_ENCLOSED);
  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if ((markerPtr->flags & DELETE_PENDING) || markerPtr->hide) {
      continue;
    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    if ((*markerPtr->classPtr->regionProc)(markerPtr, &extents, enclosed)) {
      Tcl_Obj *objPtr;

      objPtr = Tcl_GetObjResult(interp);
      Tcl_SetStringObj(objPtr, markerPtr->obj.name, -1);
      return TCL_OK;
    }
  }
  Tcl_SetStringObj(Tcl_GetObjResult(interp), "", -1);
  return TCL_OK;
}

static int ExistsOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
  return TCL_OK;
}

static int TypeOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  const char* type;

  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  switch (markerPtr->obj.classId) {
  case CID_MARKER_BITMAP:	type = "bitmap";	break;
  case CID_MARKER_IMAGE:	type = "image";		break;
  case CID_MARKER_LINE:	type = "line";		break;
  case CID_MARKER_POLYGON:	type = "polygon";	break;
  case CID_MARKER_TEXT:	type = "text";		break;
  case CID_MARKER_WINDOW:	type = "window";	break;
  default:			type = "???";		break;
  }
  Tcl_SetStringObj(Tcl_GetObjResult(interp), type, -1);
  return TCL_OK;
}

static Blt_OpSpec markerOps[] =
  {
    {"bind",      1, BindOp,   3, 6, "marker sequence command",},
    {"cget",      2, CgetOp,   5, 5, "marker option",},
    {"configure", 2, ConfigureOp, 4, 0,"marker ?marker?... ?option value?...",},
    {"create",    2, CreateOp, 4, 0, "type ?option value?...",},
    {"delete",    1, DeleteOp, 3, 0, "?marker?...",},
    {"exists",    1, ExistsOp, 4, 4, "marker",},
    {"find",      1, FindOp,   8, 8, "enclosed|overlapping x1 y1 x2 y2",},
    {"get",       1, GetOp,    4, 4, "name",},
    {"lower",     1, RelinkOp, 4, 5, "marker ?afterMarker?",},
    {"names",     1, NamesOp,  3, 0, "?pattern?...",},
    {"raise",     1, RelinkOp, 4, 5, "marker ?beforeMarker?",},
    {"type",      1, TypeOp,   4, 4, "marker",},
  };
static int nMarkerOps = sizeof(markerOps) / sizeof(Blt_OpSpec);

int Blt_MarkerOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  GraphMarkerProc *proc;
  int result;

  proc = Blt_GetOpFromObj(interp, nMarkerOps, markerOps, BLT_OP_ARG2, 
			  objc, objv,0);
  if (proc == NULL) {
    return TCL_ERROR;
  }
  result = (*proc) (graphPtr, interp, objc, objv);
  return result;
}

void Blt_MarkersToPostScript(Graph* graphPtr, Blt_Ps ps, int under)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if ((markerPtr->classPtr->postscriptProc == NULL) || 
	(markerPtr->nWorldPts == 0)) {
      continue;
    }
    if (markerPtr->drawUnder != under) {
      continue;
    }
    if ((markerPtr->flags & DELETE_PENDING) || markerPtr->hide) {
      continue;
    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    Blt_Ps_VarAppend(ps, "\n% Marker \"", markerPtr->obj.name, 
		     "\" is a ", markerPtr->obj.className, ".\n", (char*)NULL);
    (*markerPtr->classPtr->postscriptProc) (markerPtr, ps);
  }
}

void Blt_DrawMarkers(Graph* graphPtr, Drawable drawable, int under)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);

    if ((markerPtr->nWorldPts == 0) || 
	(markerPtr->drawUnder != under) ||
	(markerPtr->clipped) ||
	(markerPtr->flags & DELETE_PENDING) ||
	(markerPtr->hide)) {
      continue;
    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    (*markerPtr->classPtr->drawProc) (markerPtr, drawable);
  }
}

void
Blt_ConfigureMarkers(Graph* graphPtr)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    (*markerPtr->classPtr->configProc) (markerPtr);
  }
}

void Blt_MapMarkers(Graph* graphPtr)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if (markerPtr->nWorldPts == 0) {
      continue;
    }
    if ((markerPtr->flags & DELETE_PENDING) || markerPtr->hide) {
      continue;
    }
    if ((graphPtr->flags & MAP_ALL) || (markerPtr->flags & MAP_ITEM)) {
      (*markerPtr->classPtr->mapProc) (markerPtr);
      markerPtr->flags &= ~MAP_ITEM;
    }
  }
}

void Blt_DestroyMarkers(Graph* graphPtr)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch iter;

  for (hPtr = Tcl_FirstHashEntry(&graphPtr->markers.table, &iter); 
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
    Marker *markerPtr;

    markerPtr = Tcl_GetHashValue(hPtr);
    /*
     * Dereferencing the pointer to the hash table prevents the hash table
     * entry from being automatically deleted.
     */
    markerPtr->hashPtr = NULL;
    DestroyMarker(markerPtr);
  }
  Tcl_DeleteHashTable(&graphPtr->markers.table);
  Tcl_DeleteHashTable(&graphPtr->markers.tagTable);
  Blt_Chain_Destroy(graphPtr->markers.displayList);
}

Marker* Blt_NearestMarker(Graph* graphPtr, int x, int y, int under)
{
  Blt_ChainLink link;
  Point2d point;

  point.x = (double)x;
  point.y = (double)y;
  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if ((markerPtr->nWorldPts == 0) ||
	(markerPtr->flags & (DELETE_PENDING|MAP_ITEM)) ||
	(markerPtr->hide)) {
      continue;			/* Don't consider markers that are
				 * pending to be mapped. Even if the
				 * marker has already been mapped, the
				 * coordinates could be invalid now.
				 * Better to pick no marker than the
				 * wrong marker. */

    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    if ((markerPtr->drawUnder == under) && 
	(markerPtr->state == BLT_STATE_NORMAL)) {
      if ((*markerPtr->classPtr->pointProc) (markerPtr, &point)) {
	return markerPtr;
      }
    }
  }
  return NULL;
}

ClientData Blt_MakeMarkerTag(Graph* graphPtr, const char* tagName)
{
  Tcl_HashEntry *hPtr;
  int isNew;

  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->markers.tagTable, hPtr);
}

void Blt_FreeMarker(char* dataPtr) 
{
  Marker *markerPtr = (Marker *)dataPtr;
  DestroyMarker(markerPtr);
}

int Blt_BoxesDontOverlap(Graph* graphPtr, Region2d *extsPtr)
{
  return (((double)graphPtr->right < extsPtr->left) ||
	  ((double)graphPtr->bottom < extsPtr->top) ||
	  (extsPtr->right < (double)graphPtr->left) ||
	  (extsPtr->bottom < (double)graphPtr->top));
}

