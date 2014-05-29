/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1998 George A Howlett.
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

#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "bltGrBind.h"
#include "bltGraph.h"

static Tk_EventProc BindProc;

BindTable::BindTable(Graph* graphPtr, Tcl_Interp* interp, Tk_Window tkwinn, 
		     Blt_BindPickProc* pickProcc)
{
  graphPtr_ = graphPtr;
  flags =0;
  bindingTable = Tk_CreateBindingTable(interp);
  currentItem =NULL;
  currentContext =NULL;
  newItem =NULL;
  newContext =NULL;
  focusItem =NULL;
  focusContext =NULL;
  //  pickEvent =NULL;
  state =0;
  tkwin = tkwinn;
  pickProc = pickProcc;
  unsigned int mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
		       ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
		       PointerMotionMask);
  Tk_CreateEventHandler(tkwin, mask, BindProc, this);
}

BindTable::~BindTable()
{
  Tk_DeleteBindingTable(bindingTable);
  unsigned int mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
		       ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
		       PointerMotionMask);
  Tk_DeleteEventHandler(tkwin, mask, BindProc, this);
}

int BindTable::configure(Tcl_Interp* interp, ClientData item, 
			 int objc, Tcl_Obj* const objv[])
{
  if (objc == 0) {
    Tk_GetAllBindings(interp, bindingTable, item);
    return TCL_OK;
  }

  const char *string = Tcl_GetString(objv[0]);
  if (objc == 1) {
    const char* command = Tk_GetBinding(interp, bindingTable, item, string);
    if (!command) {
      Tcl_ResetResult(interp);
      Tcl_AppendResult(interp, "invalid binding event \"", string, "\"", NULL);
      return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), command, -1);
    return TCL_OK;
  }

  const char* seq = string;
  const char* command = Tcl_GetString(objv[1]);
  if (command[0] == '\0')
    return Tk_DeleteBinding(interp, bindingTable, item, seq);

  unsigned long mask;
  if (command[0] == '+')
    mask = Tk_CreateBinding(interp, bindingTable, item, seq, command+1, 1);
  else
    mask = Tk_CreateBinding(interp, bindingTable, item, seq, command, 0);
  if (!mask)
    return TCL_ERROR;

  if (mask & (unsigned) ~(ButtonMotionMask|Button1MotionMask
			  |Button2MotionMask|Button3MotionMask|Button4MotionMask
			  |Button5MotionMask|ButtonPressMask|ButtonReleaseMask
			  |EnterWindowMask|LeaveWindowMask|KeyPressMask
			  |KeyReleaseMask|PointerMotionMask|VirtualEventMask)) {
    Tk_DeleteBinding(interp, bindingTable, item, seq);
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "requested illegal events; ",
		     "only key, button, motion, enter, leave, and virtual ",
		     "events may be used", (char *)NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}

void BindTable::deleteBindings(ClientData object)
{
  Tk_DeleteAllBindings(bindingTable, object);

  // If this is the object currently picked, we need to repick one.
  if (currentItem == object) {
    currentItem =NULL;
    currentContext =NULL;
  }

  if (newItem == object) {
    newItem =NULL;
    newContext =NULL;
  }

  if (focusItem == object) {
    focusItem =NULL;
    focusContext =NULL;
  }
}

void BindTable::doEvent(XEvent* eventPtr, ClientData item, ClientData context)
{
  if (!tkwin || !bindingTable)
    return;

  if ((eventPtr->type == KeyPress) || (eventPtr->type == KeyRelease)) {
    item = focusItem;
    context = focusContext;
  }
  if (!item)
    return;

  int nTags;
  ClassId classId = (ClassId)(long(context));
  const char** tagArray = graphPtr_->getTags(item, classId, &nTags);
  Tk_BindEvent(bindingTable, eventPtr, tkwin, nTags, (void**)tagArray);
  if (tagArray)
    delete [] tagArray;
}

#define REPICK_IN_PROGRESS (1<<0)
#define LEFT_GRABBED_ITEM  (1<<1)

