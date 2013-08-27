
/*
 * bltBind.c --
 *
 *	This module implements object binding procedures for the BLT toolkit.
 *
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

#include "bltInt.h"
#include "bltBind.h"
#include <bltList.h>

static Tk_EventProc BindProc;

typedef struct _Blt_BindTable BindTable;

/* Make button presses on objects have implicit grab. */
#define FULLY_SIMULATE_GRAB	1

/*
 * Binding table procedures.
 */
#define REPICK_IN_PROGRESS (1<<0)
#define LEFT_GRABBED_ITEM  (1<<1)

#define ALL_BUTTONS_MASK \
	(Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)

#ifndef VirtualEventMask
#define VirtualEventMask    (1L << 30)
#endif

#define ALL_VALID_EVENTS_MASK \
	(ButtonMotionMask | Button1MotionMask | Button2MotionMask | \
	 Button3MotionMask | Button4MotionMask | Button5MotionMask | \
	 ButtonPressMask | ButtonReleaseMask | EnterWindowMask | \
	 LeaveWindowMask | KeyPressMask | KeyReleaseMask | \
	 PointerMotionMask | VirtualEventMask)

static int buttonMasks[] =
{
    0,				/* No buttons pressed */
    Button1Mask, Button2Mask, Button3Mask, Button4Mask, Button5Mask,
};


/*
 * How to make drag&drop work?
 *
 *	Right now we generate pseudo <Enter> <Leave> events within button grab
 *	on an object.  They're marked NotifyVirtual instead of NotifyAncestor.
 *	A better solution: generate new-style virtual <<DragEnter>>
 *	<<DragMotion>> <<DragLeave>> events.  These virtual events don't have
 *	to exist as "real" event sequences, like virtual events do now.
 */

/*
 *---------------------------------------------------------------------------
 *
 * DoEvent --
 *
 *	This procedure is called to invoke binding processing for a new event
 *	that is associated with the current item for a legend.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the bindings for the legend.  A binding script could delete
 *	an entry, so callers should protect themselves with Tcl_Preserve and
 *	Tcl_Release.
 *
 *---------------------------------------------------------------------------
 */
static void
DoEvent(
    BindTable *bindPtr,		/* Binding information for widget in which
				 * event occurred. */
    XEvent *eventPtr,		/* Real or simulated X event that is to be
				 * processed. */
    ClientData item,		/* Item picked. */
    ClientData context)		/* Context of item.  */
{
    Blt_List tagList;

    if ((bindPtr->tkwin == NULL) || (bindPtr->bindingTable == NULL)) {
	return;
    }
    if ((eventPtr->type == KeyPress) || (eventPtr->type == KeyRelease)) {
	item = bindPtr->focusItem;
	context = bindPtr->focusContext;
    }
    if (item == NULL) {
	return;
    }
    /*
     * Invoke the binding system.
     */
    tagList = Blt_List_Create(BLT_ONE_WORD_KEYS);
    if (bindPtr->tagProc == NULL) {
	Blt_List_Append(tagList, Tk_GetUid("all"), 0);
	Blt_List_Append(tagList, (char *)item, 0);
    } else {
	(*bindPtr->tagProc) (bindPtr, item, context, tagList);
    }
    if (Blt_List_GetLength(tagList) > 0) {
	int nTags;
	ClientData *tagArray;
#define MAX_STATIC_TAGS	64
	ClientData staticTags[MAX_STATIC_TAGS];
	Blt_ListNode node;
	
	tagArray = staticTags;
	nTags = Blt_List_GetLength(tagList);
	if (nTags >= MAX_STATIC_TAGS) {
	    tagArray = Blt_AssertMalloc(sizeof(ClientData) * nTags);
	    
	} 
	nTags = 0;
	for (node = Blt_List_FirstNode(tagList); node != NULL;
	     node = Blt_List_NextNode(node)) {
	    tagArray[nTags++] = (ClientData)Blt_List_GetKey(node);
	}
	Tk_BindEvent(bindPtr->bindingTable, eventPtr, bindPtr->tkwin, nTags, 
		tagArray);
	if (tagArray != staticTags) {
	    Blt_Free(tagArray);
	}
    }
    Blt_List_Destroy(tagList);
}

/*
 *---------------------------------------------------------------------------
 *
 * PickCurrentItem --
 *
 *	Find the topmost item in a legend that contains a given location and
 *	mark the the current item.  If the current item has changed, generate
 *	a fake exit event on the old current item and a fake enter event on
 *	the new current item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The current item may change.  If it does, then the commands associated
 *	with item entry and exit could do just about anything.  A binding
 *	script could delete the legend, so callers should protect themselves
 *	with Tcl_Preserve and Tcl_Release.
 *
 *---------------------------------------------------------------------------
 */
