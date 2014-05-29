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

BindTable::BindTable(Graph* graphPtr, Blt_BindPickProc* pickProc)
{
  graphPtr_ = graphPtr;
  flags_ =0;
  table_ = Tk_CreateBindingTable(graphPtr->interp_);
  currentItem_ =NULL;
  currentContext_ =NULL;
  newItem_ =NULL;
  newContext_ =NULL;
  focusItem_ =NULL;
  focusContext_ =NULL;
  //  pickEvent =NULL;
  state_ =0;
  pickProc_ = pickProc;
  unsigned int mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
		       ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
		       PointerMotionMask);
  Tk_CreateEventHandler(graphPtr->tkwin_, mask, BindProc, this);
}

BindTable::~BindTable()
{
  Tk_DeleteBindingTable(table_);
  unsigned int mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
		       ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
		       PointerMotionMask);
  Tk_DeleteEventHandler(graphPtr_->tkwin_, mask, BindProc, this);
}

int BindTable::configure(ClientData item, int objc, Tcl_Obj* const objv[])
{
  if (objc == 0) {
    Tk_GetAllBindings(graphPtr_->interp_, table_, item);
    return TCL_OK;
  }

  const char *string = Tcl_GetString(objv[0]);
  if (objc == 1) {
    const char* command = 
      Tk_GetBinding(graphPtr_->interp_, table_, item, string);
    if (!command) {
      Tcl_ResetResult(graphPtr_->interp_);
      Tcl_AppendResult(graphPtr_->interp_, "invalid binding event \"", 
		       string, "\"", NULL);
      return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(graphPtr_->interp_), command, -1);
    return TCL_OK;
  }

  const char* seq = string;
  const char* command = Tcl_GetString(objv[1]);
  if (command[0] == '\0')
    return Tk_DeleteBinding(graphPtr_->interp_, table_, item, seq);

  unsigned long mask;
  if (command[0] == '+')
    mask = Tk_CreateBinding(graphPtr_->interp_, table_, 
			    item, seq, command+1, 1);
  else
    mask = Tk_CreateBinding(graphPtr_->interp_, table_, 
			    item, seq, command, 0);
  if (!mask)
    return TCL_ERROR;

  if (mask & (unsigned) ~(ButtonMotionMask|Button1MotionMask
			  |Button2MotionMask|Button3MotionMask|Button4MotionMask
			  |Button5MotionMask|ButtonPressMask|ButtonReleaseMask
			  |EnterWindowMask|LeaveWindowMask|KeyPressMask
			  |KeyReleaseMask|PointerMotionMask|VirtualEventMask)) {
    Tk_DeleteBinding(graphPtr_->interp_, table_, item, seq);
    Tcl_ResetResult(graphPtr_->interp_);
    Tcl_AppendResult(graphPtr_->interp_, "requested illegal events; ",
		     "only key, button, motion, enter, leave, and virtual ",
		     "events may be used", (char *)NULL);
    return TCL_ERROR;
  }

  return TCL_OK;
}

void BindTable::deleteBindings(ClientData object)
{
  Tk_DeleteAllBindings(table_, object);

  // If this is the object currently picked, we need to repick one.
  if (currentItem_ == object) {
    currentItem_ =NULL;
    currentContext_ =NULL;
  }

  if (newItem_ == object) {
    newItem_ =NULL;
    newContext_ =NULL;
  }

  if (focusItem_ == object) {
    focusItem_ =NULL;
    focusContext_ =NULL;
  }
}

