/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

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

extern Tcl_AppInitProc Blt_GraphCmdInitProc;
extern Tcl_AppInitProc Blt_VectorCmdInitProc;

Tcl_AppInitProc Tkblt_Init;
Tcl_AppInitProc Tkblt_SafeInit;

int Tkblt_Init(Tcl_Interp *interp)
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
#ifdef USE_TK_STUBS
     Tk_InitStubs(interp, TK_PATCH_LEVEL, 0)
#else
     Tcl_PkgRequire(interp, "Tk", TK_PATCH_LEVEL, 0)
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
  if (Blt_GraphCmdInitProc(interp) != TCL_OK)
    return TCL_ERROR;

  if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }

  return TCL_OK;
}

int Tkblt_SafeInit(Tcl_Interp *interp)
{
  return Tkblt_Init(interp);
}

int Blt_InitCmd(Tcl_Interp *interp, const char *nsName, 
		Blt_InitCmdSpec *specPtr)
{
  Tcl_DString dString;
  Tcl_DStringInit(&dString);
  if (nsName)
    Tcl_DStringAppend(&dString, nsName, -1);
  Tcl_DStringAppend(&dString, "::", -1);
  Tcl_DStringAppend(&dString, specPtr->name, -1);

  const char* cmdPath = Tcl_DStringValue(&dString);
  Tcl_Command cmdToken = Tcl_FindCommand(interp, cmdPath, NULL, 0);
  if (cmdToken) {
    Tcl_DStringFree(&dString);
    return TCL_OK;		/* Assume command was already initialized */
  }
  cmdToken = Tcl_CreateObjCommand(interp, cmdPath, specPtr->cmdProc, 
				  specPtr->clientData, specPtr->cmdDeleteProc);
  Tcl_DStringFree(&dString);
  Tcl_Namespace* nsPtr = Tcl_FindNamespace(interp, nsName, NULL, 
					   TCL_LEAVE_ERR_MSG);
  if (nsPtr == NULL)
    return TCL_ERROR;

  if (Tcl_Export(interp, nsPtr, specPtr->name, FALSE) != TCL_OK)
    return TCL_ERROR;

  return TCL_OK;
}