static void
PickCurrentItem(
    BindTable *bindPtr,		/* Binding table information. */
    XEvent *eventPtr)		/* Event describing location of mouse cursor.
				 * Must be EnterWindow, LeaveWindow,
				 * ButtonRelease, or MotionNotify. */
{
    int buttonDown;
    ClientData newItem, oldItem;
    ClientData newContext;

    /*
     * Check whether or not a button is down.  If so, we'll log entry and exit
     * into and out of the current item, but not entry into any other item.
     * This implements a form of grabbing equivalent to what the X server does
     * for windows.
     */
    buttonDown = (bindPtr->state & ALL_BUTTONS_MASK);
    if (!buttonDown) {
	bindPtr->flags &= ~LEFT_GRABBED_ITEM;
    }

    /*
     * Save information about this event in the widget.  The event in the
     * widget is used for two purposes:
     *
     * 1. Event bindings: if the current item changes, fake events are
     *    generated to allow item-enter and item-leave bindings to trigger.
     * 2. Reselection: if the current item gets deleted, can use the
     *    saved event to find a new current item.
     * Translate MotionNotify events into EnterNotify events, since that's
     * what gets reported to item handlers.
     */

    if (eventPtr != &bindPtr->pickEvent) {
	if ((eventPtr->type == MotionNotify) ||
	    (eventPtr->type == ButtonRelease)) {
	    bindPtr->pickEvent.xcrossing.type = EnterNotify;
	    bindPtr->pickEvent.xcrossing.serial = eventPtr->xmotion.serial;
	    bindPtr->pickEvent.xcrossing.send_event =
		eventPtr->xmotion.send_event;
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
	    bindPtr->pickEvent.xcrossing.same_screen
		= eventPtr->xmotion.same_screen;
	    bindPtr->pickEvent.xcrossing.focus = False;
	    bindPtr->pickEvent.xcrossing.state = eventPtr->xmotion.state;
	} else {
	    bindPtr->pickEvent = *eventPtr;
	}
    }
    bindPtr->activePick = TRUE;

    /*
     * If this is a recursive call (there's already a partially completed call
     * pending on the stack; it's in the middle of processing a Leave event
     * handler for the old current item) then just return; the pending call
     * will do everything that's needed.
     */
    if (bindPtr->flags & REPICK_IN_PROGRESS) {
	return;
    }
    /*
     * A LeaveNotify event automatically means that there's no current item,
     * so the check for closest item can be skipped.
     */
    newContext = NULL;
    if (bindPtr->pickEvent.type != LeaveNotify) {
	int x, y;

	x = bindPtr->pickEvent.xcrossing.x;
	y = bindPtr->pickEvent.xcrossing.y;
	newItem = (*bindPtr->pickProc) (bindPtr->clientData, x, y, &newContext);
    } else {
	newItem = NULL;
    }
    if (((newItem == bindPtr->currentItem) && 
	 (newContext == bindPtr->currentContext)) && 
	((bindPtr->flags & LEFT_GRABBED_ITEM) == 0)) {
	/*
	 * Nothing to do:  the current item hasn't changed.
	 */
	return;
    }
#if FULLY_SIMULATE_GRAB
    if (((newItem != bindPtr->currentItem) || 
	 (newContext != bindPtr->currentContext)) && 
	(buttonDown)) {
	bindPtr->flags |= LEFT_GRABBED_ITEM;
#ifdef notdef
	fprintf(stderr, "pickcurrentitem: simulate grab.\n");
#endif
	return;
    }
#endif
    /*
     * Simulate a LeaveNotify event on the previous current item and an
     * EnterNotify event on the new current item.  Remove the "current" tag
     * from the previous current item and place it on the new current item.
     */
    oldItem = bindPtr->currentItem;
    Tcl_Preserve(oldItem);
    Tcl_Preserve(newItem);

    if ((bindPtr->currentItem != NULL) &&
	((newItem != bindPtr->currentItem) || 
	 (newContext != bindPtr->currentContext)) && 
	((bindPtr->flags & LEFT_GRABBED_ITEM) == 0)) {
	XEvent event;

	event = bindPtr->pickEvent;
	event.type = LeaveNotify;
	/*
	 * If the event's detail happens to be NotifyInferior the binding
	 * mechanism will discard the event.  To be consistent, always use
	 * NotifyAncestor.
	 */
	event.xcrossing.detail = NotifyAncestor;

	bindPtr->flags |= REPICK_IN_PROGRESS;
	DoEvent(bindPtr, &event, bindPtr->currentItem, bindPtr->currentContext);
	bindPtr->flags &= ~REPICK_IN_PROGRESS;

	/*
	 * Note: during DoEvent above, it's possible that bindPtr->newItem got
	 * reset to NULL because the item was deleted.
	 */
    }
    if (((newItem != bindPtr->currentItem) || 
	 (newContext != bindPtr->currentContext)) && 
	(buttonDown)) {
	XEvent event;

	bindPtr->flags |= LEFT_GRABBED_ITEM;
	event = bindPtr->pickEvent;
	if ((newItem != bindPtr->newItem) || 
	    (newContext != bindPtr->newContext)) {
	    ClientData savedItem;
	    ClientData savedContext;

	    /*
	     * Generate <Enter> and <Leave> events for objects during button
	     * grabs.  This isn't standard. But for example, it allows one to
	     * provide balloon help on the individual entries of the Hierbox
	     * widget.
	     */
	    savedItem = bindPtr->currentItem;
	    savedContext = bindPtr->currentContext;
	    if (bindPtr->newItem != NULL) {
		event.type = LeaveNotify;
		event.xcrossing.detail = NotifyVirtual /* Ancestor */ ;
		bindPtr->currentItem = bindPtr->newItem;
		DoEvent(bindPtr, &event, bindPtr->newItem, bindPtr->newContext);
	    }
	    bindPtr->newItem = newItem;
	    bindPtr->newContext = newContext;
	    if (newItem != NULL) {
		event.type = EnterNotify;
		event.xcrossing.detail = NotifyVirtual /* Ancestor */ ;
		bindPtr->currentItem = newItem;
		DoEvent(bindPtr, &event, newItem, newContext);
	    }
	    bindPtr->currentItem = savedItem;
	    bindPtr->currentContext = savedContext;
	}
	goto done;
    }
    /*
     * Special note:  it's possible that
     *		bindPtr->newItem == bindPtr->currentItem
     * here.  This can happen, for example, if LEFT_GRABBED_ITEM was set.
     */

    bindPtr->flags &= ~LEFT_GRABBED_ITEM;
    bindPtr->currentItem = bindPtr->newItem = newItem;
    bindPtr->currentContext = bindPtr->newContext = newContext;
    if (bindPtr->currentItem != NULL) {
	XEvent event;

	event = bindPtr->pickEvent;
	event.type = EnterNotify;
	event.xcrossing.detail = NotifyAncestor;
	DoEvent(bindPtr, &event, newItem, newContext);
#ifdef notdef
	if ((eventPtr->type == MotionNotify) ||
	    (eventPtr->type == ButtonRelease)) {
	    fprintf(stderr, "pickcurrentitem: DoEvent Button buttondown=%d.\n",
		    buttonDown);
	    event.type = eventPtr->type;
	    event.xbutton.button = eventPtr->xbutton.button;
	    DoEvent(bindPtr, &event, newItem, newContext);
	    fprintf(stderr, "pickcurrentitem: done.\n");
	}
#endif
    }
 done:
    Tcl_Release(newItem);
    Tcl_Release(oldItem);
}

/*
 *---------------------------------------------------------------------------
 *
 * BindProc --
 *
 *	This procedure is invoked by the Tk dispatcher to handle events
 *	associated with bindings on items.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the command invoked as part of the binding
 *	(if there was any).
 *
 *---------------------------------------------------------------------------
 */
