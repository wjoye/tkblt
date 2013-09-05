
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
#include "bltNsUtil.h"
#include "bltMath.h"

#ifndef BLT_LIBRARY
#  ifdef WIN32
#    define BLT_LIBRARY  "c:/Program Files/Tcl/lib/blt"##BLT_VERSION
#  else
#    define BLT_LIBRARY "unknown"
#  endif
#endif

#if (_TCL_VERSION >= _VERSION(8,5,0)) 
#define TCL_VERSION_LOADED	TCL_PATCH_LEVEL
#else 
#define TCL_VERSION_LOADED	TCL_VERSION
#endif

BLT_EXTERN Tcl_AppInitProc Blt_core_Init;
BLT_EXTERN Tcl_AppInitProc Blt_core_SafeInit;

static char libPath[1024] =
{
    BLT_LIBRARY
};

/*
 * Script to set the BLT library path in the variable global "blt_library"
 *
 * Checks the usual locations for a file (bltGraph.pro) from the BLT library.
 * The places searched in order are
 *
 *	$BLT_LIBRARY
 *	$BLT_LIBRARY/blt2.4
 *      $BLT_LIBRARY/..
 *      $BLT_LIBRARY/../blt2.4
 *	$blt_libPath
 *	$blt_libPath/blt2.4
 *      $blt_libPath/..
 *      $blt_libPath/../blt2.4
 *	$tcl_library
 *	$tcl_library/blt2.4
 *      $tcl_library/..
 *      $tcl_library/../blt2.4
 *	$env(TCL_LIBRARY)
 *	$env(TCL_LIBRARY)/blt2.4
 *      $env(TCL_LIBRARY)/..
 *      $env(TCL_LIBRARY)/../blt2.4
 *
 *  The TCL variable "blt_library" is set to the discovered path.  If the file
 *  wasn't found, no error is returned.  The actual usage of $blt_library is
 *  purposely deferred so that it can be set from within a script.
 */

/* FIXME: Change this to a namespace procedure in 3.0 */

static char initScript[] =
{"\n\
global blt_library blt_libPath blt_version tcl_library env\n\
set blt_library {}\n\
set path {}\n\
foreach var { env(BLT_LIBRARY) blt_libPath tcl_library env(TCL_LIBRARY) } { \n\
    if { ![info exists $var] } { \n\
        continue \n\
    } \n\
    set path [set $var] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
    set path [file join $path blt$blt_version ] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
    set path [file dirname [set $var]] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
    set path [file join $path blt$blt_version ] \n\
    if { [file readable [file join $path bltGraph.pro]] } { \n\
        set blt_library $path\n\
        break \n\
    } \n\
} \n\
if { $blt_library != \"\" } { \n\
    global auto_path \n\
    lappend auto_path $blt_library \n\
}\n\
unset var path\n\
\n"
};

static int
SetLibraryPath(Tcl_Interp *interp)
{
    Tcl_DString dString;
    const char *value;

    Tcl_DStringInit(&dString);
    Tcl_DStringAppend(&dString, libPath, -1);
#ifdef WIN32
    {
	HKEY key;
	DWORD result;
#  ifndef BLT_REGISTRY_KEY
#    define BLT_REGISTRY_KEY "Software\\BLT\\" BLT_VERSION "\\" TCL_VERSION
#  endif
	result = RegOpenKeyEx(
	      HKEY_LOCAL_MACHINE, /* Parent key. */
	      BLT_REGISTRY_KEY,	/* Path to sub-key. */
	      0,		/* Reserved. */
	      KEY_READ,		/* Security access mask. */
	      &key);		/* Resulting key.*/

	if (result == ERROR_SUCCESS) {
	    DWORD size;

	    /* Query once to get the size of the string needed */
	    result = RegQueryValueEx(key, "BLT_LIBRARY", NULL, NULL, NULL, 
		     &size);
	    if (result == ERROR_SUCCESS) {
		Tcl_DStringSetLength(&dString, size);
		/* And again to collect the string. */
		RegQueryValueEx(key, "BLT_LIBRARY", NULL, NULL,
				(LPBYTE)Tcl_DStringValue(&dString), &size);
		RegCloseKey(key);
	    }
	}
    }
#endif /* WIN32 */
    value = Tcl_SetVar(interp, "blt_libPath", Tcl_DStringValue(&dString),
	TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    Tcl_DStringFree(&dString);
    if (value == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*LINTLIBRARY*/
int Blt_core_Init(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    Tcl_Namespace *nsPtr;
    const char *result;
    const int isExact = 1;

  if(
#ifdef USE_TCL_STUBS
     Tcl_InitStubs(interp, TCL_VERSION_LOADED, isExact)
#else
    Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_LOADED, isExact)
#endif
     == NULL) {
    return TCL_ERROR;
  }

    /* Set the "blt_version", "blt_patchLevel", and "blt_libPath" Tcl
     * variables. We'll use them in the following script. */

    result = Tcl_SetVar(interp, "blt_version", BLT_VERSION, TCL_GLOBAL_ONLY);
    if (result == NULL) {
	return TCL_ERROR;
    }
    result = Tcl_SetVar(interp, "blt_patchLevel", BLT_PATCH_LEVEL, 
			TCL_GLOBAL_ONLY);
    if (result == NULL) {
	return TCL_ERROR;
    }
    if (SetLibraryPath(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_Eval(interp, initScript) != TCL_OK) {
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

/*LINTLIBRARY*/
int
Blt_core_SafeInit(Tcl_Interp *interp) /* Interpreter to add extra commands */
{
    return Blt_core_Init(interp);
}