void BindTable::doEvent(XEvent* eventPtr, ClientData item, ClientData context)
{
  if (!graphPtr_->tkwin_ || !table_)
    return;

  if ((eventPtr->type == KeyPress) || (eventPtr->type == KeyRelease)) {
    item = focusItem_;
    context = focusContext_;
  }
  if (!item)
    return;

  int nTags;
  ClassId classId = (ClassId)(long(context));
  const char** tagArray = graphPtr_->getTags(item, classId, &nTags);
  Tk_BindEvent(table_, eventPtr, graphPtr_->tkwin_, nTags, 
	       (void**)tagArray);
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

  int buttonDown = (bindPtr->state_ & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask));
  if (!buttonDown)
    bindPtr->flags_ &= ~LEFT_GRABBED_ITEM;

  // Save information about this event in the widget.  The event in the
  // widget is used for two purposes:
  // 1. Event bindings: if the current item changes, fake events are
  //    generated to allow item-enter and item-leave bindings to trigger.
  // 2. Reselection: if the current item gets deleted, can use the
  //    saved event to find a new current item.
  // Translate MotionNotify events into EnterNotify events, since that's
  // what gets reported to item handlers.

  if (eventPtr != &bindPtr->pickEvent_) {
    if ((eventPtr->type == MotionNotify) || (eventPtr->type == ButtonRelease)) {
      bindPtr->pickEvent_.xcrossing.type = EnterNotify;
      bindPtr->pickEvent_.xcrossing.serial = eventPtr->xmotion.serial;
      bindPtr->pickEvent_.xcrossing.send_event =	eventPtr->xmotion.send_event;
      bindPtr->pickEvent_.xcrossing.display = eventPtr->xmotion.display;
      bindPtr->pickEvent_.xcrossing.window = eventPtr->xmotion.window;
      bindPtr->pickEvent_.xcrossing.root = eventPtr->xmotion.root;
      bindPtr->pickEvent_.xcrossing.subwindow = None;
      bindPtr->pickEvent_.xcrossing.time = eventPtr->xmotion.time;
      bindPtr->pickEvent_.xcrossing.x = eventPtr->xmotion.x;
      bindPtr->pickEvent_.xcrossing.y = eventPtr->xmotion.y;
      bindPtr->pickEvent_.xcrossing.x_root = eventPtr->xmotion.x_root;
      bindPtr->pickEvent_.xcrossing.y_root = eventPtr->xmotion.y_root;
      bindPtr->pickEvent_.xcrossing.mode = NotifyNormal;
      bindPtr->pickEvent_.xcrossing.detail = NotifyNonlinear;
      bindPtr->pickEvent_.xcrossing.same_screen = eventPtr->xmotion.same_screen;
      bindPtr->pickEvent_.xcrossing.focus = False;
      bindPtr->pickEvent_.xcrossing.state = eventPtr->xmotion.state;
    }
    else
      bindPtr->pickEvent_ = *eventPtr;
  }

  // If this is a recursive call (there's already a partially completed call
  // pending on the stack; it's in the middle of processing a Leave event
  // handler for the old current item) then just return; the pending call
  // will do everything that's needed.
  if (bindPtr->flags_ & REPICK_IN_PROGRESS)
    return;

  // A LeaveNotify event automatically means that there's no current item,
  // so the check for closest item can be skipped.
  ClientData newContext =NULL;
  ClientData newItem =NULL;
  if (bindPtr->pickEvent_.type != LeaveNotify) {
    int x = bindPtr->pickEvent_.xcrossing.x;
    int y = bindPtr->pickEvent_.xcrossing.y;
    newItem = (*bindPtr->pickProc_)(bindPtr->graphPtr_, x, y, &newContext);
  }

  // Nothing to do:  the current item hasn't changed.
  if (((newItem == bindPtr->currentItem_) && (newContext == bindPtr->currentContext_)) && ((bindPtr->flags_ & LEFT_GRABBED_ITEM) == 0))
    return;

  // Simulate a LeaveNotify event on the previous current item and an
  // EnterNotify event on the new current item.  Remove the "current" tag
  // from the previous current item and place it on the new current item.
  ClientData oldItem = bindPtr->currentItem_;
  Tcl_Preserve(oldItem);
  Tcl_Preserve(newItem);

  if ((bindPtr->currentItem_ != NULL) && ((newItem != bindPtr->currentItem_) || (newContext != bindPtr->currentContext_)) && ((bindPtr->flags_ & LEFT_GRABBED_ITEM) == 0)) {
    XEvent event = bindPtr->pickEvent_;
    event.type = LeaveNotify;

    // If the event's detail happens to be NotifyInferior the binding
    // mechanism will discard the event.  To be consistent, always use
    // NotifyAncestor.
    event.xcrossing.detail = NotifyAncestor;

    bindPtr->flags_ |= REPICK_IN_PROGRESS;
    bindPtr->doEvent(&event, bindPtr->currentItem_, bindPtr->currentContext_);
    bindPtr->flags_ &= ~REPICK_IN_PROGRESS;

    // Note: during DoEvent above, it's possible that bindPtr->newItem got
    // reset to NULL because the item was deleted.
  }

  if (((newItem != bindPtr->currentItem_) || (newContext != bindPtr->currentContext_)) && (buttonDown)) {
    bindPtr->flags_ |= LEFT_GRABBED_ITEM;
    XEvent event = bindPtr->pickEvent_;
    if ((newItem != bindPtr->newItem_) || (newContext != bindPtr->newContext_)) {

      // Generate <Enter> and <Leave> events for objects during button
      // grabs. This isn't standard. But for example, it allows one to
      // provide balloon help on the individual entries of the Hierbox
      // widget.
      ClientData savedItem = bindPtr->currentItem_;
      ClientData savedContext = bindPtr->currentContext_;
      if (bindPtr->newItem_ != NULL) {
	event.type = LeaveNotify;
	event.xcrossing.detail = NotifyVirtual; // Ancestor
	bindPtr->currentItem_ = bindPtr->newItem_;
	bindPtr->doEvent(&event, bindPtr->newItem_, bindPtr->newContext_);
      }

      bindPtr->newItem_ = newItem;
      bindPtr->newContext_ = newContext;
      if (newItem != NULL) {
	event.type = EnterNotify;
	event.xcrossing.detail = NotifyVirtual; // Ancestor
	bindPtr->currentItem_ = newItem;
	bindPtr->doEvent(&event, newItem, newContext);
      }

      bindPtr->currentItem_ = savedItem;
      bindPtr->currentContext_ = savedContext;
    }
    goto done;
  }

  // Special note:  
  // it's possible that bindPtr->newItem_ == bindPtr->currentItem_
  // here. This can happen, for example, if LEFT_GRABBED_ITEM was set.
  bindPtr->flags_ &= ~LEFT_GRABBED_ITEM;
  bindPtr->currentItem_ = bindPtr->newItem_ = newItem;
  bindPtr->currentContext_ = bindPtr->newContext_ = newContext;
  if (bindPtr->currentItem_ != NULL) {
    XEvent event = bindPtr->pickEvent_;
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
	bindPtr->state_ = eventPtr->xbutton.state;
	PickCurrentItem(bindPtr, eventPtr);
	bindPtr->state_ ^= mask;
	bindPtr->doEvent(eventPtr, 
			 bindPtr->currentItem_, bindPtr->currentContext_);
      }
      else {
	// Button release: first process the event, with the button still
	// considered to be down.  Then repick the current item under the
	// assumption that the button is no longer down.
	bindPtr->state_ = eventPtr->xbutton.state;
	bindPtr->doEvent(eventPtr, 
			 bindPtr->currentItem_, bindPtr->currentContext_);
	eventPtr->xbutton.state ^= mask;
	bindPtr->state_ = eventPtr->xbutton.state;
	PickCurrentItem(bindPtr, eventPtr);
	eventPtr->xbutton.state ^= mask;
      }
    }
    break;

  case EnterNotify:
  case LeaveNotify:
    bindPtr->state_ = eventPtr->xcrossing.state;
    PickCurrentItem(bindPtr, eventPtr);
    break;

  case MotionNotify:
    bindPtr->state_ = eventPtr->xmotion.state;
    PickCurrentItem(bindPtr, eventPtr);
    bindPtr->doEvent(eventPtr, bindPtr->currentItem_, bindPtr->currentContext_);
    break;

  case KeyPress:
  case KeyRelease:
    bindPtr->state_ = eventPtr->xkey.state;
    PickCurrentItem(bindPtr, eventPtr);
    bindPtr->doEvent(eventPtr, bindPtr->currentItem_, bindPtr->currentContext_);
    break;
  }

  Tcl_Release(bindPtr->graphPtr_);
}

