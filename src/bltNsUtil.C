
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

#include "bltInt.h"
#include "bltNsUtil.h"

/*
 * A Command structure exists for each command in a namespace. The Tcl_Command
 * opaque type actually refers to these structures.
 */

typedef struct CompileProc CompileProc;
typedef struct ImportRef ImportRef;
typedef struct CommandTrace CommandTrace;

typedef struct {
    Tcl_HashEntry *hPtr;	/* Pointer to the hash table entry that refers
				 * to this command. The hash table is either a
				 * namespace's command table or an
				 * interpreter's hidden command table. This
				 * pointer is used to get a command's name
				 * from its Tcl_Command handle. NULL means
				 * that the hash table entry has been removed
				 * already (this can happen if deleteProc
				 * causes the command to be deleted or
				 * recreated). */
    Tcl_Namespace *nsPtr;	/* Points to the namespace containing this
				 * command. */
    int refCount;		/* 1 if in command hashtable plus 1 for each
				 * reference from a CmdName TCL object
				 * representing a command's name in a ByteCode
				 * instruction sequence. This structure can be
				 * freed when refCount becomes zero. */
    int cmdEpoch;		/* Incremented to invalidate any references
				 * that point to this command when it is
				 * renamed, deleted, hidden, or exposed. */
    CompileProc *compileProc;	/* Procedure called to compile command. NULL
				 * if no compile proc exists for command. */
    Tcl_ObjCmdProc *objProc;	/* Object-based command procedure. */
    ClientData objClientData;	/* Arbitrary value passed to object proc. */
    Tcl_CmdProc *proc;		/* String-based command procedure. */
    ClientData clientData;	/* Arbitrary value passed to string proc. */
    Tcl_CmdDeleteProc *deleteProc;
				/* Procedure invoked when deleting command
				 * to, e.g., free all client data. */
    ClientData deleteData;	/* Arbitrary value passed to deleteProc. */
    int flags;			/* Means that the command is in the process of
				 * being deleted (its deleteProc is currently
				 * executing). Other attempts to delete the
				 * command should be ignored. */
    ImportRef *importRefPtr;	/* List of each imported Command created in
				 * another namespace when this command is
				 * imported. These imported commands redirect
				 * invocations back to this command. The list
				 * is used to remove all those imported
				 * commands when deleting this "real"
				 * command. */
    CommandTrace *tracePtr;	/* First in list of all traces set for this
				 * command. */
} Command;

/*ARGSUSED*/
Tcl_Namespace *
Blt_GetCommandNamespace(Tcl_Command cmdToken)
{
    Command *cmdPtr = (Command *)cmdToken;

    return (Tcl_Namespace *)cmdPtr->nsPtr;
}

Tcl_CallFrame *
Blt_EnterNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr)
{
    Tcl_CallFrame *framePtr;

    framePtr = malloc(sizeof(Tcl_CallFrame));
    if (Tcl_PushCallFrame(interp, framePtr, (Tcl_Namespace *)nsPtr, 0)
	!= TCL_OK) {
	free(framePtr);
	return NULL;
    }
    return framePtr;
}

void
Blt_LeaveNamespace(Tcl_Interp *interp, Tcl_CallFrame *framePtr)
{
    Tcl_PopCallFrame(interp);
    free(framePtr);
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

