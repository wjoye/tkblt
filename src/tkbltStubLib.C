#ifndef USE_TCL_STUBS
#define USE_TCL_STUBS
#endif

#include <tcl.h>

void *tkbltStubsPtr;

CONST char *
Tkblt_InitStubs(Tcl_Interp *interp, CONST char *version, int exact)
{
    CONST char *result;

    result = Tcl_PkgRequireEx(interp, "tkblt", version, exact,
		(ClientData *) &tkbltStubsPtr);
    if (!result || !tkbltStubsPtr) {
        return (char *) NULL;
    }

    return result;
}
