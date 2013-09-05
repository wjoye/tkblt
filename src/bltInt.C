/*
 * bltCoreInit.c --
 *
 * This module initials the non-Tk command of the BLT toolkit, registering the
 * commands with the TCL interpreter.
 *
 *	Copyright 1991-2004 George A Howlett.
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

#include "bltInt.h"

Tcl_AppInitProc Blt_core_Init;
Tcl_AppInitProc Blt_core_SafeInit;
Tcl_AppInitProc Blt_x_Init;
Tcl_AppInitProc Blt_x_SafeInit;
Tcl_AppInitProc Blt_Init;

int Blt_core_Init(Tcl_Interp *interp)
{
  Tcl_Namespace *nsPtr;

  if(
#ifdef USE_TCL_STUBS
     Tcl_InitStubs(interp, TCL_PATCH_LEVEL, 0)
#else
     Tcl_PkgRequire(interp, "Tcl", TCL_PATCH_LEVEL, 0)
#endif
     == NULL) {
    return TCL_ERROR;
  }

  nsPtr = Tcl_FindNamespace(interp, "::blt", (Tcl_Namespace *)NULL, 0);
  if (nsPtr == NULL) {
    nsPtr = Tcl_CreateNamespace(interp, "::blt", NULL, NULL);
    if (nsPtr == NULL) {
      return TCL_ERROR;
    }
  }

  if (Blt_VectorCmdInitProc(interp) != TCL_OK)
    return TCL_ERROR;

  if (Tcl_PkgProvide(interp, "blt_core", BLT_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
  return TCL_OK;
}

int
Blt_core_SafeInit(Tcl_Interp *interp)
{
  return Blt_core_Init(interp);
}

int
Blt_x_Init(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    Tcl_Namespace *nsPtr;

  if(
#ifdef USE_TCL_STUBS
     Tcl_InitStubs(interp, TCL_PATCH_LEVEL, 0)
#else
    Tcl_PkgRequire(interp, "Tcl", TCL_PATCH_LEVEL, 0)
#endif
     == NULL) {
    return TCL_ERROR;
  }

  if(
#ifdef USE_TCL_STUBS
     Tk_InitStubs(interp, TK_PATCH_LEVEL, 0)
#else
     Tcl_PkgRequire(interp, "Tk", TK_PATCH_LEVEL, 0)
#endif
     == NULL) {
    return TCL_ERROR;
  }

    nsPtr = Tcl_CreateNamespace(interp, "::blt::tk", NULL, NULL);
    if (nsPtr == NULL) {
	return TCL_ERROR;
    }
    nsPtr = Tcl_FindNamespace(interp, "::blt", NULL, TCL_LEAVE_ERR_MSG);
    if (nsPtr == NULL) {
	return TCL_ERROR;
    }

    if (Blt_GraphCmdInitProc(interp) != TCL_OK)
      return TCL_ERROR;

    if (Tcl_PkgProvide(interp, "blt_extra", BLT_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

int
Blt_x_SafeInit(Tcl_Interp *interp)
{
    return Blt_x_Init(interp);
}

int
Blt_Init(Tcl_Interp *interp) 
{
    if (Blt_core_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_x_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

