/*
 * bltNsUtil.c --
 *
 * This module implements utility namespace procedures for the BLT toolkit.
 *
 *	Copyright 1997-2008 George A Howlett.
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

#include <tclPort.h>
#include <tclInt.h>

#include "bltInt.h"
#include "bltNsUtil.h"

/*ARGSUSED*/
Tcl_Namespace *
Blt_GetCommandNamespace(Tcl_Command cmdToken)
{
  Command *cmdPtr = (Command *)cmdToken;

  return (Tcl_Namespace *)cmdPtr->nsPtr;
}

int
Blt_ParseObjectName(Tcl_Interp *interp, const char *path, 
		    Blt_ObjectName *namePtr, unsigned int flags)
{
  char *last, *colon;

  namePtr->nsPtr = NULL;
  namePtr->name = NULL;
  colon = NULL;

  /* Find the last namespace separator in the qualified name. */
  last = (char *)(path + strlen(path));
  while (--last > path) {
    if ((*last == ':') && (*(last - 1) == ':')) {
      last++;		/* just after the last "::" */
      colon = last - 2;
      break;
    }
  }
  if (colon == NULL) {
    namePtr->name = path;
    if ((flags & BLT_NO_DEFAULT_NS) == 0) {
      namePtr->nsPtr = Tcl_GetCurrentNamespace(interp);
    }
    return TRUE;		/* No namespace designated in name. */
  }

  /* Separate the namespace and the object name. */
  *colon = '\0';
  if (path[0] == '\0') {
    namePtr->nsPtr = Tcl_GetGlobalNamespace(interp);
  } else {
    namePtr->nsPtr = Tcl_FindNamespace(interp, (char *)path, NULL, 
				       (flags & BLT_NO_ERROR_MSG) ? 0 : TCL_LEAVE_ERR_MSG);
  }
  /* Repair the string. */    *colon = ':';

  if (namePtr->nsPtr == NULL) {
    return FALSE;		/* Namespace doesn't exist. */
  }
  namePtr->name =last;
  return TRUE;
}

char *
Blt_MakeQualifiedName(Blt_ObjectName *namePtr, Tcl_DString *resultPtr)
{
  Tcl_DStringInit(resultPtr);
  if ((namePtr->nsPtr->fullName[0] != ':') || 
      (namePtr->nsPtr->fullName[1] != ':') ||
      (namePtr->nsPtr->fullName[2] != '\0')) {
    Tcl_DStringAppend(resultPtr, namePtr->nsPtr->fullName, -1);
  }
  Tcl_DStringAppend(resultPtr, "::", -1);
  Tcl_DStringAppend(resultPtr, (char *)namePtr->name, -1);
  return Tcl_DStringValue(resultPtr);
}

static Tcl_Namespace* NamespaceOfVariable(Var *varPtr)
{
  if (varPtr->flags & VAR_IN_HASHTABLE) {
    VarInHash *vhashPtr = (VarInHash *)varPtr;
    TclVarHashTable *vtablePtr;

    vtablePtr = (TclVarHashTable *)vhashPtr->entry.tablePtr;
    return (Tcl_Namespace*)(vtablePtr->nsPtr);
  }
  return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVariableNamespace --
 *
 *	Returns the namespace context of the variable.  If NULL, this
 *	indicates that the variable is local to the call frame.
 *
 * Results:
 *	Returns the context of the namespace in an opaque type.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Namespace *
Blt_GetVariableNamespace(Tcl_Interp *interp, const char *path)
{
  Blt_ObjectName objName;

  if (!Blt_ParseObjectName(interp, path, &objName, BLT_NO_DEFAULT_NS)) {
    return NULL;
  }
  if (objName.nsPtr == NULL) {
    Var *varPtr;

    varPtr = (Var *)Tcl_FindNamespaceVar(interp, (char *)path, 
					 (Tcl_Namespace *)NULL, 
					 TCL_GLOBAL_ONLY);
    if (varPtr != NULL) {
      return NamespaceOfVariable(varPtr);
    }
  }
  return objName.nsPtr;    
}