static void PickCurrentItem(BindTable* bindPtr, XEvent* eventPtr)
{
  // Check whether or not a button is down.  If so, we'll log entry and exit
  // into and out of the current item, but not entry into any other item.
  // This implements a form of grabbing equivalent to what the X server does
  // for windows.

  int buttonDown = (bindPtr->state & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask));
  if (!buttonDown)
    bindPtr->flags &= ~LEFT_GRABBED_ITEM;

  // Save information about this event in the widget.  The event in the
  // widget is used for two purposes:
  // 1. Event bindings: if the current item changes, fake events are
  //    generated to allow item-enter and item-leave bindings to trigger.
  // 2. Reselection: if the current item gets deleted, can use the
  //    saved event to find a new current item.
  // Translate MotionNotify events into EnterNotify events, since that's
  // what gets reported to item handlers.

  if (eventPtr != &bindPtr->pickEvent) {
    if ((eventPtr->type == MotionNotify) || (eventPtr->type == ButtonRelease)) {
      bindPtr->pickEvent.xcrossing.type = EnterNotify;
      bindPtr->pickEvent.xcrossing.serial = eventPtr->xmotion.serial;
      bindPtr->pickEvent.xcrossing.send_event =	eventPtr->xmotion.send_event;
      bindPtr->pickEvent.xcrossing.display = eventPtr->xmotion.display;
      bindPtr->pickEvent.xcrossing.window = eventPtr->xmotion.window;
      bindPtr->pickEvent.xcrossing.root = eventPtr->xmotion.root;
      bindPtr->pickEvent.xcrossing.subwindow = None;
      bindPtr->pickEvent.xcrossing.time = eventPtr->xmotion.time;
      bindPtr->pickEvent.xcrossing.x = eventPtr->xmotion.x;
      bindPtr->pickEvent.xcrossing.y = eventPtr->xmotion.y;
      bindPtr->pickEvent.xcrossing.x_root = eventPtr->xmotion.x_root;
      bindPtr->pickEvent.xcrossing.y_root = eventPtr->xmotion.y_root;
      bindPtr->pickEvent.xcrossing.mode = NotifyNormal;
      bindPtr->pickEvent.xcrossing.detail = NotifyNonlinear;
      bindPtr->pickEvent.xcrossing.same_screen = eventPtr->xmotion.same_screen;
      bindPtr->pickEvent.xcrossing.focus = False;
      bindPtr->pickEvent.xcrossing.state = eventPtr->xmotion.state;
    }
    else
      bindPtr->pickEvent = *eventPtr;
  }

  // If this is a recursive call (there's already a partially completed call
  // pending on the stack; it's in the middle of processing a Leave event
  // handler for the old current item) then just return; the pending call
  // will do everything that's needed.
  if (bindPtr->flags & REPICK_IN_PROGRESS)
    return;

  // A LeaveNotify event automatically means that there's no current item,
  // so the check for closest item can be skipped.
  ClientData newContext =NULL;
  ClientData newItem =NULL;
  if (bindPtr->pickEvent.type != LeaveNotify) {
    int x = bindPtr->pickEvent.xcrossing.x;
    int y = bindPtr->pickEvent.xcrossing.y;
    newItem = (*bindPtr->pickProc)(bindPtr->graphPtr_, x, y, &newContext);
  }

  // Nothing to do:  the current item hasn't changed.
  if (((newItem == bindPtr->currentItem) && (newContext == bindPtr->currentContext)) && ((bindPtr->flags & LEFT_GRABBED_ITEM) == 0))
    return;

  // Simulate a LeaveNotify event on the previous current item and an
  // EnterNotify event on the new current item.  Remove the "current" tag
  // from the previous current item and place it on the new current item.
  ClientData oldItem = bindPtr->currentItem;
  Tcl_Preserve(oldItem);
  Tcl_Preserve(newItem);

  if ((bindPtr->currentItem != NULL) && ((newItem != bindPtr->currentItem) || (newContext != bindPtr->currentContext)) && ((bindPtr->flags & LEFT_GRABBED_ITEM) == 0)) {
    XEvent event = bindPtr->pickEvent;
    event.type = LeaveNotify;

    // If the event's detail happens to be NotifyInferior the binding
    // mechanism will discard the event.  To be consistent, always use
    // NotifyAncestor.
    event.xcrossing.detail = NotifyAncestor;

    bindPtr->flags |= REPICK_IN_PROGRESS;
    bindPtr->doEvent(&event, bindPtr->currentItem, bindPtr->currentContext);
    bindPtr->flags &= ~REPICK_IN_PROGRESS;

    // Note: during DoEvent above, it's possible that bindPtr->newItem got
    // reset to NULL because the item was deleted.
  }

  if (((newItem != bindPtr->currentItem) || (newContext != bindPtr->currentContext)) && (buttonDown)) {
    bindPtr->flags |= LEFT_GRABBED_ITEM;
    XEvent event = bindPtr->pickEvent;
    if ((newItem != bindPtr->newItem) || 
	(newContext != bindPtr->newContext)) {

      // Generate <Enter> and <Leave> events for objects during button
      // grabs. This isn't standard. But for example, it allows one to
      // provide balloon help on the individual entries of the Hierbox
      // widget.
      ClientData savedItem = bindPtr->currentItem;
      ClientData savedContext = bindPtr->currentContext;
      if (bindPtr->newItem != NULL) {
	event.type = LeaveNotify;
	event.xcrossing.detail = NotifyVirtual; // Ancestor
	bindPtr->currentItem = bindPtr->newItem;
	bindPtr->doEvent(&event, bindPtr->newItem, bindPtr->newContext);
      }

      bindPtr->newItem = newItem;
      bindPtr->newContext = newContext;
      if (newItem != NULL) {
	event.type = EnterNotify;
	event.xcrossing.detail = NotifyVirtual; // Ancestor
	bindPtr->currentItem = newItem;
	bindPtr->doEvent(&event, newItem, newContext);
      }

      bindPtr->currentItem = savedItem;
      bindPtr->currentContext = savedContext;
    }
    goto done;
  }

  // Special note:  it's possible that bindPtr->newItem == bindPtr->currentItem
  // here. This can happen, for example, if LEFT_GRABBED_ITEM was set.
  bindPtr->flags &= ~LEFT_GRABBED_ITEM;
  bindPtr->currentItem = bindPtr->newItem = newItem;
  bindPtr->currentContext = bindPtr->newContext = newContext;
  if (bindPtr->currentItem != NULL) {
    XEvent event = bindPtr->pickEvent;
    event.type = EnterNotify;
    event.xcrossing.detail = NotifyAncestor;
    bindPtr->doEvent(&event, newItem, newContext);
  }

 done:
  Tcl_Release(newItem);
  Tcl_Release(oldItem);
}

