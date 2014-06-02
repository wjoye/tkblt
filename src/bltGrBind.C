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
#include "bltGrLegd.h"

static Tk_EventProc BindProc;

BindTable::BindTable(Graph* graphPtr, Pick* pickPtr)
{
  graphPtr_ = graphPtr;
  pickPtr_ = pickPtr;
  flags_ =0;
  table_ = Tk_CreateBindingTable(graphPtr->interp_);
  currentItem_ =NULL;
  currentContext_ =CID_NONE;
  newItem_ =NULL;
  newContext_ =CID_NONE;
  focusItem_ =NULL;
  focusContext_ =CID_NONE;
  //  pickEvent =NULL;
  state_ =0;

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

  if (currentItem_ == object) {
    currentItem_ =NULL;
    currentContext_ =CID_NONE;
  }

  if (newItem_ == object) {
    newItem_ =NULL;
    newContext_ =CID_NONE;
  }

  if (focusItem_ == object) {
    focusItem_ =NULL;
    focusContext_ =CID_NONE;
  }
}

void BindTable::doEvent(XEvent* eventPtr)
{
  if (!graphPtr_->tkwin_ || !table_)
    return;

  ClientData item = currentItem_;
  ClassId classId = currentContext_;

  if ((eventPtr->type == KeyPress) || (eventPtr->type == KeyRelease)) {
    item = focusItem_;
    classId = focusContext_;
  }
  if (!item)
    return;

  int nTags;
  const char** tagArray = graphPtr_->getTags(item, classId, &nTags);
  Tk_BindEvent(table_, eventPtr, graphPtr_->tkwin_, nTags, (void**)tagArray);
  if (tagArray)
    delete [] tagArray;
}

#define REPICK_IN_PROGRESS (1<<0)
#define LEFT_GRABBED_ITEM  (1<<1)

