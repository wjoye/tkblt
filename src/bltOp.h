/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_OpSpec --
 *
 * 	Structure to specify a set of operations for a TCL command.
 *      This is passed to the Blt_GetOp procedure to look
 *      for a function pointer associated with the operation name.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    const char *name;		/* Name of operation */
    int minChars;		/* Minimum # characters to disambiguate */
    void *proc;
    int minArgs;		/* Minimum # args required */
    int maxArgs;		/* Maximum # args required */
    const char *usage;		/* Usage message */
} Blt_OpSpec;

typedef enum {
    BLT_OP_ARG0,		/* Op is the first argument. */
    BLT_OP_ARG1,		/* Op is the second argument. */
    BLT_OP_ARG2,		/* Op is the third argument. */
    BLT_OP_ARG3,		/* Op is the fourth argument. */
    BLT_OP_ARG4			/* Op is the fifth argument. */

} Blt_OpIndex;

#define BLT_OP_BINARY_SEARCH	0
#define BLT_OP_LINEAR_SEARCH	1

extern void *Blt_GetOpFromObj(Tcl_Interp *interp, int nSpecs, 
	Blt_OpSpec *specs, int operPos, int objc, Tcl_Obj *const *objv, 
	int flags);