static void BindProc(ClientData clientData, XEvent* eventPtr)
{
  // This code below keeps track of the current modifier state in
  // bindPtr->state.  This information is used to defer repicks of the
  // current item while buttons are down.

  BindTable* bindPtr = (BindTable*)clientData;

  Tcl_Preserve(bindPtr->graphPtr_);

  switch (eventPtr->type) {
  case ButtonPress:
  case ButtonRelease:
    {
      int mask = 0;
      switch (eventPtr->xbutton.button) {
      case Button1:
	mask = Button1Mask;
	break;
      case Button2:
	mask = Button2Mask;
	break;
      case Button3:
	mask = Button3Mask;
	break;
      case Button4:
	mask = Button4Mask;
	break;
      case Button5:
	mask = Button5Mask;
	break;
      default:
	break;
      }

      // For button press events, repick the current item using the button
      // state before the event, then process the event.  For button release
      // events, first process the event, then repick the current item using
      // the button state *after* the event (the button has logically gone
      // up before we change the current item).

      if (eventPtr->type == ButtonPress) {
	// On a button press, first repick the current item using the
	// button state before the event, the process the event.
	bindPtr->state = eventPtr->xbutton.state;
	PickCurrentItem(bindPtr, eventPtr);
	bindPtr->state ^= mask;
	bindPtr->doEvent(eventPtr, bindPtr->currentItem, bindPtr->currentContext);
      }
      else {
	// Button release: first process the event, with the button still
	// considered to be down.  Then repick the current item under the
	// assumption that the button is no longer down.
	bindPtr->state = eventPtr->xbutton.state;
	bindPtr->doEvent(eventPtr, bindPtr->currentItem, bindPtr->currentContext);
	eventPtr->xbutton.state ^= mask;
	bindPtr->state = eventPtr->xbutton.state;
	PickCurrentItem(bindPtr, eventPtr);
	eventPtr->xbutton.state ^= mask;
      }
    }
    break;

  case EnterNotify:
  case LeaveNotify:
    bindPtr->state = eventPtr->xcrossing.state;
    PickCurrentItem(bindPtr, eventPtr);
    break;

  case MotionNotify:
    bindPtr->state = eventPtr->xmotion.state;
    PickCurrentItem(bindPtr, eventPtr);
    bindPtr->doEvent(eventPtr, bindPtr->currentItem, bindPtr->currentContext);
    break;

  case KeyPress:
  case KeyRelease:
    bindPtr->state = eventPtr->xkey.state;
    PickCurrentItem(bindPtr, eventPtr);
    bindPtr->doEvent(eventPtr, bindPtr->currentItem, bindPtr->currentContext);
    break;
  }

  Tcl_Release(bindPtr->graphPtr_);
}