static void
BindProc(
    ClientData clientData,	/* Pointer to widget structure. */
    XEvent *eventPtr)		/* Pointer to X event that just happened. */
{
    BindTable *bindPtr = clientData;
    int mask;

    Tcl_Preserve(bindPtr->clientData);
    /*
     * This code below keeps track of the current modifier state in
     * bindPtr->state.  This information is used to defer repicks of the
     * current item while buttons are down.
     */
    switch (eventPtr->type) {
    case ButtonPress:
    case ButtonRelease:
	mask = 0;
	if ((eventPtr->xbutton.button >= Button1) &&
	    (eventPtr->xbutton.button <= Button5)) {
	    mask = buttonMasks[eventPtr->xbutton.button];
	}
	/*
	 * For button press events, repick the current item using the button
	 * state before the event, then process the event.  For button release
	 * events, first process the event, then repick the current item using
	 * the button state *after* the event (the button has logically gone
	 * up before we change the current item).
	 */
	if (eventPtr->type == ButtonPress) {

	    /*
	     * On a button press, first repick the current item using the
	     * button state before the event, the process the event.
	     */

	    bindPtr->state = eventPtr->xbutton.state;
	    PickCurrentItem(bindPtr, eventPtr);
	    bindPtr->state ^= mask;
	    DoEvent(bindPtr, eventPtr, bindPtr->currentItem, 
		bindPtr->currentContext);

	} else {

	    /*
	     * Button release: first process the event, with the button still
	     * considered to be down.  Then repick the current item under the
	     * assumption that the button is no longer down.
	     */
	    bindPtr->state = eventPtr->xbutton.state;
	    DoEvent(bindPtr, eventPtr, bindPtr->currentItem, 
		bindPtr->currentContext);
	    eventPtr->xbutton.state ^= mask;
	    bindPtr->state = eventPtr->xbutton.state;
	    PickCurrentItem(bindPtr, eventPtr);
	    eventPtr->xbutton.state ^= mask;
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
	DoEvent(bindPtr, eventPtr, bindPtr->currentItem, 
		bindPtr->currentContext);
	break;

    case KeyPress:
    case KeyRelease:
	bindPtr->state = eventPtr->xkey.state;
	PickCurrentItem(bindPtr, eventPtr);
	DoEvent(bindPtr, eventPtr, bindPtr->currentItem, 
		bindPtr->currentContext);
	break;
    }
    Tcl_Release(bindPtr->clientData);
}

int
Blt_ConfigureBindings(
    Tcl_Interp *interp,
    BindTable *bindPtr,
    ClientData item,
    int argc,
    const char **argv)
{
    const char *command;
    unsigned long mask;
    const char *seq;

    if (argc == 0) {
	Tk_GetAllBindings(interp, bindPtr->bindingTable, item);
	return TCL_OK;
    }
    if (argc == 1) {
	command = Tk_GetBinding(interp, bindPtr->bindingTable, item, argv[0]);
	if (command == NULL) {
	    Tcl_AppendResult(interp, "can't find event \"", argv[0], "\"",
			     (char *)NULL);
	    return TCL_ERROR;
	}
	Tcl_SetStringObj(Tcl_GetObjResult(interp), command, -1);
	return TCL_OK;
    }

    seq = argv[0];
    command = argv[1];

    if (command[0] == '\0') {
	return Tk_DeleteBinding(interp, bindPtr->bindingTable, item, seq);
    }

    if (command[0] == '+') {
	mask = Tk_CreateBinding(interp, bindPtr->bindingTable, item, seq,
		command + 1, TRUE);
    } else {
	mask = Tk_CreateBinding(interp, bindPtr->bindingTable, item, seq,
		command, FALSE);
    }
    if (mask == 0) {
	Tcl_AppendResult(interp, "event mask can't be zero for \"", item, "\"",
			     (char *)NULL);
	return TCL_ERROR;
    }
    if (mask & (unsigned)~ALL_VALID_EVENTS_MASK) {
	Tk_DeleteBinding(interp, bindPtr->bindingTable, item, seq);
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "requested illegal events; ",
		 "only key, button, motion, enter, leave, and virtual ",
		 "events may be used", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}


int
Blt_ConfigureBindingsFromObj(
    Tcl_Interp *interp,
    BindTable *bindPtr,
    ClientData item,
    int objc,
    Tcl_Obj *const *objv)
{
    const char *command;
    unsigned long mask;
    const char *seq;
    const char *string;

    if (objc == 0) {
	Tk_GetAllBindings(interp, bindPtr->bindingTable, item);
	return TCL_OK;
    }
    string = Tcl_GetString(objv[0]);
    if (objc == 1) {
	command = Tk_GetBinding(interp, bindPtr->bindingTable, item, string);
	if (command == NULL) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "invalid binding event \"", string, "\"", 
		(char *)NULL);
	    return TCL_ERROR;
	}
	Tcl_SetStringObj(Tcl_GetObjResult(interp), command, -1);
	return TCL_OK;
    }

    seq = string;
    command = Tcl_GetString(objv[1]);

    if (command[0] == '\0') {
	return Tk_DeleteBinding(interp, bindPtr->bindingTable, item, seq);
    }

    if (command[0] == '+') {
	mask = Tk_CreateBinding(interp, bindPtr->bindingTable, item, seq,
		command + 1, TRUE);
    } else {
	mask = Tk_CreateBinding(interp, bindPtr->bindingTable, item, seq,
		command, FALSE);
    }
    if (mask == 0) {
	return TCL_ERROR;
    }
    if (mask & (unsigned)~ALL_VALID_EVENTS_MASK) {
	Tk_DeleteBinding(interp, bindPtr->bindingTable, item, seq);
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "requested illegal events; ",
		 "only key, button, motion, enter, leave, and virtual ",
		 "events may be used", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

Blt_BindTable
Blt_CreateBindingTable(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    ClientData clientData,
    Blt_BindPickProc *pickProc,
    Blt_BindTagProc *tagProc)
{
    unsigned int mask;
    BindTable *bindPtr;

    bindPtr = Blt_AssertCalloc(1, sizeof(BindTable));
    bindPtr->bindingTable = Tk_CreateBindingTable(interp);
    bindPtr->clientData = clientData;
    bindPtr->tkwin = tkwin;
    bindPtr->pickProc = pickProc;
    bindPtr->tagProc = tagProc;
    mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
	ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
	PointerMotionMask);
    Tk_CreateEventHandler(tkwin, mask, BindProc, bindPtr);
    return bindPtr;
}

void
Blt_DestroyBindingTable(BindTable *bindPtr)
{
    unsigned int mask;

    Tk_DeleteBindingTable(bindPtr->bindingTable);
    mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
	ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
	PointerMotionMask);
    Tk_DeleteEventHandler(bindPtr->tkwin, mask, BindProc, bindPtr);
    Blt_Free(bindPtr);
}

void
Blt_PickCurrentItem(BindTable *bindPtr)
{
    if (bindPtr->activePick) {
	PickCurrentItem(bindPtr, &bindPtr->pickEvent);
    }
}

void
Blt_DeleteBindings(
    BindTable *bindPtr,
    ClientData object)
{
    Tk_DeleteAllBindings(bindPtr->bindingTable, object);

    /*
     * If this is the object currently picked, we need to repick one.
     */
    if (bindPtr->currentItem == object) {
	bindPtr->currentItem = NULL;
	bindPtr->currentContext = NULL;
    }
    if (bindPtr->newItem == object) {
	bindPtr->newItem = NULL;
	bindPtr->newContext = NULL;
    }
    if (bindPtr->focusItem == object) {
	bindPtr->focusItem = NULL;
	bindPtr->focusContext = NULL;
    }
}

