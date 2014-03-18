/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/* 
 *	Copyright (c) 1990-1994 The Regents of the University of California.
 *	Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 *	See the file "license.terms" for information on usage and redistribution
 *	of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *	Copyright 2003-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdarg.h>

extern "C" {
#include "bltInt.h"
#include "bltConfig.h"
};

void RestoreProc(ClientData clientData, Tk_Window tkwin,
		 char *ptr, char *savePtr)
{
  *(double*)ptr = *(double*)savePtr;
}

// State
const char* stateObjOption[] = {"normal", "active", "disabled", NULL};

// Point
static Tk_CustomOptionSetProc PointSetProc;
static Tk_CustomOptionGetProc PointGetProc;
Tk_ObjCustomOption pointObjOption =
  {
    "point", PointSetProc, PointGetProc, NULL, NULL, NULL
  };

static int PointSetProc(ClientData clientData, Tcl_Interp *interp,
			Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			int offset, char* save, int flags)
{
  XPoint* pointPtr = (XPoint*)(widgRec + offset);
  int x, y;

  if (Blt_GetXY(interp, tkwin, Tcl_GetString(*objPtr), &x, &y) != TCL_OK)
    return TCL_ERROR;

  pointPtr->x = x;
  pointPtr->y = y;

  return TCL_OK;
};

static Tcl_Obj* PointGetProc(ClientData clientData, Tk_Window tkwin, 
			     char *widgRec, int offset)
{
  XPoint* pointPtr = (XPoint*)(widgRec + offset);

  if ((pointPtr->x != -SHRT_MAX) && (pointPtr->y != -SHRT_MAX)) {
    char string[200];
    sprintf_s(string, 200, "@%d,%d", pointPtr->x, pointPtr->y);
    return Tcl_NewStringObj(string, -1);
  } 
  else
    return Tcl_NewStringObj("", -1);
};

// Dashes
static Tk_CustomOptionSetProc DashesSetProc;
static Tk_CustomOptionGetProc DashesGetProc;
Tk_ObjCustomOption dashesObjOption =
  {
    "dashes", DashesSetProc, DashesGetProc, NULL, NULL, NULL
  };

static int DashesSetProc(ClientData clientData, Tcl_Interp *interp,
			 Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
			 int offset, char* save, int flags)
{
  Blt_Dashes* dashesPtr = (Blt_Dashes*)(widgRec + offset);

  int length;
  const char* string = Tcl_GetStringFromObj(*objPtr, &length);
  if (!string) {
    dashesPtr->values[0] = 0;
    return TCL_OK;
  }

  if (!string[0])
    dashesPtr->values[0] = 0;
  else if (!strncmp(string, "dot", length)) {	
    /* 1 */
    dashesPtr->values[0] = 1;
    dashesPtr->values[1] = 0;
  }
  else if (!strncmp(string, "dash", length)) {	
    /* 5 2 */
    dashesPtr->values[0] = 5;
    dashesPtr->values[1] = 2;
    dashesPtr->values[2] = 0;
  }
  else if (!strncmp(string, "dashdot", length)) { 
    /* 2 4 2 */
    dashesPtr->values[0] = 2;
    dashesPtr->values[1] = 4;
    dashesPtr->values[2] = 2;
    dashesPtr->values[3] = 0;
  }
  else if (!strncmp(string, "dashdotdot", length)) { 
    /* 2 4 2 2 */
    dashesPtr->values[0] = 2;
    dashesPtr->values[1] = 4;
    dashesPtr->values[2] = 2;
    dashesPtr->values[3] = 2;
    dashesPtr->values[4] = 0;
  }
  else {
    int objc;
    Tcl_Obj** objv;
    if (Tcl_ListObjGetElements(interp, *objPtr, &objc, &objv) != TCL_OK)
      return TCL_ERROR;

    if (objc > 11) {	/* This is the postscript limit */
      Tcl_AppendResult(interp, "too many values in dash list \"", 
		       string, "\"", (char *)NULL);
      return TCL_ERROR;
    }

    int ii;
    for (ii=0; ii<objc; ii++) {
      int value;
      if (Tcl_GetIntFromObj(interp, objv[ii], &value) != TCL_OK)
	return TCL_ERROR;

      /*
       * Backward compatibility:
       * Allow list of 0 to turn off dashes
       */
      if ((value == 0) && (objc == 1))
	break;

      if ((value < 1) || (value > 255)) {
	Tcl_AppendResult(interp, "dash value \"", 
			 Tcl_GetString(objv[ii]), "\" is out of range", 
			 (char *)NULL);
	return TCL_ERROR;
      }
      dashesPtr->values[ii] = (unsigned char)value;
    }

    // Make sure the array ends with a NUL byte
    dashesPtr->values[ii] = 0;
  }

  return TCL_OK;
};

static Tcl_Obj* DashesGetProc(ClientData clientData, Tk_Window tkwin, 
			     char *widgRec, int offset)
{
  Blt_Dashes* dashesPtr = (Blt_Dashes*)(widgRec + offset);

  // count how many
  int cnt =0;
  while (dashesPtr->values[cnt])
    cnt++;

  if (!cnt)
    return Tcl_NewListObj(0, (Tcl_Obj**)NULL);

  Tcl_Obj** ll = (Tcl_Obj**)calloc(cnt, sizeof(Tcl_Obj*));
  for (int ii=0; ii<cnt; ii++)
    ll[ii] = Tcl_NewIntObj(dashesPtr->values[ii]);
  Tcl_Obj* listObjPtr = Tcl_NewListObj(cnt, ll);
  free(ll);

  return listObjPtr;
};

// List
static Tk_CustomOptionSetProc ListSetProc;
static Tk_CustomOptionGetProc ListGetProc;
static Tk_CustomOptionFreeProc ListFreeProc;
Tk_ObjCustomOption listObjOption =
  {
    "list", ListSetProc, ListGetProc, RestoreProc, ListFreeProc, NULL
  };

static int ListSetProc(ClientData clientData, Tcl_Interp *interp,
		       Tk_Window tkwin, Tcl_Obj** objPtr, char* widgRec,
		       int offset, char* savePtr, int flags)
{
  const char*** listPtr = (const char***)(widgRec + offset);
  *(double*)savePtr = *(double*)listPtr;

  const char** argv;
  int argc;
  if (Tcl_SplitList(interp, Tcl_GetString(*objPtr), &argc, &argv) != TCL_OK)
    return TCL_ERROR;

  *listPtr = argv;

  return TCL_OK;
};

static Tcl_Obj* ListGetProc(ClientData clientData, Tk_Window tkwin, 
			    char *widgRec, int offset)
{
  const char*** listPtr = (const char***)(widgRec + offset);

  // count how many
  int cnt=0;
  for (const char** p = *listPtr; *p != NULL; p++,cnt++) {}
  if (!cnt)
    return Tcl_NewListObj(0, (Tcl_Obj**)NULL);

  Tcl_Obj** ll = (Tcl_Obj**)calloc(cnt, sizeof(Tcl_Obj*));
  for (int ii=0; ii<cnt; ii++)
    ll[ii] = Tcl_NewStringObj((*listPtr)[ii], -1);
  Tcl_Obj* listObjPtr = Tcl_NewListObj(cnt, ll);
  free(ll);

  return listObjPtr;
};

static void ListFreeProc(ClientData clientData, Tk_Window tkwin,
			 char *ptr)
{
  const char** argv = *(const char***)ptr;
  if (argv)
    Tcl_Free((char*)argv);
}