void BindTable::pickItem(XEvent* eventPtr)
{
  // This code comes from tkCanvas.c

  // Check whether or not a button is down.  If so, we'll log entry and exit
  // into and out of the current item, but not entry into any other item.
  // This implements a form of grabbing equivalent to what the X server does
  // for windows.

  int buttonDown = state_
    & (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask);

  // Save information about this event in the widget.  The event in the
  // widget is used for two purposes:
  // 1. Event bindings: if the current item changes, fake events are
  //    generated to allow item-enter and item-leave bindings to trigger.
  // 2. Reselection: if the current item gets deleted, can use the
  //    saved event to find a new current item.
  // Translate MotionNotify events into EnterNotify events, since that's
  // what gets reported to item handlers.

  if (eventPtr != &pickEvent_) {
    if ((eventPtr->type == MotionNotify) || (eventPtr->type == ButtonRelease)) {
      pickEvent_.xcrossing.type = EnterNotify;
      pickEvent_.xcrossing.serial = eventPtr->xmotion.serial;
      pickEvent_.xcrossing.send_event =	eventPtr->xmotion.send_event;
      pickEvent_.xcrossing.display = eventPtr->xmotion.display;
      pickEvent_.xcrossing.window = eventPtr->xmotion.window;
      pickEvent_.xcrossing.root = eventPtr->xmotion.root;
      pickEvent_.xcrossing.subwindow = None;
      pickEvent_.xcrossing.time = eventPtr->xmotion.time;
      pickEvent_.xcrossing.x = eventPtr->xmotion.x;
      pickEvent_.xcrossing.y = eventPtr->xmotion.y;
      pickEvent_.xcrossing.x_root = eventPtr->xmotion.x_root;
      pickEvent_.xcrossing.y_root = eventPtr->xmotion.y_root;
      pickEvent_.xcrossing.mode = NotifyNormal;
      pickEvent_.xcrossing.detail = NotifyNonlinear;
      pickEvent_.xcrossing.same_screen = eventPtr->xmotion.same_screen;
      pickEvent_.xcrossing.focus = False;
      pickEvent_.xcrossing.state = eventPtr->xmotion.state;
    }
    else
      pickEvent_ = *eventPtr;
  }

  // If this is a recursive call (there's already a partially completed call
  // pending on the stack; it's in the middle of processing a Leave event
  // handler for the old current item) then just return; the pending call
  // will do everything that's needed.
  if (flags_ & REPICK_IN_PROGRESS)
    return;

  // A LeaveNotify event automatically means that there's no current item,
  // so the check for closest item can be skipped.
  if (pickEvent_.type != LeaveNotify) {
    int x = pickEvent_.xcrossing.x;
    int y = pickEvent_.xcrossing.y;
    newItem_ = pickPtr_->pickEntry(x, y, &newContext_);
  }
  else {
    newItem_ =NULL;
    newContext_ = CID_NONE;
  }

  // Nothing to do: the current item hasn't changed.
  if ((newItem_ == currentItem_) && !(flags_ & LEFT_GRABBED_ITEM))
    return;

  if (!buttonDown)
    flags_ &= ~LEFT_GRABBED_ITEM;

  if ((newItem_ != currentItem_) && buttonDown) {
    flags_ |= LEFT_GRABBED_ITEM;
    return;
  }

  // Simulate a LeaveNotify event on the previous current item and an
  // EnterNotify event on the new current item.  Remove the "current" tag
  // from the previous current item and place it on the new current item.
  if (currentItem_ && (newItem_ != currentItem_) && !(flags_ & LEFT_GRABBED_ITEM)) {
    XEvent event = pickEvent_;
    event.type = LeaveNotify;

    // If the event's detail happens to be NotifyInferior the binding
    // mechanism will discard the event.  To be consistent, always use
    // NotifyAncestor.
    event.xcrossing.detail = NotifyAncestor;
    flags_ |= REPICK_IN_PROGRESS;
    doEvent(&event);
    flags_ &= ~REPICK_IN_PROGRESS;

    // Note: during DoEvent above, it's possible that newItem got
    // reset to NULL because the item was deleted.
    if ((newItem_ != currentItem_) && buttonDown) {
      flags_ |= LEFT_GRABBED_ITEM;
      return;
    }
  }

  // Special note:  
  // it's possible that newItem_ == currentItem_
  // here. This can happen, for example, if LEFT_GRABBED_ITEM was set.
  flags_ &= ~LEFT_GRABBED_ITEM;
  currentItem_ = newItem_;
  currentContext_ = newContext_;
  if (currentItem_ != NULL) {
    XEvent event = pickEvent_;
    event.type = EnterNotify;
    event.xcrossing.detail = NotifyAncestor;
    doEvent(&event);
  }
}

static void BindProc(ClientData clientData, XEvent* eventPtr)
{
  // This code comes from tkCanvas.c

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
	bindPtr->pickItem(eventPtr);
	bindPtr->state_ ^= mask;
	bindPtr->doEvent(eventPtr);
      }
      else {
	// Button release: first process the event, with the button still
	// considered to be down.  Then repick the current item under the
	// assumption that the button is no longer down.
	bindPtr->state_ = eventPtr->xbutton.state;
	bindPtr->doEvent(eventPtr);
	eventPtr->xbutton.state ^= mask;
	bindPtr->state_ = eventPtr->xbutton.state;
	bindPtr->pickItem(eventPtr);
	eventPtr->xbutton.state ^= mask;
      }
    }
    break;

  case EnterNotify:
  case LeaveNotify:
    bindPtr->state_ = eventPtr->xcrossing.state;
    bindPtr->pickItem(eventPtr);
    break;

  case MotionNotify:
    bindPtr->state_ = eventPtr->xmotion.state;
    bindPtr->pickItem(eventPtr);
    bindPtr->doEvent(eventPtr);
    break;

  case KeyPress:
  case KeyRelease:
    bindPtr->state_ = eventPtr->xkey.state;
    bindPtr->pickItem(eventPtr);
    bindPtr->doEvent(eventPtr);
    break;
  }

  Tcl_Release(bindPtr->graphPtr_);
}