void
Blt_MoveBindingTable(
    BindTable *bindPtr,
    Tk_Window tkwin)
{
    unsigned int mask;

    mask = (KeyPressMask | KeyReleaseMask | ButtonPressMask |
	ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
	PointerMotionMask);
    if (bindPtr->tkwin != NULL) {
	Tk_DeleteEventHandler(bindPtr->tkwin, mask, BindProc, bindPtr);
    }
    Tk_CreateEventHandler(tkwin, mask, BindProc, bindPtr);
    bindPtr->tkwin = tkwin;
}

/*
 * The following union is used to hold the detail information from an
 * XEvent (including Tk's XVirtualEvent extension).
 */
typedef union {
    KeySym	keySym;	    /* KeySym that corresponds to xkey.keycode. */
    int		button;	    /* Button that was pressed (xbutton.button). */
    Tk_Uid	name;	    /* Tk_Uid of virtual event. */
    ClientData	clientData; /* Used when type of Detail is unknown, and to
			     * ensure that all bytes of Detail are initialized
			     * when this structure is used in a hash key. */
} Detail;


/*
 * The following structure defines a pattern, which is matched against X
 * events as part of the process of converting X events into TCL commands.
 */
typedef struct {
    int eventType;		/* Type of X event, e.g. ButtonPress. */
    int needMods;		/* Mask of modifiers that must be
				 * present (0 means no modifiers are
				 * required). */
    Detail detail;		/* Additional information that must
				 * match event.  Normally this is 0,
				 * meaning no additional information
				 * must match.  For KeyPress and
				 * KeyRelease events, a keySym may
				 * be specified to select a
				 * particular keystroke (0 means any
				 * keystrokes).  For button events,
				 * specifies a particular button (0
				 * means any buttons are OK).  For virtual
				 * events, specifies the Tk_Uid of the
				 * virtual event name (never 0). */
} Pattern;

typedef struct {
    const char *name;		/* Name of modifier. */
    int mask;			/* Button/modifier mask value, such as
				 * Button1Mask. */
    int flags;			/* Various flags; see below for
				 * definitions. */
} EventModifier;

/*
 * Flags for EventModifier structures:
 *
 * DOUBLE -		Non-zero means duplicate this event,
 *			e.g. for double-clicks.
 * TRIPLE -		Non-zero means triplicate this event,
 *			e.g. for triple-clicks.
 * QUADRUPLE -		Non-zero means quadruple this event,
 *			e.g. for 4-fold-clicks.
 * MULT_CLICKS -	Combination of all of above.
 */

#define DOUBLE		(1<<0)
#define TRIPLE		(1<<1)
#define QUADRUPLE	(1<<2)
#define MULT_CLICKS	(DOUBLE|TRIPLE|QUADRUPLE)

#define META_MASK	(AnyModifier<<1)
#define ALT_MASK	(AnyModifier<<2)

static EventModifier eventModifiers[] = {
    {"Alt",		ALT_MASK,	0},
    {"Any",		0,		0},	/* Ignored: historical relic. */
    {"B1",		Button1Mask,	0},
    {"B2",		Button2Mask,	0},
    {"B3",		Button3Mask,	0},
    {"B4",		Button4Mask,	0},
    {"B5",		Button5Mask,	0},
    {"Button1",		Button1Mask,	0},
    {"Button2",		Button2Mask,	0},
    {"Button3",		Button3Mask,	0},
    {"Button4",		Button4Mask,	0},
    {"Button5",		Button5Mask,	0},
    {"Command",		Mod1Mask,	0},
    {"Control",		ControlMask,	0},
    {"Double",		0,		DOUBLE},
    {"Lock",		LockMask,	0},
    {"M",		META_MASK,	0},
    {"M1",		Mod1Mask,	0},
    {"M2",		Mod2Mask,	0},
    {"M3",		Mod3Mask,	0},
    {"M4",		Mod4Mask,	0},
    {"M5",		Mod5Mask,	0},
    {"Meta",		META_MASK,	0},
    {"Mod1",		Mod1Mask,	0},
    {"Mod2",		Mod2Mask,	0},
    {"Mod3",		Mod3Mask,	0},
    {"Mod4",		Mod4Mask,	0},
    {"Mod5",		Mod5Mask,	0},
    {"Option",		Mod2Mask,	0},
    {"Quadruple",	0,		QUADRUPLE},
    {"Shift",		ShiftMask,	0},
    {"Triple",		0,		TRIPLE},
};

typedef struct {
    const char *name;		/* Name of event. */
    int type;			/* Event type for X, such as
				 * ButtonPress. */
    int eventMask;		/* Mask bits (for XSelectInput)
				 * for this event type. */
} EventInfo;

/*
 * Note:  some of the masks below are an OR-ed combination of
 * several masks.  This is necessary because X doesn't report
 * up events unless you also ask for down events.  Also, X
 * doesn't report button state in motion events unless you've
 * asked about button events.
 */

static EventInfo events[] = {
    {"Activate",	ActivateNotify,		ActivateMask},
    {"Button",		ButtonPress,		ButtonPressMask},
    {"ButtonPress",	ButtonPress,		ButtonPressMask},
    {"ButtonRelease",	ButtonRelease, ButtonPressMask|ButtonReleaseMask},
    {"Circulate",	CirculateNotify,	StructureNotifyMask},
    {"CirculateRequest", CirculateRequest,	SubstructureRedirectMask},
    {"Colormap",	ColormapNotify,		ColormapChangeMask},
    {"Configure",	ConfigureNotify,	StructureNotifyMask},
    {"ConfigureRequest", ConfigureRequest,	SubstructureRedirectMask},
    {"Create",		CreateNotify,		SubstructureNotifyMask},
    {"Deactivate",	DeactivateNotify,	ActivateMask},
    {"Destroy",		DestroyNotify,		StructureNotifyMask},
    {"Enter",		EnterNotify,		EnterWindowMask},
    {"Expose",		Expose,			ExposureMask},
    {"FocusIn",		FocusIn,		FocusChangeMask},
    {"FocusOut",	FocusOut,		FocusChangeMask},
    {"Gravity",		GravityNotify,		StructureNotifyMask},
    {"Key",		KeyPress,		KeyPressMask},
    {"KeyPress",	KeyPress,		KeyPressMask},
    {"KeyRelease",	KeyRelease,		KeyPressMask|KeyReleaseMask},
    {"Leave",		LeaveNotify,		LeaveWindowMask},
    {"Map",		MapNotify,		StructureNotifyMask},
    {"MapRequest",	MapRequest,             SubstructureRedirectMask},
    {"Motion",		MotionNotify, ButtonPressMask|PointerMotionMask},
    {"MouseWheel",	MouseWheelEvent,	MouseWheelMask},
    {"Property",	PropertyNotify,		PropertyChangeMask},
    {"Reparent",	ReparentNotify,		StructureNotifyMask},
    {"ResizeRequest",	ResizeRequest,		ResizeRedirectMask},
    {"Unmap",		UnmapNotify,		StructureNotifyMask},
    {"Visibility",	VisibilityNotify,	VisibilityChangeMask},
};

