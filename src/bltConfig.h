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

#ifndef BLT_CONFIG_H
#define BLT_CONFIG_H

#define BLT_STATE_NORMAL		(0)
#define BLT_STATE_ACTIVE		(1<<0)
#define BLT_STATE_DISABLED		(1<<1)

extern const char* stateObjOption[];

typedef struct {
    unsigned char values[12];
    int offset;
} Blt_Dashes;

#define LineIsDashed(d) ((d).values[0] != 0)

extern Tk_ObjCustomOption pointObjOption;
extern Tk_ObjCustomOption dashesObjOption;
extern Tk_ObjCustomOption listObjOption;

extern void Blt_SetDashes (Display *display, GC gc, Blt_Dashes *dashesPtr);

#endif /* BLT_CONFIG_H */
