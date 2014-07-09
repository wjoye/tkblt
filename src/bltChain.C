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

#include "bltChain.h"

using namespace Blt;

#ifndef ALIGN
#define ALIGN(a)							\
  (((size_t)a + (sizeof(double) - 1)) & (~(sizeof(double) - 1)))
#endif

Chain Blt::Chain_Create(void)
{
  Chain chainPtr =(Chain)malloc(sizeof(_Chain));
  if (chainPtr)
    Chain_Init(chainPtr);
  return chainPtr;
}

ChainLink Blt::Chain_AllocLink(size_t extraSize)
{
  size_t linkSize = ALIGN(sizeof(_ChainLink));
  ChainLink linkPtr = (ChainLink)calloc(1, linkSize + extraSize);
  if (extraSize > 0) {
    // Point clientData at the memory beyond the normal structure
    linkPtr->clientData = (ClientData)((char *)linkPtr + linkSize);
  }
  return linkPtr;
}

void Blt::Chain_InitLink(ChainLink linkPtr)
{
  linkPtr->clientData = NULL;
  linkPtr->next = linkPtr->prev = NULL;
}

ChainLink Blt::Chain_NewLink(void)
{
  ChainLink linkPtr = (ChainLink)malloc(sizeof(_ChainLink));
  linkPtr->clientData = NULL;
  linkPtr->next = linkPtr->prev = NULL;
  return linkPtr;
}

void Blt::Chain_Reset(Chain chainPtr)
{
  if (chainPtr) {
    ChainLink oldPtr;
    ChainLink linkPtr = chainPtr->head;

    while (linkPtr) {
      oldPtr = linkPtr;
      linkPtr = linkPtr->next;
      free(oldPtr);
    }
    Chain_Init(chainPtr);
  }
}

void Blt::Chain_Destroy(Chain chainPtr)
{
  if (chainPtr) {
    Chain_Reset(chainPtr);
    free(chainPtr);
    chainPtr = NULL;
  }
}

void Blt::Chain_Init(Chain chainPtr)
{
  chainPtr->nLinks = 0;
  chainPtr->head = chainPtr->tail = NULL;
}

void Blt::Chain_LinkAfter(Chain chainPtr, ChainLink linkPtr, ChainLink afterPtr)
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

void Blt::Chain_LinkBefore(Chain chainPtr, ChainLink linkPtr, 
			   ChainLink beforePtr)
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

void Blt::Chain_UnlinkLink(Chain chainPtr, ChainLink linkPtr)
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

void Blt::Chain_DeleteLink(Chain chain, ChainLink link)
{
  Chain_UnlinkLink(chain, link);
  free(link);
  link = NULL;
}

ChainLink Blt::Chain_Append(Chain chain, ClientData clientData)
{
  ChainLink link = Chain_NewLink();
  Chain_LinkAfter(chain, link, (ChainLink)NULL);
  Chain_SetValue(link, clientData);
  return link;
}

ChainLink Blt::Chain_Prepend(Chain chain, ClientData clientData)
{
  ChainLink link = Chain_NewLink();
  Chain_LinkBefore(chain, link, (ChainLink)NULL);
  Chain_SetValue(link, clientData);
  return link;
}

int Blt::Chain_IsBefore(ChainLink firstPtr, ChainLink lastPtr)
{
  for (ChainLink linkPtr = firstPtr; linkPtr; linkPtr = linkPtr->next) {
    if (linkPtr == lastPtr)
      return 1;
  }
  return 0;
}