/*
 * The defines and table below are used to classify events into
 * various groups.  The reason for this is that logically identical
 * fields (e.g. "state") appear at different places in different
 * types of events.  The classification masks can be used to figure
 * out quickly where to extract information from events.
 */

#define KEY			0x1
#define BUTTON			0x2
#define MOTION			0x4
#define CROSSING		0x8
#define FOCUS			0x10
#define EXPOSE			0x20
#define VISIBILITY		0x40
#define CREATE			0x80
#define DESTROY			0x100
#define UNMAP			0x200
#define MAP			0x400
#define REPARENT		0x800
#define CONFIG			0x1000
#define GRAVITY			0x2000
#define CIRC			0x4000
#define PROP			0x8000
#define COLORMAP		0x10000
#define VIRTUAL			0x20000
#define ACTIVATE		0x40000
#define	MAPREQ			0x80000
#define	CONFIGREQ		0x100000
#define	RESIZEREQ		0x200000
#define CIRCREQ			0x400000

#define KEY_BUTTON_MOTION_VIRTUAL	(KEY|BUTTON|MOTION|VIRTUAL)
#define KEY_BUTTON_MOTION_CROSSING	(KEY|BUTTON|MOTION|CROSSING|VIRTUAL)

static int flagArray[TK_LASTEVENT+1] = {
   /* Not used */		0,
   /* Not used */		0,
   /* KeyPress */		KEY,
   /* KeyRelease */		KEY,
   /* ButtonPress */		BUTTON,
   /* ButtonRelease */		BUTTON,
   /* MotionNotify */		MOTION,
   /* EnterNotify */		CROSSING,
   /* LeaveNotify */		CROSSING,
   /* FocusIn */		FOCUS,
   /* FocusOut */		FOCUS,
   /* KeymapNotify */		0,
   /* Expose */			EXPOSE,
   /* GraphicsExpose */		EXPOSE,
   /* NoExpose */		0,
   /* VisibilityNotify */	VISIBILITY,
   /* CreateNotify */		CREATE,
   /* DestroyNotify */		DESTROY,
   /* UnmapNotify */		UNMAP,
   /* MapNotify */		MAP,
   /* MapRequest */		MAPREQ,
   /* ReparentNotify */		REPARENT,
   /* ConfigureNotify */	CONFIG,
   /* ConfigureRequest */	CONFIGREQ,
   /* GravityNotify */		GRAVITY,
   /* ResizeRequest */		RESIZEREQ,
   /* CirculateNotify */	CIRC,
   /* CirculateRequest */	0,
   /* PropertyNotify */		PROP,
   /* SelectionClear */		0,
   /* SelectionRequest */	0,
   /* SelectionNotify */	0,
   /* ColormapNotify */		COLORMAP,
   /* ClientMessage */		0,
   /* MappingNotify */		0,
#ifdef GenericEvent
   /* GenericEvent */		0,
#endif
   /* VirtualEvent */		VIRTUAL,
   /* Activate */		ACTIVATE,	    
   /* Deactivate */		ACTIVATE,
   /* MouseWheel */		KEY
};


static EventModifier *
FindModifier(const char *string)
{
    int high, low;
    char c;

    low = 0;
    high = (sizeof(eventModifiers) / sizeof(EventModifier)) - 1;
    c = string[0];
    while (low <= high) {
	EventModifier *modPtr;
	int compare;
	int median;
	
	median = (low + high) >> 1;
	modPtr = eventModifiers + median;

	/* Test the first character */
	compare = c - modPtr->name[0];
	if (compare == 0) {
	    compare = strcmp(string, modPtr->name);
	}
	if (compare < 0) {
	    high = median - 1;
	} else if (compare > 0) {
	    low = median + 1;
	} else {
	    return modPtr;	/* Modifier found. */
	}
    }
    return NULL;		/* Can't find modifier */
}

static EventInfo *
FindEvent(const char *string)
{
    int high, low;
    char c;

    low = 0;
    high = (sizeof(events) / sizeof(EventInfo)) - 1;
    c = string[0];
    while (low <= high) {
	EventInfo *infoPtr;
	int compare;
	int median;
	
	median = (low + high) >> 1;
	infoPtr = events + median;

	/* Test the first character */
	compare = c - infoPtr->name[0];
	if (compare == 0) {
	    compare = strcmp(string, infoPtr->name);
	}
	if (compare < 0) {
	    high = median - 1;
	} else if (compare > 0) {
	    low = median + 1;
	} else {
	    return infoPtr;	/* Event found. */
	}
    }
    return NULL;		/* Can't find event. */
}


/*
 *----------------------------------------------------------------------
 *
 * GetField --
 *
 *	Used to parse pattern descriptions.  Copies up to
 *	size characters from p to copy, stopping at end of
 *	string, space, "-", ">", or whenever size is
 *	exceeded.
 *
 * Results:
 *	The return value is a pointer to the character just
 *	after the last one copied (usually "-" or space or
 *	">", but could be anything if size was exceeded).
 *	Also places NULL-terminated string (up to size
 *	character, including NULL), at copy.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
GetField(p, copy, size)
    char *p;			/* Pointer to part of pattern. */
    char *copy;			/* Place to copy field. */
    int size;			/* Maximum number of characters to
				 * copy. */
{
    while ((*p != '\0') && !isspace(UCHAR(*p)) && (*p != '>')
	    && (*p != '-') && (size > 1)) {
	*copy = *p;
	p++;
	copy++;
	size--;
    }
    *copy = '\0';
    return p;
}

