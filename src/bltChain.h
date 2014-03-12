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

typedef struct _Blt_Chain *Blt_Chain;
typedef struct _Blt_ChainLink *Blt_ChainLink;

/*
 * A Blt_ChainLink is the container structure for the Blt_Chain.
 */

struct _Blt_ChainLink {
    Blt_ChainLink prev;		/* Link to the previous link */
    Blt_ChainLink next;		/* Link to the next link */
    ClientData clientData;	/* Pointer to the data object */
};

typedef int (Blt_ChainCompareProc)(Blt_ChainLink *l1Ptr, Blt_ChainLink *l2Ptr);

/*
 * A Blt_Chain is a doubly chained list structure.
 */
struct _Blt_Chain {
    Blt_ChainLink head;		/* Pointer to first element in chain */
    Blt_ChainLink tail;		/* Pointer to last element in chain */
    long nLinks;		/* Number of elements in chain */
};

extern void Blt_Chain_Init(Blt_Chain chain);
extern Blt_Chain Blt_Chain_Create(void);
extern void Blt_Chain_Destroy(Blt_Chain chain);
extern Blt_ChainLink Blt_Chain_NewLink(void);
extern Blt_ChainLink Blt_Chain_AllocLink(size_t size);
extern Blt_ChainLink Blt_Chain_Append(Blt_Chain chain, 
	ClientData clientData);
extern Blt_ChainLink Blt_Chain_Prepend(Blt_Chain chain, 
	ClientData clientData);
extern void Blt_Chain_Reset(Blt_Chain chain);
extern void Blt_Chain_InitLink(Blt_ChainLink link);
extern void Blt_Chain_LinkAfter(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink after);
extern void Blt_Chain_LinkBefore(Blt_Chain chain, Blt_ChainLink link, 
	Blt_ChainLink before);
extern void Blt_Chain_UnlinkLink(Blt_Chain chain, Blt_ChainLink link);
extern void Blt_Chain_DeleteLink(Blt_Chain chain, Blt_ChainLink link);
extern int Blt_Chain_IsBefore(Blt_ChainLink first, Blt_ChainLink last);

#define Blt_Chain_GetLength(c)	(((c) == NULL) ? 0 : (c)->nLinks)
#define Blt_Chain_FirstLink(c)	(((c) == NULL) ? NULL : (c)->head)
#define Blt_Chain_LastLink(c)	(((c) == NULL) ? NULL : (c)->tail)
#define Blt_Chain_PrevLink(l)	((l)->prev)
#define Blt_Chain_NextLink(l) 	((l)->next)
#define Blt_Chain_GetValue(l)  	((l)->clientData)
#define Blt_Chain_FirstValue(c)	(((c)->head == NULL) ? NULL : (c)->head->clientData)
#define Blt_Chain_SetValue(l, value) ((l)->clientData = (ClientData)(value))
#define Blt_Chain_AppendLink(c, l) \
	(Blt_Chain_LinkAfter((c), (l), (Blt_ChainLink)NULL))
#define Blt_Chain_PrependLink(c, l) \
	(Blt_Chain_LinkBefore((c), (l), (Blt_ChainLink)NULL))

#endif /* _BLT_CHAIN_H */
