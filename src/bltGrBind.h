/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1998-2004 George A Howlett.
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

#ifndef _BLT_BIND_H
#define _BLT_BIND_H

#include <tk.h>

#include <bltList.h>

typedef struct _Blt_BindTable *Blt_BindTable;

typedef ClientData (Blt_BindPickProc)(ClientData clientData, int x, int y, 
				      ClientData *contextPtr);

typedef void (Blt_BindTagProc)(Blt_BindTable bindTable, ClientData object, 
			       ClientData context, Blt_List list);

struct _Blt_BindTable {
  unsigned int flags;
  Tk_BindingTable bindingTable;
  ClientData currentItem; // The item currently containing the mouse pointer
  ClientData currentContext; // One word indicating what kind of object
  ClientData newItem; // The item that is about to become the current one
  ClientData newContext; // One-word indicating what kind of object was picked
  ClientData focusItem;
  ClientData focusContext;
  XEvent pickEvent; // The event upon which the choice of the current tab
  int activePick; // The pick event has been initialized so that we can repick
  int state; // Last known modifier state
  ClientData clientData;
  Tk_Window tkwin;
  Blt_BindPickProc *pickProc; // Routine to report the item the mouse is over
  Blt_BindTagProc *tagProc; // Routine to report tags picked items
};

extern Blt_BindTable Blt_CreateBindingTable(Tcl_Interp* interp, 
					    Tk_Window tkwin, 
					    ClientData clientData, 
					    Blt_BindPickProc *pickProc,
					    Blt_BindTagProc *tagProc);
extern void Blt_DestroyBindingTable(Blt_BindTable table);
extern void Blt_DeleteBindings(Blt_BindTable table, ClientData object);
extern int Blt_ConfigureBindingsFromObj(Tcl_Interp* interp, 
					Blt_BindTable table, 
					ClientData item, 
					int objc, Tcl_Obj *const *objv);

#endif