static int
ParseEventDescription(Tcl_Interp *interp, const char **eventStringPtr,
		      Pattern *patPtr, unsigned long *eventMaskPtr)
{
    char *p;
    unsigned long eventMask;
    int count, eventFlags;
#define FIELD_SIZE 48
    char field[FIELD_SIZE];
    EventInfo *infoPtr;

    Tcl_DString copy;
    Tcl_DStringInit(&copy);
    p = Tcl_DStringAppend(&copy, *eventStringPtr, -1);

    patPtr->eventType = -1;
    patPtr->needMods = 0;
    patPtr->detail.clientData = 0;

    eventMask = 0;
    count = 1;
    
    /*
     * Handle simple ASCII characters.
     */

    if (*p != '<') {
	char string[2];

	patPtr->eventType = KeyPress;
	eventMask = KeyPressMask;
	string[0] = *p;
	string[1] = 0;
	patPtr->detail.keySym = XStringToKeysym(string);
	if (patPtr->detail.keySym == NoSymbol) {
	    if (isprint(UCHAR(*p))) {
		patPtr->detail.keySym = *p;
	    } else {
		char buf[64];
		
		sprintf(buf, "bad ASCII character 0x%x", (unsigned char) *p);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		count = 0;
		goto done;
	    }
	}
	p++;
	goto end;
    }

    /*
     * A physical event description consists of:
     *
     * 1. open angle bracket.
     * 2. any number of modifiers, each followed by spaces
     *    or dashes.
     * 3. an optional event name.
     * 4. an option button or keysym name.  Either this or
     *    item 3 *must* be present;  if both are present
     *    then they are separated by spaces or dashes.
     * 5. a close angle bracket.
     */

    p++;

    while (1) {
	EventModifier *modPtr;
	p = GetField(p, field, FIELD_SIZE);
	if (*p == '>') {
	    /*
	     * This solves the problem of, e.g., <Control-M> being
	     * misinterpreted as Control + Meta + missing keysym
	     * instead of Control + KeyPress + M.
	     */
	     break;
	}
	modPtr = FindModifier(field);
	if (modPtr == NULL) {
	    break;
	}
	patPtr->needMods |= modPtr->mask;
	if (modPtr->flags & (MULT_CLICKS)) {
	    int i = modPtr->flags & MULT_CLICKS;
	    count = 2;
	    while (i >>= 1) count++;
	}
	while ((*p == '-') || isspace(UCHAR(*p))) {
	    p++;
	}
    }

    eventFlags = 0;
    infoPtr = FindEvent(field);
    if (infoPtr != NULL) {
	patPtr->eventType = infoPtr->type;
	eventFlags = flagArray[infoPtr->type];
	eventMask = infoPtr->eventMask;
	while ((*p == '-') || isspace(UCHAR(*p))) {
	    p++;
	}
	p = GetField(p, field, FIELD_SIZE);
    }
    if (*field != '\0') {
	if ((*field >= '1') && (*field <= '5') && (field[1] == '\0')) {
	    if (eventFlags == 0) {
		patPtr->eventType = ButtonPress;
		eventMask = ButtonPressMask;
	    } else if (eventFlags & KEY) {
		goto getKeysym;
	    } else if ((eventFlags & BUTTON) == 0) {
		Tcl_AppendResult(interp, "specified button \"", field,
			"\" for non-button event", (char *) NULL);
		count = 0;
		goto done;
	    }
	    patPtr->detail.button = (*field - '0');
	} else {
	    getKeysym:
	    patPtr->detail.keySym = XStringToKeysym(field);
	    if (patPtr->detail.keySym == NoSymbol) {
		Tcl_AppendResult(interp, "bad event type or keysym \"",
			field, "\"", (char *)NULL);
		count = 0;
		goto done;
	    }
	    if (eventFlags == 0) {
		patPtr->eventType = KeyPress;
		eventMask = KeyPressMask;
	    } else if ((eventFlags & KEY) == 0) {
		Tcl_AppendResult(interp, "specified keysym \"", field,
			"\" for non-key event", (char *)NULL);
		count = 0;
		goto done;
	    }
	}
    } else if (eventFlags == 0) {
	Tcl_AppendResult(interp, "no event type or button # or keysym", 
			 (char *)NULL);
	count = 0;
	goto done;
    }

    while ((*p == '-') || isspace(UCHAR(*p))) {
	p++;
    }
    if (*p != '>') {
	while (*p != '\0') {
	    p++;
	    if (*p == '>') {
		Tcl_AppendResult(interp, 
				 "extra characters after detail in binding",
				 (char *)NULL);
		count = 0;
		goto done;
	    }
	}
	Tcl_AppendResult(interp, "missing \">\" in binding", (char *)NULL);
	count = 0;
	goto done;
    }
    p++;

end:
    *eventStringPtr += (p - Tcl_DStringValue(&copy));
    *eventMaskPtr |= eventMask;
done:
    Tcl_DStringFree(&copy);
    return count;
}

typedef struct {
    int numKey;			/* Integer representation of a value. */
    const char *strKey;		/* String representation of a value. */
} TkStateMap;

static TkStateMap notifyMode[] = {
    {NotifyNormal,		"NotifyNormal"},
    {NotifyGrab,		"NotifyGrab"},
    {NotifyUngrab,		"NotifyUngrab"},
    {NotifyWhileGrabbed,	"NotifyWhileGrabbed"},
    {-1, NULL}
};

static TkStateMap notifyDetail[] = {
    {NotifyAncestor,		"NotifyAncestor"},
    {NotifyVirtual,		"NotifyVirtual"},
    {NotifyInferior,		"NotifyInferior"},
    {NotifyNonlinear,		"NotifyNonlinear"},
    {NotifyNonlinearVirtual,	"NotifyNonlinearVirtual"},
    {NotifyPointer,		"NotifyPointer"},
    {NotifyPointerRoot,		"NotifyPointerRoot"},
    {NotifyDetailNone,		"NotifyDetailNone"},
    {-1, NULL}
};

static TkStateMap circPlace[] = {
    {PlaceOnTop,		"PlaceOnTop"},
    {PlaceOnBottom,		"PlaceOnBottom"},
    {-1, NULL}
};

static TkStateMap visNotify[] = {
    {VisibilityUnobscured,	    "VisibilityUnobscured"},
    {VisibilityPartiallyObscured,   "VisibilityPartiallyObscured"},
    {VisibilityFullyObscured,	    "VisibilityFullyObscured"},
    {-1, NULL}
};

static TkStateMap configureRequestDetail[] = {
    {None,		"None"},
    {Above,		"Above"},
    {Below,		"Below"},
    {BottomIf,		"BottomIf"},
    {TopIf,		"TopIf"},
    {Opposite,		"Opposite"},
    {-1, NULL}
};

static TkStateMap propNotify[] = {
    {PropertyNewValue,	"NewValue"},
    {PropertyDelete,	"Delete"},
    {-1, NULL}
};

