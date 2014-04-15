/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1993-2004 George A Howlett.
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

#ifndef _BLT_TEXT_H
#define _BLT_TEXT_H

#define UPDATE_GC 1

typedef struct {
  unsigned int state;
  XColor* color;
  Tk_Font font;
  double angle;
  Tk_Justify justify;
  Tk_Anchor anchor;
  int xPad;
  int yPad;
  unsigned short int leader;
  short int underline;
  int maxLength;

  /* Private fields. */
  unsigned short flags;
  GC gc;
} TextStyle;

extern void Blt_GetTextExtents(Tk_Font font, int leader, const char *text, 
			       int textLen, unsigned int *widthPtr, 
			       unsigned int *heightPtr);
extern void Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text, 
			      unsigned int *widthPtr, unsigned int *heightPtr);
extern void Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *tsPtr);
extern void Blt_Ts_FreeStyle(Display *display, TextStyle *tsPtr);
extern void Blt_DrawText(Tk_Window tkwin, Drawable drawable, 
			 const char *string, TextStyle *tsPtr, 
			 int x, int y);
extern void Blt_DrawText2(Tk_Window tkwin, Drawable drawable, 
			  const char *string, TextStyle *tsPtr, 
			  int x, int y, Dim2D * dimPtr);
extern void Blt_Ts_DrawText(Tk_Window tkwin, Drawable drawable, 
			    const char *text, int textLen, TextStyle *tsPtr,
			    int x, int y);
extern void Blt_GetBoundingBox (int width, int height, float angle, 
	double *widthPtr, double *heightPtr, Point2d *points);

#define Blt_Ts_InitStyle(ts)			\
  ((ts).anchor = TK_ANCHOR_NW,			\
   (ts).color = (XColor*)NULL,			\
   (ts).font = NULL,				\
   (ts).justify = TK_JUSTIFY_LEFT,		\
   (ts).leader = 0,				\
   (ts).underline = -1,				\
   (ts).xPad = 0,				\
   (ts).yPad = 0,				\
   (ts).state = 0,				\
   (ts).flags = 0,				\
   (ts).gc = NULL,				\
   (ts).maxLength = -1,				\
   (ts).angle = 0.0)

#endif
