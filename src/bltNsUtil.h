/*
 * bltNsUtil.h --
 *
 *	Copyright 1993-2004 George A Howlett.
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

#ifndef BLT_NS_UTIL_H
#define BLT_NS_UTIL_H 1

#ifndef TCL_NAMESPACE_ONLY
#define TCL_NAMESPACE_ONLY TCL_GLOBAL_ONLY
#endif

#define NS_SEARCH_NONE		(0)
#define NS_SEARCH_CURRENT	(1<<0)
#define NS_SEARCH_GLOBAL	(1<<1)
#define NS_SEARCH_BOTH		(NS_SEARCH_GLOBAL | NS_SEARCH_CURRENT)

typedef struct {
    const char *name;
    Tcl_Namespace *nsPtr;
} Blt_ObjectName;

#define BLT_NO_DEFAULT_NS	(1<<0)
#define BLT_NO_ERROR_MSG	(1<<1)

#ifndef USE_TCL_STUBS

extern Tcl_Command Tcl_FindCommand(Tcl_Interp *interp, const char *name, 
	Tcl_Namespace *nsPtr, int flags);
/*
 * Namespace procedures not prototyped defined in Tcl.h 
 */
extern Tcl_Namespace *Tcl_GetCurrentNamespace(Tcl_Interp *interp);

extern Tcl_Namespace *Tcl_GetGlobalNamespace(Tcl_Interp *interp);

extern Tcl_Namespace *Tcl_CreateNamespace(Tcl_Interp *interp, 
	const char *name, ClientData clientData, 
	Tcl_NamespaceDeleteProc *nsDelProc);

extern void Tcl_DeleteNamespace(Tcl_Namespace *nsPtr);

extern Tcl_Namespace *Tcl_FindNamespace(Tcl_Interp *interp, 
	const char *name, Tcl_Namespace *context, int flags);

extern int Tcl_Export(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	const char *name, int resetFlag);

extern Tcl_Var Tcl_FindNamespaceVar(Tcl_Interp *interp, const char *name, 
	Tcl_Namespace *contextNsPtr, int flags);

extern void Tcl_PopCallFrame(Tcl_Interp *interp);

extern int Tcl_PushCallFrame(Tcl_Interp *interp, Tcl_CallFrame *framePtr, 
	Tcl_Namespace *nsPtr, int isProcCallFrame);

#endif /* USE_TCL_STUBS */

/* 
 * Auxillary procedures 
 */
extern Tcl_Namespace *Blt_GetVariableNamespace(Tcl_Interp *interp, 
	const char *varName);

extern Tcl_Namespace *Blt_GetCommandNamespace(Tcl_Command cmdToken);

extern Tcl_CallFrame *Blt_EnterNamespace(Tcl_Interp *interp, 
	Tcl_Namespace *nsPtr);

extern void Blt_LeaveNamespace(Tcl_Interp *interp, Tcl_CallFrame *framePtr);

extern int Blt_ParseObjectName(Tcl_Interp *interp, const char *name, 
	Blt_ObjectName *objNamePtr, unsigned int flags);

extern char *Blt_MakeQualifiedName(Blt_ObjectName *objNamePtr, 
	Tcl_DString *resultPtr);

#endif /* BLT_NS_UTIL_H */
