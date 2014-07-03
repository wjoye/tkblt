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

#include <stdlib.h>

extern "C" {
#include "bltChain.h"
};

#ifndef ALIGN
#define ALIGN(a)							\
  (((size_t)a + (sizeof(double) - 1)) & (~(sizeof(double) - 1)))
#endif

typedef int (QSortCompareProc) (const void *, const void *);

typedef struct _Blt_ChainLink ChainLink;
typedef struct _Blt_Chain Chain;

Blt_Chain Blt_Chain_Create(void)
{
  Chain* chainPtr =(Chain*)malloc(sizeof(Chain));
  if (chainPtr) {
    Blt_Chain_Init(chainPtr);
  }
  return chainPtr;
}

Blt_ChainLink Blt_Chain_AllocLink(size_t extraSize)
{
  size_t linkSize = ALIGN(sizeof(ChainLink));
  ChainLink* linkPtr = (ChainLink*)calloc(1, linkSize + extraSize);
  if (extraSize > 0) {
    // Point clientData at the memory beyond the normal structure
    linkPtr->clientData = (ClientData)((char *)linkPtr + linkSize);
  }
  return linkPtr;
}

void Blt_Chain_InitLink(ChainLink* linkPtr)
{
  linkPtr->clientData = NULL;
  linkPtr->next = linkPtr->prev = NULL;
}

Blt_ChainLink Blt_Chain_NewLink(void)
{
  ChainLink* linkPtr = (ChainLink*)malloc(sizeof(ChainLink));
  linkPtr->clientData = NULL;
  linkPtr->next = linkPtr->prev = NULL;
  return linkPtr;
}

void Blt_Chain_Reset(Chain* chainPtr)
{
  if (chainPtr) {
    ChainLink* oldPtr;
    ChainLink* linkPtr = chainPtr->head;

    while (linkPtr) {
      oldPtr = linkPtr;
      linkPtr = linkPtr->next;
      free(oldPtr);
    }
    Blt_Chain_Init(chainPtr);
  }
}

void Blt_Chain_Destroy(Chain* chainPtr)
{
  if (chainPtr) {
    Blt_Chain_Reset(chainPtr);
    free(chainPtr);
    chainPtr = NULL;
  }
}

void Blt_Chain_Init(Chain* chainPtr)
{
  chainPtr->nLinks = 0;
  chainPtr->head = chainPtr->tail = NULL;
}

void Blt_Chain_LinkAfter(Chain* chainPtr, ChainLink* linkPtr, 
			 ChainLink* afterPtr)
{
  if (chainPtr->head == NULL)
    chainPtr->tail = chainPtr->head = linkPtr;
  else {
    if (afterPtr == NULL) {
      // Append to the end of the chain
      linkPtr->next = NULL;
      linkPtr->prev = chainPtr->tail;
      chainPtr->tail->next = linkPtr;
      chainPtr->tail = linkPtr;
    } 
    else {
      linkPtr->next = afterPtr->next;
      linkPtr->prev = afterPtr;
      if (afterPtr == chainPtr->tail)
	chainPtr->tail = linkPtr;
      else
	afterPtr->next->prev = linkPtr;
      afterPtr->next = linkPtr;
    }
  }
  chainPtr->nLinks++;
}

void Blt_Chain_LinkBefore(Chain* chainPtr, ChainLink* linkPtr, 
			  ChainLink* beforePtr)
{
  if (chainPtr->head == NULL)
    chainPtr->tail = chainPtr->head = linkPtr;
  else {
    if (beforePtr == NULL) {
      // Prepend to the front of the chain
      linkPtr->next = chainPtr->head;
      linkPtr->prev = NULL;
      chainPtr->head->prev = linkPtr;
      chainPtr->head = linkPtr;
    }
    else {
      linkPtr->prev = beforePtr->prev;
      linkPtr->next = beforePtr;
      if (beforePtr == chainPtr->head)
	chainPtr->head = linkPtr;
      else
	beforePtr->prev->next = linkPtr;
      beforePtr->prev = linkPtr;
    }
  }
  chainPtr->nLinks++;
}

void Blt_Chain_UnlinkLink(Chain* chainPtr, ChainLink* linkPtr)
{
  // Indicates if the link is actually remove from the chain
  int unlinked;

  unlinked = 0;
  if (chainPtr->head == linkPtr) {
    chainPtr->head = linkPtr->next;
    unlinked = 1;
  }
  if (chainPtr->tail == linkPtr) {
    chainPtr->tail = linkPtr->prev;
    unlinked = 1;
  }
  if (linkPtr->next) {
    linkPtr->next->prev = linkPtr->prev;
    unlinked = 1;
  }
  if (linkPtr->prev) {
    linkPtr->prev->next = linkPtr->next;
    unlinked = 1;
  }
  if (unlinked) {
    chainPtr->nLinks--;
  }
  linkPtr->prev = linkPtr->next = NULL;
}

void Blt_Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link)
{
  Blt_Chain_UnlinkLink(chain, link);
  free(link);
  link = NULL;
}

Blt_ChainLink Blt_Chain_Append(Blt_Chain chain, ClientData clientData)
{
  Blt_ChainLink link;

  link = Blt_Chain_NewLink();
  Blt_Chain_LinkAfter(chain, link, (Blt_ChainLink)NULL);
  Blt_Chain_SetValue(link, clientData);
  return link;
}

Blt_ChainLink Blt_Chain_Prepend(Blt_Chain chain, ClientData clientData)
{
  Blt_ChainLink link;

  link = Blt_Chain_NewLink();
  Blt_Chain_LinkBefore(chain, link, (Blt_ChainLink)NULL);
  Blt_Chain_SetValue(link, clientData);
  return link;
}

void Blt_Chain_Sort(Chain *chainPtr, Blt_ChainCompareProc *proc)
{
    if (chainPtr->nLinks < 2)
	return;

    ChainLink** linkArr = 
      (ChainLink**)malloc(sizeof(Blt_ChainLink) * (chainPtr->nLinks + 1));
    if (!linkArr)
	return;

    long i = 0;
    ChainLink *linkPtr;
    for (linkPtr = chainPtr->head; linkPtr != NULL; 
	 linkPtr = linkPtr->next) { 
	linkArr[i++] = linkPtr;
    }
    qsort((char *)linkArr, chainPtr->nLinks, sizeof(Blt_ChainLink),
	(QSortCompareProc *)proc);

    /* Rethread the chain. */
    linkPtr = linkArr[0];
    chainPtr->head = linkPtr;
    linkPtr->prev = NULL;
    for (i = 1; i < chainPtr->nLinks; i++) {
	linkPtr->next = linkArr[i];
	linkPtr->next->prev = linkPtr;
	linkPtr = linkPtr->next;
    }
    chainPtr->tail = linkPtr;
    linkPtr->next = NULL;
    free(linkArr);
}

int Blt_Chain_IsBefore(ChainLink* firstPtr, ChainLink* lastPtr)
{
  for (ChainLink* linkPtr = firstPtr; linkPtr; linkPtr = linkPtr->next) {
    if (linkPtr == lastPtr)
      return 1;
  }
  return 0;
}

