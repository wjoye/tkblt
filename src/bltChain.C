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

Chain::Chain()
{
  head_ =NULL;
  tail_ =NULL;
  nLinks_ =0;
}

Chain::~Chain()
{
  ChainLink* linkPtr = head_;
  while (linkPtr) {
    ChainLink* oldPtr =linkPtr;
    linkPtr = linkPtr->next;
    free(oldPtr);
  }
}

void Chain::reset()
{
  ChainLink* linkPtr = head_;
  while (linkPtr) {
    ChainLink* oldPtr = linkPtr;
    linkPtr = linkPtr->next;
    free(oldPtr);
  }
  head_ =NULL;
  tail_ =NULL;
  nLinks_ =0;
}

void Chain::linkAfter(ChainLink* linkPtr, ChainLink* afterPtr)
{
  if (!head_) {
    head_ = linkPtr;
    tail_ = linkPtr;
  }
  else {
    if (!afterPtr) {
      linkPtr->next = NULL;
      linkPtr->prev = tail_;
      tail_->next = linkPtr;
      tail_ = linkPtr;
    } 
    else {
      linkPtr->next = afterPtr->next;
      linkPtr->prev = afterPtr;
      if (afterPtr == tail_)
	tail_ = linkPtr;
      else
	afterPtr->next->prev = linkPtr;
      afterPtr->next = linkPtr;
    }
  }

  nLinks_++;
}

void Chain::linkBefore(ChainLink* linkPtr, ChainLink* beforePtr)
{
  if (!head_) {
    head_ = linkPtr;
    tail_ = linkPtr;
  }
  else {
    if (beforePtr == NULL) {
      linkPtr->next = head_;
      linkPtr->prev = NULL;
      head_->prev = linkPtr;
      head_ = linkPtr;
    }
    else {
      linkPtr->prev = beforePtr->prev;
      linkPtr->next = beforePtr;
      if (beforePtr == head_)
	head_ = linkPtr;
      else
	beforePtr->prev->next = linkPtr;
      beforePtr->prev = linkPtr;
    }
  }

  nLinks_++;
}

void Chain::unlinkLink(ChainLink* linkPtr)
{
  // Indicates if the link is actually remove from the chain
  int unlinked;

  unlinked = 0;
  if (head_ == linkPtr) {
    head_ = linkPtr->next;
    unlinked = 1;
  }
  if (tail_ == linkPtr) {
    tail_ = linkPtr->prev;
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
  if (unlinked)
    nLinks_--;

  linkPtr->prev =NULL;
  linkPtr->next =NULL;
}

void Chain::deleteLink(ChainLink* link)
{
  unlinkLink(link);
  free(link);
  link = NULL;
}

ChainLink* Chain::append(void* clientData)
{
  ChainLink* link = Chain_NewLink();
  linkAfter(link, NULL);
  Chain_SetValue(link, clientData);
  return link;
}

ChainLink* Chain::prepend(void* clientData)
{
  ChainLink* link = Chain_NewLink();
  linkBefore(link, NULL);
  Chain_SetValue(link, clientData);
  return link;
}

ChainLink* Blt::Chain_AllocLink(size_t extraSize)
{
  size_t linkSize = ALIGN(sizeof(ChainLink));
  ChainLink* linkPtr = (ChainLink*)calloc(1, linkSize + extraSize);
  if (extraSize > 0) {
    // Point clientData at the memory beyond the normal structure
    linkPtr->clientData = ((char *)linkPtr + linkSize);
  }

  return linkPtr;
}

void Blt::Chain_InitLink(ChainLink* linkPtr)
{
  linkPtr->clientData = NULL;
  linkPtr->next = linkPtr->prev = NULL;
}

ChainLink* Blt::Chain_NewLink(void)
{
  ChainLink* linkPtr = (ChainLink*)malloc(sizeof(ChainLink));
  linkPtr->clientData = NULL;
  linkPtr->next = linkPtr->prev = NULL;
  return linkPtr;
}

int Blt::Chain_IsBefore(ChainLink* firstPtr, ChainLink* lastPtr)
{
  for (ChainLink* linkPtr = firstPtr; linkPtr; linkPtr = linkPtr->next)
    if (linkPtr == lastPtr)
      return 1;

  return 0;
}