/*
 *---------------------------------------------------------------------------
 *
 * HandleEventGenerate --
 *
 *	Helper function for the "event generate" command.  Generate and
 *	process an XEvent, constructed from information parsed from the
 *	event description string and its optional arguments.
 *
 *	argv[0] contains name of the target window.
 *	argv[1] contains pattern string for one event (e.g, <Control-v>).
 *	argv[2..argc-1] contains -field/option pairs for specifying
 *		        additional detail in the generated event.
 *
 *	Either virtual or physical events can be generated this way.
 *	The event description string must contain the specification
 *	for only one event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When constructing the event, 
 *	 event.xany.serial is filled with the current X serial number.
 *	 event.xany.window is filled with the target window.
 *	 event.xany.display is filled with the target window's display.
 *	Any other fields in eventPtr which are not specified by the pattern
 *	string or the optional arguments, are set to 0.
 *
 *	The event may be handled sychronously or asynchronously, depending
 *	on the value specified by the optional "-when" option.  The
 *	default setting is synchronous.
 *
 *---------------------------------------------------------------------------
 */
static int
SendEventCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    XEvent event;    
    CONST char *p;
    char *name;
    Window window;
    Display *display;
    Tk_Window tkwin;
    int count, flags, synch, i, number, warp;
    Tcl_QueuePosition pos;
    Pattern pat;
    unsigned long eventMask;
    static CONST char *fieldStrings[] = {
	"-when",	"-above",	"-borderwidth",	"-button",
	"-count",	"-delta",	"-detail",	"-focus",
	"-height",
	"-keycode",	"-keysym",	"-mode",	"-override",
	"-place",	"-root",	"-rootx",	"-rooty",
	"-sendevent",	"-serial",	"-state",	"-subwindow",
	"-time",	"-warp",	"-width",	"-window",
	"-x",		"-y",	NULL
    };
    enum field {
	EVENT_WHEN,	EVENT_ABOVE,	EVENT_BORDER,	EVENT_BUTTON,
	EVENT_COUNT,	EVENT_DELTA,	EVENT_DETAIL,	EVENT_FOCUS,
	EVENT_HEIGHT,
	EVENT_KEYCODE,	EVENT_KEYSYM,	EVENT_MODE,	EVENT_OVERRIDE,
	EVENT_PLACE,	EVENT_ROOT,	EVENT_ROOTX,	EVENT_ROOTY,
	EVENT_SEND,	EVENT_SERIAL,	EVENT_STATE,	EVENT_SUBWINDOW,
	EVENT_TIME,	EVENT_WARP,	EVENT_WIDTH,	EVENT_WINDOW,
	EVENT_X,	EVENT_Y
    };
    tkwin = Tk_MainWindow(interp);
    if (Blt_GetWindowFromObj(interp, objv[1], &window) != TCL_OK) {
	return TCL_ERROR;
    }
    name = Tcl_GetStringFromObj(objv[2], NULL);

    display = Tk_Display(tkwin);
    p = name;
    eventMask = 0;
    count = ParseEventDescription(interp, &p, &pat, &eventMask);
    if (count == 0) {
	return TCL_ERROR;
    }
    if (count != 1) {
	Tcl_AppendResult(interp, "Double or Triple modifier not allowed",
			 (char *)NULL);
	return TCL_ERROR;
    }
    if (*p != '\0') {
	Tcl_AppendResult(interp, "only one event specification allowed",
			 (char *)NULL);
	return TCL_ERROR;
    }

    memset((VOID *) &event, 0, sizeof(event));
    event.xany.type = pat.eventType;
    event.xany.serial = NextRequest(display);
    event.xany.send_event = False;
    event.xany.window = window;
    event.xany.display = display;

    flags = flagArray[event.xany.type];
    if (flags & DESTROY) {
	/*
	 * Event DestroyNotify should be generated by destroying 
	 * the window.
	 */
	XDestroyWindow(display, window);
	return TCL_OK;
    }
    if (flags & (KEY_BUTTON_MOTION_VIRTUAL)) {
	event.xkey.state = pat.needMods;
	if ((flags & KEY) && (event.xany.type != MouseWheelEvent)) {
	    TkpSetKeycodeAndState(tkwin, pat.detail.keySym, &event);
	} else if (flags & BUTTON) {
	    event.xbutton.button = pat.detail.button;
	} else if (flags & VIRTUAL) {
	    ((XVirtualEvent *) &event)->name = pat.detail.name;
	}
    }
    if (flags & (CREATE|UNMAP|MAP|REPARENT|CONFIG|GRAVITY|CIRC)) {
	event.xcreatewindow.window = event.xany.window;
    }

    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
	event.xkey.x_root = -1;
	event.xkey.y_root = -1;
    }

    /*
     * Process the remaining arguments to fill in additional fields
     * of the event.
     */

    synch = 1;
    warp = 0;
    pos = TCL_QUEUE_TAIL;
    for (i = 3; i < objc; i += 2) {
	Tcl_Obj *optionPtr, *valuePtr;
	int index;
	
	optionPtr = objv[i];
	valuePtr = objv[i + 1];

	if (Tcl_GetIndexFromObj(interp, optionPtr, fieldStrings, "option",
		TCL_EXACT, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((objc & 1) == 0) {
	    /*
	     * This test occurs after Tcl_GetIndexFromObj() so that
	     * "event generate <Button> -xyz" will return the error message
	     * that "-xyz" is a bad option, rather than that the value
	     * for "-xyz" is missing.
	     */

	    Tcl_AppendResult(interp, "value for \"",
		    Tcl_GetStringFromObj(optionPtr, NULL), "\" missing",
		    (char *)NULL);
	    return TCL_ERROR;
	}

	switch ((enum field) index) {
	case EVENT_WARP: {
	    if (Tcl_GetBooleanFromObj(interp, valuePtr, &warp) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (!(flags & (KEY_BUTTON_MOTION_VIRTUAL))) {
		goto badopt;
	    }
	    break;
	}
	case EVENT_WHEN: {
#ifdef notdef
	    pos = (Tcl_QueuePosition) TkFindStateNumObj(interp, optionPtr, 
		queuePosition, valuePtr);
	    if ((int) pos < -1) {
		return TCL_ERROR;
	    }
	    synch = 0;
	    if ((int) pos == -1) {
		synch = 1;
	    }
#endif
	    break;
	}
	case EVENT_ABOVE: {
	    Window window2;

	    if (Blt_GetWindowFromObj(interp, valuePtr, &window2) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & CONFIG) {
		event.xconfigure.above = window2;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_BORDER: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number)!=TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (CREATE|CONFIG)) {
		event.xcreatewindow.border_width = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_BUTTON: {
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & BUTTON) {
		event.xbutton.button = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_COUNT: {
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & EXPOSE) {
		event.xexpose.count = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_DELTA: {
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((flags & KEY) && (event.xkey.type == MouseWheelEvent)) {
		event.xkey.keycode = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_DETAIL: {
	    number = TkFindStateNumObj(interp, optionPtr, notifyDetail, 
				       valuePtr);
	    if (number < 0) {
		return TCL_ERROR;
	    }
	    if (flags & FOCUS) {
		event.xfocus.detail = number;
	    } else if (flags & CROSSING) {
		event.xcrossing.detail = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_FOCUS: {
	    if (Tcl_GetBooleanFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & CROSSING) {
		event.xcrossing.focus = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_HEIGHT: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & EXPOSE) {
		event.xexpose.height = number;
	    } else if (flags & CONFIG) {
		event.xconfigure.height = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_KEYCODE: {
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if ((flags & KEY) && (event.xkey.type != MouseWheelEvent)) {
		event.xkey.keycode = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_KEYSYM: {
	    KeySym keysym;
	    char *value;
	    
	    value = Tcl_GetStringFromObj(valuePtr, NULL);
	    keysym = TkStringToKeysym(value);
	    if (keysym == NoSymbol) {
		Tcl_AppendResult(interp, "unknown keysym \"", value, "\"",
				 (char *)NULL);
		return TCL_ERROR;
	    }
	    
	    TkpSetKeycodeAndState(tkwin, keysym, &event);
	    if (event.xkey.keycode == 0) {
		Tcl_AppendResult(interp, "no keycode for keysym \"", value,
				 "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    if (!(flags & KEY) || (event.xkey.type == MouseWheelEvent)) {
		goto badopt;
	    }
	    break;
	}
	case EVENT_MODE: {
	    number = TkFindStateNumObj(interp, optionPtr, notifyMode,
				       valuePtr);
	    if (number < 0) {
		return TCL_ERROR;
	    }
	    if (flags & CROSSING) {
		event.xcrossing.mode = number;
	    } else if (flags & FOCUS) {
		event.xfocus.mode = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_OVERRIDE: {
	    if (Tcl_GetBooleanFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & CREATE) {
		event.xcreatewindow.override_redirect = number;
	    } else if (flags & MAP) {
		event.xmap.override_redirect = number;
	    } else if (flags & REPARENT) {
		event.xreparent.override_redirect = number;
	    } else if (flags & CONFIG) {
		event.xconfigure.override_redirect = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_PLACE: {
	    number = TkFindStateNumObj(interp, optionPtr, circPlace,
				       valuePtr);
	    if (number < 0) {
		return TCL_ERROR;
	    }
	    if (flags & CIRC) {
		event.xcirculate.place = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_ROOT: {
	    Window window2;

	    if (Blt_GetWindowFromObj(interp, valuePtr, &window2) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		event.xkey.root = window2;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_ROOTX: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		event.xkey.x_root = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_ROOTY: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		event.xkey.y_root = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_SEND: {
	    CONST char *value;
	    
	    value = Tcl_GetStringFromObj(valuePtr, NULL);
	    if (isdigit(UCHAR(value[0]))) {
		/*
		 * Allow arbitrary integer values for the field; they
		 * are needed by a few of the tests in the Tk test suite.
		 */
		
		if (Tcl_GetIntFromObj(interp, valuePtr, &number)
		    != TCL_OK) {
		    return TCL_ERROR;
		}
	    } else {
		if (Tcl_GetBooleanFromObj(interp, valuePtr, &number)
		    != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    event.xany.send_event = number;
	    break;
	}
	case EVENT_SERIAL: {
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    event.xany.serial = number;
	    break;
	}
	case EVENT_STATE: {
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		if (Tcl_GetIntFromObj(interp, valuePtr, &number)
		    != TCL_OK) {
		    return TCL_ERROR;
		}
		if (flags & (KEY_BUTTON_MOTION_VIRTUAL)) {
		    event.xkey.state = number;
		} else {
		    event.xcrossing.state = number;
		}
	    } else if (flags & VISIBILITY) {
		number = TkFindStateNumObj(interp, optionPtr, visNotify,
					   valuePtr);
		if (number < 0) {
		    return TCL_ERROR;
		}
		event.xvisibility.state = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_SUBWINDOW: {
	    Window window2;

	    if (Blt_GetWindowFromObj(interp, valuePtr, &window2) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		event.xkey.subwindow = window2;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_TIME: {
	    if (Tcl_GetIntFromObj(interp, valuePtr, &number) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		event.xkey.time = (Time) number;
	    } else if (flags & PROP) {
		event.xproperty.time = (Time) number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_WIDTH: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & EXPOSE) {
		event.xexpose.width = number;
	    } else if (flags & (CREATE|CONFIG)) {
		event.xcreatewindow.width = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_WINDOW: {
	    Window window2;

	    if (Blt_GetWindowFromObj(interp, valuePtr, &window2) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (CREATE|UNMAP|MAP|REPARENT|CONFIG
			 |GRAVITY|CIRC)) {
		event.xcreatewindow.window = window2;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_X: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number)
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {	
		event.xkey.x = number;
		/*
		 * Only modify rootx as well if it hasn't been changed.
		 */
		if (event.xkey.x_root == -1) {
		    int rootX, rootY;
		    
		    Tk_GetRootCoords(tkwin, &rootX, &rootY);
		    event.xkey.x_root = rootX + number;
		}
	    } else if (flags & EXPOSE) {
		event.xexpose.x = number;
	    } else if (flags & (CREATE|CONFIG|GRAVITY)) { 
		event.xcreatewindow.x = number;
	    } else if (flags & REPARENT) {		
		event.xreparent.x = number;
	    } else {
		goto badopt;
	    }
	    break;
	}
	case EVENT_Y: {
	    if (Tk_GetPixelsFromObj(interp, tkwin, valuePtr, &number)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
		if (flags & (KEY_BUTTON_MOTION_VIRTUAL|CROSSING)) {
		    event.xkey.y = number;
		    /*
		     * Only modify rooty as well if it hasn't been changed.
		     */
		    if (event.xkey.y_root == -1) {
			int rootX, rootY;

			Tk_GetRootCoords(tkwin, &rootX, &rootY);
			event.xkey.y_root = rootY + number;
		    }
		} else if (flags & EXPOSE) {
		    event.xexpose.y = number;
		} else if (flags & (CREATE|CONFIG|GRAVITY)) {
		    event.xcreatewindow.y = number;
		} else if (flags & REPARENT) {
		    event.xreparent.y = number;
		} else {
		    goto badopt;
		}
		break;
	    }
	}
	continue;
	
	badopt:
	Tcl_AppendResult(interp, name, " event doesn't accept \"",
		Tcl_GetStringFromObj(optionPtr, NULL), "\" option", 
		(char *)NULL);
	return TCL_ERROR;
    }
    if (!XSendEvent(display, window, False, pat.eventType, &event)) {
	fprintf(stderr, "synthethic event failed\n");
    }
    return TCL_OK;
}

int
Blt_SendEventCmdInitProc(Tcl_Interp *interp)
{
    static Blt_InitCmdSpec cmdSpec = { 
	"sendevent", SendEventCmd, 
    };
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
