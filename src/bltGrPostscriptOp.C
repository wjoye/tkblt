/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
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

#include "bltGraph.h"
#include "bltGrPageSetup.h"
#include "bltGrPageSetupOp.h"
#include "bltPs.h"

using namespace Blt;

int PageSetupObjConfigure(Graph* graphPtr, Tcl_Interp* interp, 
			  int objc, Tcl_Obj* const objv[])
{
  PageSetup* setupPtr = graphPtr->pageSetup_;
  Tk_SavedOptions savedOptions;
  int mask =0;
  int error;
  Tcl_Obj* errorResult;

  for (error=0; error<=1; error++) {
    if (!error) {
      if (Tk_SetOptions(interp, (char*)setupPtr->ops_, setupPtr->optionTable_, 
			objc, objv, graphPtr->tkwin_, &savedOptions, &mask)
	  != TCL_OK)
	continue;
    }
    else {
      errorResult = Tcl_GetObjResult(interp);
      Tcl_IncrRefCount(errorResult);
      Tk_RestoreSavedOptions(&savedOptions);
    }

    break; 
  }

  if (!error) {
    Tk_FreeSavedOptions(&savedOptions);
    return TCL_OK;
  }
  else {
    Tcl_SetObjResult(interp, errorResult);
    Tcl_DecrRefCount(errorResult);
    return TCL_ERROR;
  }
}

static int CgetOp(ClientData clientData, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;

  if (objc != 4) {
    Tcl_WrongNumArgs(interp, 2, objv, "cget option");
    return TCL_ERROR;
  }

  PageSetup *setupPtr = graphPtr->pageSetup_;
  Tcl_Obj* objPtr = Tk_GetOptionValue(interp, 
				      (char*)setupPtr->ops_, 
				      setupPtr->optionTable_,
				      objv[3], graphPtr->tkwin_);
  if (objPtr == NULL)
    return TCL_ERROR;
  else
    Tcl_SetObjResult(interp, objPtr);
  return TCL_OK;
}

static int ConfigureOp(ClientData clientData, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;
  PageSetup* setupPtr = graphPtr->pageSetup_;
  if (objc <= 4) {
    Tcl_Obj* objPtr = Tk_GetOptionInfo(interp, (char*)setupPtr->ops_, 
				       setupPtr->optionTable_, 
				       (objc == 4) ? objv[3] : NULL, 
				       graphPtr->tkwin_);
    if (objPtr == NULL)
      return TCL_ERROR;
    else
      Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
  } 
  else
    return PageSetupObjConfigure(graphPtr, interp, objc-3, objv+3);
}

static int OutputOp(ClientData clientData, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Graph* graphPtr = (Graph*)clientData;

  const char *fileName = NULL;
  Tcl_Channel channel = NULL;
  if (objc > 3) {
    fileName = Tcl_GetString(objv[3]);
    if (fileName[0] != '-') {
      objv++, objc--;		/* First argument is the file name. */
      channel = Tcl_OpenFileChannel(interp, fileName, "w", 0666);
      if (!channel)
	return TCL_ERROR;

      if (Tcl_SetChannelOption(interp, channel, "-translation", "binary") 
	  != TCL_OK)
	return TCL_ERROR;
    }
  }

  PostScript *psPtr = Blt_Ps_Create(graphPtr->interp_, graphPtr->pageSetup_);
  
  if (PageSetupObjConfigure(graphPtr, interp, objc-3, objv+3) != TCL_OK) {
    if (channel)
      Tcl_Close(interp, channel);
    Blt_Ps_Free(psPtr);
    return TCL_ERROR;
  }

  if (graphPtr->print(fileName, psPtr) != TCL_OK) {
    if (channel)
      Tcl_Close(interp, channel);
    Blt_Ps_Free(psPtr);
    return TCL_ERROR;
  }

  int length;
  const char *buffer = Blt_Ps_GetValue(psPtr, &length);
  if (channel) {
    int nBytes = Tcl_Write(channel, buffer, length);
    if (nBytes < 0) {
      Tcl_AppendResult(interp, "error writing file \"", fileName, "\": ",
		       Tcl_PosixError(interp), (char *)NULL);
      if (channel)
	Tcl_Close(interp, channel);
      Blt_Ps_Free(psPtr);
      return TCL_ERROR;
    }
    Tcl_Close(interp, channel);
  }
  else
    Tcl_SetStringObj(Tcl_GetObjResult(interp), buffer, length);

  Blt_Ps_Free(psPtr);
  return TCL_OK;
}

const TkEnsemble pageSetupEnsemble[] = {
  {"cget",      CgetOp, 0},
  {"configure", ConfigureOp, 0},
  {"output",    OutputOp, 0},
  { 0,0,0 }
};

