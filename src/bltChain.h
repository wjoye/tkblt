/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
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
#ifndef _BLT_CHAIN_H
#define _BLT_CHAIN_H

#include <tcl.h>

#define Chain_GetLength(c)	(((c) == NULL) ? 0 : (c)->nLinks)
#define Chain_FirstLink(c)	(((c) == NULL) ? NULL : (c)->head)
#define Chain_LastLink(c)	(((c) == NULL) ? NULL : (c)->tail)
#define Chain_PrevLink(l)	((l)->prev)
#define Chain_NextLink(l) 	((l)->next)
#define Chain_GetValue(l)  	((l)->clientData)
#define Chain_FirstValue(c)	(((c)->head == NULL) ? NULL : (c)->head->clientData)
#define Chain_SetValue(l, value) ((l)->clientData = (ClientData)(value))
#define Chain_AppendLink(c, l) (Chain_LinkAfter((c), (l), (Blt_ChainLink)NULL))
#define Chain_PrependLink(c, l) (Chain_LinkBefore((c), (l), (Blt_ChainLink)NULL))

namespace Blt {

  typedef struct _Blt_Chain *Blt_Chain;
  typedef struct _Blt_ChainLink *Blt_ChainLink;

  struct _Blt_ChainLink {
    Blt_ChainLink prev;		/* Link to the previous link */
    Blt_ChainLink next;		/* Link to the next link */
    ClientData clientData;	/* Pointer to the data object */
  };

  typedef int (Blt_ChainCompareProc)(Blt_ChainLink *l1Ptr, Blt_ChainLink *l2Ptr);

  struct _Blt_Chain {
    Blt_ChainLink head;		/* Pointer to first element in chain */
    Blt_ChainLink tail;		/* Pointer to last element in chain */
    long nLinks;		/* Number of elements in chain */
  };

  extern Blt_Chain Chain_Create(void);
  extern Blt_ChainLink Chain_AllocLink(size_t size);
  extern void Chain_InitLink(Blt_ChainLink link);
  extern void Chain_Init(Blt_Chain chain);
  extern Blt_ChainLink Chain_NewLink(void);
  extern void Chain_Reset(Blt_Chain chain);
  extern void Chain_Destroy(Blt_Chain chain);
  extern void Chain_LinkAfter(Blt_Chain chain, Blt_ChainLink link, Blt_ChainLink after);
  extern void Chain_LinkBefore(Blt_Chain chain, Blt_ChainLink link, Blt_ChainLink before);
  extern void Chain_UnlinkLink(Blt_Chain chain, Blt_ChainLink link);
  extern void Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link);
  extern Blt_ChainLink Chain_Append(Blt_Chain chain, ClientData clientData);
  extern Blt_ChainLink Chain_Prepend(Blt_Chain chain, ClientData clientData);
  extern int Chain_IsBefore(Blt_ChainLink first, Blt_ChainLink last);
};
#endif /* _BLT_CHAIN_H */
