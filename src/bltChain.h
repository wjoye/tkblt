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

#define Chain_GetLength(c) (((c) == NULL) ? 0 : (c)->nLinks)
#define Chain_FirstLink(c) (((c) == NULL) ? NULL : (c)->head)
#define Chain_LastLink(c) (((c) == NULL) ? NULL : (c)->tail)
#define Chain_PrevLink(l) ((l)->prev)
#define Chain_NextLink(l) ((l)->next)
#define Chain_GetValue(l) ((l)->clientData)
#define Chain_FirstValue(c) (((c)->head == NULL) ? NULL : (c)->head->clientData)
#define Chain_SetValue(l, v) ((l)->clientData = (void*)(v))
#define Chain_AppendLink(c, l) (Chain_LinkAfter((c), (l), (ChainLink*)NULL))
#define Chain_PrependLink(c, l) (Chain_LinkBefore((c), (l), (ChainLink*)NULL))

namespace Blt {

  struct ChainLink {
    ChainLink* prev;
    ChainLink* next;
    void* clientData;
  };

  struct Chain {
    ChainLink* head;
    ChainLink* tail;
    long nLinks;
  };

  extern Chain* Chain_Create(void);
  extern ChainLink* Chain_AllocLink(size_t size);
  extern void Chain_InitLink(ChainLink* link);
  extern void Chain_Init(Chain* chain);
  extern ChainLink* Chain_NewLink(void);
  extern void Chain_Reset(Chain* chain);
  extern void Chain_Destroy(Chain* chain);
  extern void Chain_LinkAfter(Chain* chain, ChainLink* link, ChainLink* after);
  extern void Chain_LinkBefore(Chain* chain, ChainLink* link,ChainLink* before);
  extern void Chain_UnlinkLink(Chain* chain, ChainLink* link);
  extern void Chain_DeleteLink(Chain* chain, ChainLink* link);
  extern ChainLink* Chain_Append(Chain* chain, void* clientData);
  extern ChainLink* Chain_Prepend(Chain* chain, void* clientData);
  extern int Chain_IsBefore(ChainLink* first, ChainLink* last);
};

#endif