// Support

static void AddComments(Blt_Ps ps, const char **comments)
{
  const char **p;
  for (p = comments; *p; p += 2) {
    if (*(p+1) == NULL) {
      break;
    }
    Blt_Ps_Format(ps, "%% %s: %s\n", *p, *(p+1));
  }
}

int PostScriptPreamble(Graph* graphPtr, const char *fileName, Blt_Ps ps)
{
  PageSetup *setupPtr = graphPtr->pageSetup_;
  PageSetupOptions* ops = (PageSetupOptions*)setupPtr->ops_;
  time_t ticks;
  char date[200];			/* Holds the date string from ctime() */
  char *newline;

  if (fileName == NULL) {
    fileName = Tk_PathName(graphPtr->tkwin_);
  }
  Blt_Ps_Append(ps, "%!PS-Adobe-3.0 EPSF-3.0\n");

  /*
   * The "BoundingBox" comment is required for EPS files. The box
   * coordinates are integers, so we need round away from the center of the
   * box.
   */
  Blt_Ps_Format(ps, "%%%%BoundingBox: %d %d %d %d\n",
		setupPtr->left, setupPtr->paperHeight - setupPtr->top,
		setupPtr->right, setupPtr->paperHeight - setupPtr->bottom);
	
  Blt_Ps_Append(ps, "%%Pages: 0\n");

  Blt_Ps_Format(ps, "%%%%Creator: (%s %s %s)\n", 
		PACKAGE_NAME, PACKAGE_VERSION, Tk_Class(graphPtr->tkwin_));

  ticks = time((time_t *) NULL);
  strcpy(date, ctime(&ticks));
  newline = date + strlen(date) - 1;
  if (*newline == '\n') {
    *newline = '\0';
  }
  Blt_Ps_Format(ps, "%%%%CreationDate: (%s)\n", date);
  Blt_Ps_Format(ps, "%%%%Title: (%s)\n", fileName);
  Blt_Ps_Append(ps, "%%DocumentData: Clean7Bit\n");
  if (ops->landscape) {
    Blt_Ps_Append(ps, "%%Orientation: Landscape\n");
  } else {
    Blt_Ps_Append(ps, "%%Orientation: Portrait\n");
  }
  Blt_Ps_Append(ps, "%%DocumentNeededResources: font Helvetica Courier\n");
  AddComments(ps, ops->comments);
  Blt_Ps_Append(ps, "%%EndComments\n\n");
  if (Blt_Ps_IncludeFile(graphPtr->interp_, ps, "bltGraph.pro") != TCL_OK) {
    return TCL_ERROR;
  }
  if (ops->footer) {
    const char *who;

    who = getenv("LOGNAME");
    if (who == NULL) {
      who = "???";
    }
    Blt_Ps_VarAppend(ps,
		     "8 /Helvetica SetFont\n",
		     "10 30 moveto\n",
		     "(Date: ", date, ") show\n",
		     "10 20 moveto\n",
		     "(File: ", fileName, ") show\n",
		     "10 10 moveto\n",
		     "(Created by: ", who, "@", Tcl_GetHostName(), ") show\n",
		     "0 0 moveto\n",
		     (char *)NULL);
  }
  /*
   * Set the conversion from PostScript to X11 coordinates.  Scale pica to
   * pixels and flip the y-axis (the origin is the upperleft corner).
   */
  Blt_Ps_VarAppend(ps,
		   "% Transform coordinate system to use X11 coordinates\n\n",
		   "% 1. Flip y-axis over by reversing the scale,\n",
		   "% 2. Translate the origin to the other side of the page,\n",
		   "%    making the origin the upper left corner\n", (char *)NULL);
  Blt_Ps_Format(ps, "1 -1 scale\n");
  /* Papersize is in pixels.  Translate the new origin *after* changing the
   * scale. */
  Blt_Ps_Format(ps, "0 %d translate\n\n", -setupPtr->paperHeight);
  Blt_Ps_VarAppend(ps, "% User defined page layout\n\n",
		   "% Set color level\n", (char *)NULL);
  Blt_Ps_Format(ps, "%% Set origin\n%d %d translate\n\n",
		setupPtr->left, setupPtr->bottom);
  if (ops->landscape) {
    Blt_Ps_Format(ps,
		  "%% Landscape orientation\n0 %g translate\n-90 rotate\n",
		  ((double)graphPtr->width_ * setupPtr->scale));
  }
  Blt_Ps_Append(ps, "\n%%EndSetup\n\n");
  return TCL_OK;
}

