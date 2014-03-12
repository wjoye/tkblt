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

#define DEF_TEXT_FLAGS (TK_PARTIAL_OK | TK_IGNORE_NEWLINES)
#define UPDATE_GC	1

/*
 * TextStyle --
 *
 * 	A somewhat convenient structure to hold text attributes that determine
 * 	how a text string is to be displayed on the screen or drawn with
 * 	PostScript commands.  The alternative is to pass lots of parameters to
 * 	the drawing and printing routines. This seems like a more efficient
 * 	and less cumbersome way of passing parameters.
 */
typedef struct {
    unsigned int state;			/* If non-zero, indicates to draw text
					 * in the active color */
    XColor* color;			/* Color to draw the text. */
    Tk_Font font;			/* Font to use to draw text */
    double angle;			/* Rotation of text in degrees. */
    Tk_Justify justify;			/* Justification of the text
					 * string. This only matters if the
					 * text is composed of multiple
					 * lines. */
    Tk_Anchor anchor;			/* Indicates how the text is anchored
					 * around its x,y coordinates. */
    int xPad, yPad;			/* # pixels padding of around text
					 * region. */
    unsigned short int leader;		/* # pixels spacing between lines of
					 * text. */
    short int underline;		/* Index of character to be underlined,
					 * -1 if no underline. */
    int maxLength;			/* Maximum length in pixels of text */
    /* Private fields. */
    unsigned short flags;
    GC gc;			/* GC used to draw the text */
} TextStyle;

extern void Blt_GetTextExtents(Tk_Font font, int leader, const char *text, 
	int textLen, unsigned int *widthPtr, unsigned int *heightPtr);

extern void Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text, 
	unsigned int *widthPtr, unsigned int *heightPtr);

extern void Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *tsPtr);

extern void Blt_Ts_FreeStyle(Display *display, TextStyle *tsPtr);

extern void Blt_DrawText(Tk_Window tkwin, Drawable drawable, 
	const char *string, TextStyle *tsPtr, int x, int y);

extern void Blt_DrawText2(Tk_Window tkwin, Drawable drawable, 
	const char *string, TextStyle *tsPtr, int x, int y, Dim2D * dimPtr);

extern void Blt_Ts_DrawText(Tk_Window tkwin, Drawable drawable, 
	const char *text, int textLen, TextStyle *tsPtr, int x, int y);

#define Blt_Ts_GetAnchor(ts)		((ts).anchor)
#define Blt_Ts_GetAngle(ts)		((ts).angle)
#define Blt_Ts_GetBackground(ts)	((ts).bg)
#define Blt_Ts_GetFont(ts)		((ts).font)
#define Blt_Ts_GetForeground(ts)	((ts).color)
#define Blt_Ts_GetJustify(ts)		((ts).justify)
#define Blt_Ts_GetLeader(ts)		((ts).leader)

#define Blt_Ts_SetAnchor(ts, a)	((ts).anchor = (a))
#define Blt_Ts_SetAngle(ts, r)		((ts).angle = (double)(r))
#define Blt_Ts_SetBackground(ts, b)	((ts).bg = (b))
#define Blt_Ts_SetFont(ts, f)		\
	(((ts).font != (f)) ? ((ts).font = (f), (ts).flags |= UPDATE_GC) : 0)
#define Blt_Ts_SetForeground(ts, c)    \
	(((ts).color != (c)) ? ((ts).color = (c), (ts).flags |= UPDATE_GC) : 0)
#define Blt_Ts_SetJustify(ts, j)	((ts).justify = (j))
#define Blt_Ts_SetLeader(ts, l)	((ts).leader = (l))
#define Blt_Ts_SetMaxLength(ts, l)	((ts).maxLength = (l))
#define Blt_Ts_SetPadding(ts, x, y)     ((ts).xPad = (x), (ts).yPad = (y))
#define Blt_Ts_SetState(ts, s)		((ts).state = (s))
#define Blt_Ts_SetUnderline(ts, ul)	((ts).underline = (ul))

#define Blt_Ts_InitStyle(ts)		\
    ((ts).anchor = TK_ANCHOR_NW,	\
     (ts).color = (XColor*)NULL,	\
     (ts).font = NULL,			\
     (ts).justify = TK_JUSTIFY_LEFT,	\
     (ts).leader = 0,			\
     (ts).underline = -1,		       \
     (ts).xPad = 0,    \
     (ts).yPad = 0,    \
     (ts).state = 0,			       \
     (ts).flags = 0,			       \
     (ts).gc = NULL,			       \
     (ts).maxLength = -1,		       \
     (ts).angle = 0.0)

#endif /* _BLT_TEXT_H */
