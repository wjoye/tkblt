
/*
 * bltText.c --
 *
 * This module implements multi-line, rotate-able text for the BLT toolkit.
 *
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

#include "bltInt.h"
#include <bltHash.h>
#include <X11/Xutil.h>
#include "tkIntBorder.h"
#include "bltImage.h"
#include "bltBitmap.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltBgStyle.h"

#define WINDEBUG	0

static Blt_HashTable bitmapGCTable;
static int initialized;

GC
Blt_GetBitmapGC(Tk_Window tkwin)
{
    int isNew;
    GC gc;
    Display *display;
    Blt_HashEntry *hPtr;

    if (!initialized) {
	Blt_InitHashTable(&bitmapGCTable, BLT_ONE_WORD_KEYS);
	initialized = TRUE;
    }
    display = Tk_Display(tkwin);
    hPtr = Blt_CreateHashEntry(&bitmapGCTable, (char *)display, &isNew);
    if (isNew) {
	Pixmap bitmap;
	XGCValues gcValues;
	unsigned long gcMask;
	Window root;

	root = Tk_RootWindow(tkwin);
	bitmap = Tk_GetPixmap(display, root, 1, 1, 1);
	gcValues.foreground = gcValues.background = 0;
	gcMask = (GCForeground | GCBackground);
	gc = Blt_GetPrivateGCFromDrawable(display, bitmap, gcMask, &gcValues);
	Tk_FreePixmap(display, bitmap);
	Blt_SetHashValue(hPtr, gc);
    } else {
	gc = (GC)Blt_GetHashValue(hPtr);
    }
    return gc;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetTextExtents --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *	string.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_GetTextExtents(
    Blt_Font font, 
    int leader,
    const char *text,		/* Text string to be measured. */
    int textLen,		/* Length of the text. If -1, indicates that
				 * text is an ASCIZ string that the length
				 * should be computed with strlen. */
    unsigned int *widthPtr, 
    unsigned int *heightPtr)
{
    unsigned int lineHeight;

    if (text == NULL) {
	return;			/* NULL string? */
    }
    {
	Blt_FontMetrics fm;

	Blt_GetFontMetrics(font, &fm);
	lineHeight = fm.linespace;
    }
    if (textLen < 0) {
	textLen = strlen(text);
    }
    { 
	unsigned int lineLen;		/* # of characters on each line */
	const char *p, *pend;
	const char *line;
	unsigned int maxWidth, maxHeight;

	maxWidth = maxHeight = 0;
	lineLen = 0;
	for (p = line = text, pend = text + textLen; p < pend; p++) {
	    if (*p == '\n') {
		if (lineLen > 0) {
		    unsigned int lineWidth;
		    
		    lineWidth = Blt_TextWidth(font, line, lineLen);
		    if (lineWidth > maxWidth) {
			maxWidth = lineWidth;
		    }
		}
		maxHeight += lineHeight;
		line = p + 1;	/* Point to the start of the next line. */
		lineLen = 0;	/* Reset counter to indicate the start of a
				 * new line. */
		continue;
	    }
	    lineLen++;
	}
	if ((lineLen > 0) && (*(p - 1) != '\n')) {
	    unsigned int lineWidth;
	    
	    maxHeight += lineHeight;
	    lineWidth = Blt_TextWidth(font, line, lineLen);
	    if (lineWidth > maxWidth) {
		maxWidth = lineWidth;
	    }
	}
	*widthPtr = maxWidth;
	*heightPtr = maxHeight;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_GetExtents --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of
 *	the text string.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ts_GetExtents(TextStyle *tsPtr, const char *text, unsigned int *widthPtr, 
		  unsigned int *heightPtr)
{

    if (text == NULL) {
	*widthPtr = *heightPtr = 0;
    } else {
	unsigned int w, h;

	Blt_GetTextExtents(tsPtr->font, tsPtr->leader, text, -1, &w, &h);
	*widthPtr = w + PADDING(tsPtr->xPad);
	*heightPtr = h + PADDING(tsPtr->yPad);
    } 
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetBoundingBox
 *
 *	Computes the dimensions of the bounding box surrounding a rectangle
 *	rotated about its center.  If pointArr isn't NULL, the coordinates of
 *	the rotated rectangle are also returned.
 *
 *	The dimensions are determined by rotating the rectangle, and doubling
 *	the maximum x-coordinate and y-coordinate.
 *
 *		w = 2 * maxX,  h = 2 * maxY
 *
 *	Since the rectangle is centered at 0,0, the coordinates of the
 *	bounding box are (-w/2,-h/2 w/2,-h/2, w/2,h/2 -w/2,h/2).
 *
 *  		0 ------- 1
 *  		|         |
 *  		|    x    |
 *  		|         |
 *  		3 ------- 2
 *
 * Results:
 *	The width and height of the bounding box containing the rotated
 *	rectangle are returned.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_GetBoundingBox(
    int width, int height,	/* Unrotated region */
    float angle,		/* Rotation of box */
    double *rotWidthPtr, 
    double *rotHeightPtr,	/* (out) Bounding box region */
    Point2d *bbox)		/* (out) Points of the rotated box */
{
    int i;
    double sinTheta, cosTheta;
    double radians;
    double xMax, yMax;
    double x, y;
    Point2d corner[4];

    angle = fmod(angle, 360.0);
    if (fmod(angle, (double)90.0) == 0.0) {
	int ll, ur, ul, lr;
	double rotWidth, rotHeight;
	int quadrant;

	/* Handle right-angle rotations specially. */

	quadrant = (int)(angle / 90.0);
	switch (quadrant) {
	case ROTATE_270:	/* 270 degrees */
	    ul = 3, ur = 0, lr = 1, ll = 2;
	    rotWidth = (double)height;
	    rotHeight = (double)width;
	    break;
	case ROTATE_90:		/* 90 degrees */
	    ul = 1, ur = 2, lr = 3, ll = 0;
	    rotWidth = (double)height;
	    rotHeight = (double)width;
	    break;
	case ROTATE_180:	/* 180 degrees */
	    ul = 2, ur = 3, lr = 0, ll = 1;
	    rotWidth = (double)width;
	    rotHeight = (double)height;
	    break;
	default:
	case ROTATE_0:		/* 0 degrees */
	    ul = 0, ur = 1, lr = 2, ll = 3;
	    rotWidth = (double)width;
	    rotHeight = (double)height;
	    break;
	}
	if (bbox != NULL) {
	    x = rotWidth * 0.5;
	    y = rotHeight * 0.5;
	    bbox[ll].x = bbox[ul].x = -x;
	    bbox[ur].y = bbox[ul].y = -y;
	    bbox[lr].x = bbox[ur].x = x;
	    bbox[ll].y = bbox[lr].y = y;
	}
	*rotWidthPtr = rotWidth;
	*rotHeightPtr = rotHeight;
	return;
    }
    /* Set the four corners of the rectangle whose center is the origin. */
    corner[1].x = corner[2].x = (double)width * 0.5;
    corner[0].x = corner[3].x = -corner[1].x;
    corner[2].y = corner[3].y = (double)height * 0.5;
    corner[0].y = corner[1].y = -corner[2].y;

    radians = (-angle / 180.0) * M_PI;
    sinTheta = sin(radians), cosTheta = cos(radians);
    xMax = yMax = 0.0;

    /* Rotate the four corners and find the maximum X and Y coordinates */

    for (i = 0; i < 4; i++) {
	x = (corner[i].x * cosTheta) - (corner[i].y * sinTheta);
	y = (corner[i].x * sinTheta) + (corner[i].y * cosTheta);
	if (x > xMax) {
	    xMax = x;
	}
	if (y > yMax) {
	    yMax = y;
	}
	if (bbox != NULL) {
	    bbox[i].x = x;
	    bbox[i].y = y;
	}
    }

    /*
     * By symmetry, the width and height of the bounding box are twice the
     * maximum x and y coordinates.
     */
    *rotWidthPtr = xMax + xMax;
    *rotHeightPtr = yMax + yMax;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TranslateAnchor --
 *
 * 	Translate the coordinates of a given bounding box based upon the
 * 	anchor specified.  The anchor indicates where the given x-y position
 * 	is in relation to the bounding box.
 *
 *  		nw --- n --- ne
 *  		|            |
 *  		w   center   e
 *  		|            |
 *  		sw --- s --- se
 *
 * 	The coordinates returned are translated to the origin of the bounding
 * 	box (suitable for giving to XCopyArea, XCopyPlane, etc.)
 *
 * Results:
 *	The translated coordinates of the bounding box are returned.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_TranslateAnchor(
    int x, int y,		/* Window coordinates of anchor */
    int w, int h,		/* Extents of the bounding box */
    Tk_Anchor anchor,		/* Direction of the anchor */
    int *xPtr, int *yPtr)
{
    switch (anchor) {
    case TK_ANCHOR_NW:		/* Upper left corner */
	break;
    case TK_ANCHOR_W:		/* Left center */
	y -= (h / 2);
	break;
    case TK_ANCHOR_SW:		/* Lower left corner */
	y -= h;
	break;
    case TK_ANCHOR_N:		/* Top center */
	x -= (w / 2);
	break;
    case TK_ANCHOR_CENTER:	/* Center */
	x -= (w / 2);
	y -= (h / 2);
	break;
    case TK_ANCHOR_S:		/* Bottom center */
	x -= (w / 2);
	y -= h;
	break;
    case TK_ANCHOR_NE:		/* Upper right corner */
	x -= w;
	break;
    case TK_ANCHOR_E:		/* Right center */
	x -= w;
	y -= (h / 2);
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
	x -= w;
	y -= h;
	break;
    }
    *xPtr = x;
    *yPtr = y;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_AnchorPoint --
 *
 * 	Translates a position, using both the dimensions of the bounding box,
 * 	and the anchor direction, returning the coordinates of the upper-left
 * 	corner of the box. The anchor indicates where the given x-y position
 * 	is in relation to the bounding box.
 *
 *  		nw --- n --- ne
 *  		|            |
 *  		w   center   e
 *  		|            |
 *  		sw --- s --- se
 *
 * 	The coordinates returned are translated to the origin of the bounding
 * 	box (suitable for giving to XCopyArea, XCopyPlane, etc.)
 *
 * Results:
 *	The translated coordinates of the bounding box are returned.
 *
 *---------------------------------------------------------------------------
 */
Point2d
Blt_AnchorPoint(
    double x, double y,		/* Coordinates of anchor. */
    double w, double h,		/* Extents of the bounding box */
    Tk_Anchor anchor)		/* Direction of the anchor */
{
    Point2d t;

    switch (anchor) {
    case TK_ANCHOR_NW:		/* Upper left corner */
	break;
    case TK_ANCHOR_W:		/* Left center */
	y -= (h * 0.5);
	break;
    case TK_ANCHOR_SW:		/* Lower left corner */
	y -= h;
	break;
    case TK_ANCHOR_N:		/* Top center */
	x -= (w * 0.5);
	break;
    case TK_ANCHOR_CENTER:	/* Center */
	x -= (w * 0.5);
	y -= (h * 0.5);
	break;
    case TK_ANCHOR_S:		/* Bottom center */
	x -= (w * 0.5);
	y -= h;
	break;
    case TK_ANCHOR_NE:		/* Upper right corner */
	x -= w;
	break;
    case TK_ANCHOR_E:		/* Right center */
	x -= w;
	y -= (h * 0.5);
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
	x -= w;
	y -= h;
	break;
    }
    t.x = x;
    t.y = y;
    return t;
}

static INLINE int
SizeOfUtfChar(const char *s)	/* Buffer in which the UTF-8 representation of
				 * the Tcl_UniChar is stored.  Buffer must be
				 * large enough to hold the UTF-8 character
				 * (at most TCL_UTF_MAX bytes). */
{
    int byte;
    
    byte = *((unsigned char *)s);
    if (byte < 0xC0) {
	return 1;
    } else if ((byte < 0xE0) && ((s[1] & 0xC0) == 0x80)) {
	return 2;
    } else if ((byte < 0xF0) && 
	       ((s[1] & 0xC0) == 0x80) && ((s[2] & 0xC0) == 0x80)) {
	return 3;
    }
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MeasureText --
 *
 *	Draw a string of characters on the screen.  Blt_DrawChars()
 *	expands control characters that occur in the string to 
 *	\xNN sequences.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_MeasureText(
    Blt_Font font,		/* Font in which characters will be drawn;
				 * must be the same as font used in GC. */
    const char *text,		/* UTF-8 string to be displayed.  Need not be
				 * '\0' terminated.  All Tk meta-characters
				 * (tabs, control characters, and newlines)
				 * should be stripped out of the string that
				 * is passed to this function.  If they are
				 * not stripped out, they will be displayed as
				 * regular printing characters. */
    int textLen,		/* # of bytes to draw in text string. */
    int maxLength, 
    int *countPtr)
{
    int elWidth;
    const char *s, *send;
    int accum, count, threshold;
    int nBytes;

    if (maxLength < 0) {
	if (countPtr != NULL) {
	    nBytes = textLen;
	}
	return Blt_TextWidth(font, text, textLen);
    }
    elWidth = Blt_TextWidth(font, "...", 3);
    threshold = maxLength - elWidth;
    if (threshold <= 0) {
	return 0;
    }
#if !HAVE_UTF
    nBytes = 1;
#endif /* !HAVE_UTF */
    count = accum = 0;
    for (s = text, send = s + textLen; s < send; s += nBytes) {
#if HAVE_UTF
	Tcl_UniChar ch;
#endif /* HAVE_UTF */
	int w;
#if HAVE_UTF
	nBytes =  Tcl_UtfToUniChar (s, &ch);
#endif /* HAVE_UTF */
	w = Blt_TextWidth(font, s, nBytes);
	if ((accum + w) > threshold) {
	    if (countPtr != NULL) {
		*countPtr = count;
	    }
	    return accum + elWidth;
	}
	accum += w;
	count += nBytes;
    }
    if (countPtr != NULL) {
	*countPtr = count;
    }
    return accum;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_CreateLayout --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *	string.
 *
 *---------------------------------------------------------------------------
 */
TextLayout *
Blt_Ts_CreateLayout(const char *text, int textLen, TextStyle *tsPtr)
{
    TextFragment *fp;
    TextLayout *layoutPtr;
    Blt_FontMetrics fm;
    int count;			/* Count # of characters on each line */
    int lineHeight;
    size_t maxHeight, maxWidth;
    size_t nFrags;
    int width;			/* Running dimensions of the text */
    const char *p, *endp, *start;
    int i;
    size_t size;

    nFrags = 0;
    endp = text + ((textLen < 0) ? strlen(text) : textLen);
    for (p = text; p < endp; p++) {
	if (*p == '\n') {
	    nFrags++;
	}
    }
    if ((p != text) && (*(p - 1) != '\n')) {
	nFrags++;
    }
    size = sizeof(TextLayout) + (sizeof(TextFragment) * (nFrags - 1));

    layoutPtr = calloc(1, size);
    layoutPtr->nFrags = nFrags;

    nFrags = count = 0;
    width = maxWidth = 0;
    maxHeight = tsPtr->padTop;
    Blt_GetFontMetrics(tsPtr->font, &fm);
    lineHeight = fm.linespace + tsPtr->leader;

    fp = layoutPtr->fragments;
    for (p = start = text; p < endp; p++) {
	if (*p == '\n') {
	    if (count > 0) {
		width = Blt_TextWidth(tsPtr->font, start, count);
		if (width > maxWidth) {
		    maxWidth = width;
		}
	    } else {
		width = 0;
	    }
	    fp->width = width;
	    fp->count = count;
	    fp->sy = fp->y = maxHeight + fm.ascent;
	    fp->text = start;
	    maxHeight += lineHeight;
	    fp++;
	    nFrags++;
	    start = p + 1;	/* Start the text on the next line */
	    count = 0;		/* Reset to indicate the start of a new
				 * line */
	    continue;
	}
	count++;
    }

    if (nFrags < layoutPtr->nFrags) {
	width = Blt_TextWidth(tsPtr->font, start, count);
	if (width > maxWidth) {
	    maxWidth = width;
	}
	fp->width = width;
	fp->count = count;
	fp->sy = fp->y = maxHeight + fm.ascent;
	fp->text = start;
	maxHeight += lineHeight;
	nFrags++;
    }
    maxHeight += tsPtr->padBottom;
    maxWidth += PADDING(tsPtr->xPad);
    fp = layoutPtr->fragments;
    for (i = 0; i < nFrags; i++, fp++) {
	switch (tsPtr->justify) {
	default:
	case TK_JUSTIFY_LEFT:
	    /* No offset for left justified text strings */
	    fp->x = fp->sx = tsPtr->padLeft;
	    break;
	case TK_JUSTIFY_RIGHT:
	    fp->x = fp->sx = (maxWidth - fp->width) - tsPtr->padRight;
	    break;
	case TK_JUSTIFY_CENTER:
	    fp->x = fp->sx = (maxWidth - fp->width) / 2;
	    break;
	}
    }
    if (tsPtr->underline >= 0) {
	fp = layoutPtr->fragments;
	for (i = 0; i < nFrags; i++, fp++) {
	    int first, last;

	    first = fp->text - text;
	    last = first + fp->count;
	    if ((tsPtr->underline >= first) && (tsPtr->underline < last)) {
		layoutPtr->underlinePtr = fp;
		layoutPtr->underline = tsPtr->underline - first;
		break;
	    }
	}
    }
    layoutPtr->width = maxWidth;
    layoutPtr->height = maxHeight - tsPtr->leader;
    return layoutPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawCharsWithEllipsis --
 *
 *	Draw a string of characters on the screen.  Blt_DrawChars()
 *	expands control characters that occur in the string to 
 *	\xNN sequences.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawCharsWithEllipsis(
    Tk_Window tkwin,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    GC gc,			/* Graphics context for drawing characters. */
    Blt_Font font,		/* Font in which characters will be drawn;
				 * must be the same as font used in GC. */
    int depth,
    float angle,
    const char *text,		/* UTF-8 string to be displayed.  Need not be
				 * '\0' terminated.  All Tk meta-characters
				 * (tabs, control characters, and newlines)
				 * should be stripped out of the string that
				 * is passed to this function.  If they are
				 * not stripped out, they will be displayed as
				 * regular printing characters. */
    int textLen,		/* # of bytes to draw in text string. */
    int x, int y,		/* Coordinates at which to place origin of
				 * string when drawing. */
    int maxLength)
{
    int elWidth;
    const char *s, *send;
    Tcl_DString dString;
    int nBytes;
    int accum, threshold;

#if HAVE_UTF
    Tcl_UniChar ch;
#endif /* HAVE_UTF */
    accum = 0;
    elWidth = Blt_TextWidth(font, "...", 3);
    if (maxLength < elWidth) {
	return;
    }
    threshold = maxLength - elWidth;
    Tcl_DStringInit(&dString);
#if !HAVE_UTF
    nBytes = 1;
#endif /* !HAVE_UTF */
    for (s = text, send = s + textLen; s < send; s += nBytes) {
#if HAVE_UTF
	nBytes =  Tcl_UtfToUniChar (s, &ch);
#endif /* HAVE_UTF */
	accum += Blt_TextWidth(font, s, nBytes);
	if (accum > threshold) {
	    break;
	}
	Tcl_DStringAppend(&dString, s, nBytes);
    }
    if (s < send) {
	Tcl_DStringAppend(&dString, "...", 3);
    }
    Blt_DrawChars(Tk_Display(tkwin), drawable, gc, font, depth, angle, 
	Tcl_DStringValue(&dString), Tcl_DStringLength(&dString), x, y);
    Tcl_DStringFree(&dString);
}

void
Blt_DrawLayout(Tk_Window tkwin, Drawable drawable, GC gc, Blt_Font font,
	       int depth, float angle, int x, int y, TextLayout *layoutPtr,
	       int maxLength)
{
    TextFragment *fp, *fend;
    Blt_FontMetrics fm;

    Blt_GetFontMetrics(font, &fm);
    for (fp = layoutPtr->fragments, fend = fp + layoutPtr->nFrags; 
	 fp < fend; fp++) {
	int sx, sy;

	sx = x + fp->sx, sy = y + fp->sy;
	if ((maxLength > 0) && ((fp->width + fp->x) > maxLength)) {
	    Blt_DrawCharsWithEllipsis(tkwin, drawable, gc, font, depth, angle, 
		fp->text, fp->count, sx, sy, maxLength - fp->x);
	} else {
 	    Blt_DrawChars(Tk_Display(tkwin), drawable, gc, font, depth, angle, 
		fp->text, fp->count, sx, sy);
	}
    }
    if (layoutPtr->underlinePtr != NULL) {
	fp = layoutPtr->underlinePtr;
	Blt_UnderlineChars(Tk_Display(tkwin), drawable, gc, font, fp->text, 
		fp->count, x + fp->sx, y + fp->sy, layoutPtr->underline, 
		layoutPtr->underline + 1, maxLength);
    }
}


#ifdef WIN32
/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_Bitmap --
 *
 *	Draw a bitmap, using the the given window coordinates as an anchor for
 *	the text bounding box.
 *
 * Results:
 *	Returns the bitmap representing the text string.
 *
 * Side Effects:
 *	Bitmap is drawn using the given font and GC in the drawable at the
 *	given coordinates, anchor, and rotation.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_Ts_Bitmap(
    Tk_Window tkwin,
    TextLayout *layoutPtr,		/* Text string to draw */
    TextStyle *stylePtr,		/* Text attributes: rotation, color,
					 * font, linespacing, justification,
					 * etc. */
    int *bmWidthPtr,
    int *bmHeightPtr)			/* Extents of rotated text string */
{
    Pixmap bitmap;
    Window root;
    GC gc;
    HDC hDC;
    TkWinDCState state;

    /* Create a temporary bitmap to contain the text string */
    root = Tk_RootWindow(tkwin);
    bitmap = Tk_GetPixmap(Tk_Display(tkwin), root, layoutPtr->width, 
	layoutPtr->height, 1);
    assert(bitmap != None);
    if (bitmap == None) {
	return None;		/* Can't allocate pixmap. */
    }
    gc = Blt_GetBitmapGC(tkwin);

    /* Clear the pixmap and draw the text string into it */
    hDC = TkWinGetDrawableDC(Tk_Display(tkwin), bitmap, &state);
    PatBlt(hDC, 0, 0, layoutPtr->width, layoutPtr->height, WHITENESS);
    TkWinReleaseDrawableDC(bitmap, hDC, &state);

    XSetFont(Tk_Display(tkwin), gc, Blt_FontId(stylePtr->font));
    XSetForeground(Tk_Display(tkwin), gc, 1);
    Blt_DrawLayout(tkwin, bitmap, gc, stylePtr->font, 1, 0.0f, 0, 0, layoutPtr, 
	stylePtr->maxLength);

    /*
     * Under Win32, 1 is off and 0 is on. That's why we're inverting the
     * bitmap here.
     */
    hDC = TkWinGetDrawableDC(Tk_Display(tkwin), bitmap, &state);
    PatBlt(hDC, 0, 0, layoutPtr->width, layoutPtr->height, DSTINVERT);
    TkWinReleaseDrawableDC(bitmap, hDC, &state);

    *bmWidthPtr = layoutPtr->width, *bmHeightPtr = layoutPtr->height;
    return bitmap;
}
#else 
/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_Bitmap --
 *
 *	Draw a bitmap, using the the given window coordinates as an anchor for
 *	the text bounding box.
 *
 * Results:
 *	Returns the bitmap representing the text string.
 *
 * Side Effects:
 *	Bitmap is drawn using the given font and GC in the drawable at the
 *	given coordinates, anchor, and rotation.
 *
 *---------------------------------------------------------------------------
 */
Pixmap
Blt_Ts_Bitmap(
    Tk_Window tkwin,
    TextLayout *layoutPtr,		/* Text string to draw */
    TextStyle *stylePtr,		/* Text attributes: rotation, color,
					 * font, linespacing, justification,
					 * etc. */
    int *bmWidthPtr,
    int *bmHeightPtr)			/* Extents of rotated text string */
{
    Pixmap bitmap;
    GC gc;

    /* Create a bitmap big enough to contain the text. */
    bitmap = Tk_GetPixmap(Tk_Display(tkwin), Tk_RootWindow(tkwin), 
	layoutPtr->width, layoutPtr->height, 1);
    assert(bitmap != None);
    if (bitmap == None) {
	return None;		/* Can't allocate pixmap. */
    }
    gc = Blt_GetBitmapGC(tkwin);

    /* Clear the bitmap.  Background is 0. */
    XSetForeground(Tk_Display(tkwin), gc, 0);
    XFillRectangle(Tk_Display(tkwin), bitmap, gc, 0, 0, layoutPtr->width, 
	layoutPtr->height);

    /* Draw the text into the bitmap. Foreground is 1. */
    XSetFont(Tk_Display(tkwin), gc, Blt_FontId(stylePtr->font));
    XSetForeground(Tk_Display(tkwin), gc, 1);
    Blt_DrawLayout(tkwin, bitmap, gc, stylePtr->font, 1, 0.0f, 0, 0, layoutPtr, 
	stylePtr->maxLength);
    *bmWidthPtr = layoutPtr->width, *bmHeightPtr = layoutPtr->height;
    return bitmap;
}
#endif /* WIN32 */

void
Blt_Ts_SetDrawStyle(
    TextStyle *stylePtr,
    Blt_Font font,
    GC gc,
    XColor *normalColor, 
    float angle,
    Tk_Anchor anchor,
    Tk_Justify justify,
    int leader)
{
    stylePtr->xPad.side1 = stylePtr->xPad.side2 = 0;
    stylePtr->yPad.side1 = stylePtr->yPad.side2 = 0;
    stylePtr->state = 0;
    stylePtr->anchor = anchor;
    stylePtr->color = normalColor;
    stylePtr->font = font;
    stylePtr->gc = gc;
    stylePtr->justify = justify;
    stylePtr->leader = leader;
    stylePtr->angle = (float)angle;
}

static void
DrawStandardLayout(Tk_Window tkwin, Drawable drawable, TextStyle *stylePtr, 
		   TextLayout *layoutPtr, int x, int y)			
{
    int w, h;
    /*
     * This is the easy case of no rotation. Simply draw the text
     * using the standard drawing routines.  Handle offset printing
     * for engraved (disabled) text.
     */
    w = layoutPtr->width;
    h = layoutPtr->height;
    if ((stylePtr->maxLength > 0) && (stylePtr->maxLength < w)) {
	w = stylePtr->maxLength;
    }
    Blt_TranslateAnchor(x, y, w, h, stylePtr->anchor, &x, &y);
    if (stylePtr->state & (STATE_DISABLED | STATE_EMPHASIS)) {
	TkBorder *borderPtr = (TkBorder *) Blt_BackgroundBorder(stylePtr->bg);
	XColor *color1, *color2;

	color1 = borderPtr->lightColor, color2 = borderPtr->darkColor;
	if (stylePtr->state & STATE_EMPHASIS) {
	    XColor *hold;
	    
	    hold = color1, color1 = color2, color2 = hold;
	}
	if (color1 != NULL) {
	    XSetForeground(Tk_Display(tkwin), stylePtr->gc, color1->pixel);
	}
	Blt_DrawLayout(tkwin, drawable, stylePtr->gc, stylePtr->font, 
		Tk_Depth(tkwin), 0.0f, x+1, y+1, layoutPtr,stylePtr->maxLength);
	if (color2 != NULL) {
	    XSetForeground(Tk_Display(tkwin), stylePtr->gc, color2->pixel);
	}
	Blt_DrawLayout(tkwin, drawable, stylePtr->gc, stylePtr->font, 
		Tk_Depth(tkwin), 0.0f, x, y, layoutPtr, stylePtr->maxLength);
	
	/* Reset the foreground color back to its original setting, so not to
	 * invalidate the GC cache. */
	XSetForeground(Tk_Display(tkwin), stylePtr->gc, stylePtr->color->pixel);
	
	return;		/* Done */
    }
    Blt_DrawLayout(tkwin, drawable, stylePtr->gc, stylePtr->font, 
	Tk_Depth(tkwin), 0.0f, x, y, layoutPtr, stylePtr->maxLength);
}


static void
RotateStartingTextPositions(TextLayout *lPtr, int w, int h, float angle)
{
    Point2d off1, off2;
    TextFragment *fp, *fend;
    double radians;
    double rw, rh;
    double sinTheta, cosTheta;
    
    Blt_GetBoundingBox(w, h, angle, &rw, &rh, (Point2d *)NULL);
    off1.x = (double)w * 0.5;
    off1.y = (double)h * 0.5;
    off2.x = rw * 0.5;
    off2.y = rh * 0.5;
    radians = (-angle / 180.0) * M_PI;
    
    sinTheta = sin(radians), cosTheta = cos(radians);
    for (fp = lPtr->fragments, fend = fp + lPtr->nFrags; fp < fend; fp++) {
	Point2d p, q;
	
	p.x = fp->x - off1.x;
	p.y = fp->y - off1.y;
	q.x = (p.x * cosTheta) - (p.y * sinTheta);
	q.y = (p.x * sinTheta) + (p.y * cosTheta);
	q.x += off2.x;
	q.y += off2.y;
	fp->sx = ROUND(q.x);
	fp->sy = ROUND(q.y);
    }
}

void
Blt_RotateStartingTextPositions(TextLayout *lPtr, float angle)
{
    RotateStartingTextPositions(lPtr, lPtr->width, lPtr->height, angle);
}

int
Blt_DrawTextWithRotatedFont(Tk_Window tkwin, Drawable drawable, float angle,
			    TextStyle *stylePtr, TextLayout *layoutPtr, 
			    int x, int y)
{
    double rw, rh;
    int w, h;

    w = layoutPtr->width;
    h = layoutPtr->height;
    if ((stylePtr->maxLength > 0) && (stylePtr->maxLength < w)) {
	w = stylePtr->maxLength;
    }
    RotateStartingTextPositions(layoutPtr, w, h, angle);
    Blt_GetBoundingBox(w, h, angle, &rw, &rh, (Point2d *)NULL);
    Blt_TranslateAnchor(x, y, (int)rw, (int)rh, stylePtr->anchor, &x, &y);
    if (stylePtr->state & (STATE_DISABLED | STATE_EMPHASIS)) {
	TkBorder *borderPtr = (TkBorder *)Blt_BackgroundBorder(stylePtr->bg);
	XColor *color1, *color2;
	
	color1 = borderPtr->lightColor, color2 = borderPtr->darkColor;
	if (stylePtr->state & STATE_EMPHASIS) {
	    XColor *hold;
	    
	    hold = color1, color1 = color2, color2 = hold;
	}
	if (color1 != NULL) {
	    XSetForeground(Tk_Display(tkwin), stylePtr->gc, color1->pixel);
	    Blt_DrawLayout(tkwin, drawable, stylePtr->gc, stylePtr->font, 
		Tk_Depth(tkwin), angle, x, y, layoutPtr, stylePtr->maxLength); 
	}
	if (color2 != NULL) {
	    XSetForeground(Tk_Display(tkwin), stylePtr->gc, color2->pixel);
	    Blt_DrawLayout(tkwin, drawable, stylePtr->gc, stylePtr->font, 
		Tk_Depth(tkwin), angle, x, y, layoutPtr, stylePtr->maxLength); 
	}
	XSetForeground(Tk_Display(tkwin), stylePtr->gc, stylePtr->color->pixel);
	return TRUE;
    }
    XSetForeground(Tk_Display(tkwin), stylePtr->gc, stylePtr->color->pixel);
    Blt_DrawLayout(tkwin, drawable, stylePtr->gc, stylePtr->font, 
	Tk_Depth(tkwin), angle, x, y, layoutPtr, stylePtr->maxLength); 
    return TRUE;
}

static void
Blt_DrawTextWithRotatedBitmap(
    Tk_Window tkwin,
    Drawable drawable,
    float angle,
    TextStyle *stylePtr,		/* Text attribute information */
    TextLayout *layoutPtr,
    int x, int y)			/* Window coordinates to draw text */
{
    int width, height;
    Display *display;
    Pixmap bitmap;

    display = Tk_Display(tkwin);
    /*
     * Rotate the text by writing the text into a bitmap and rotating the
     * bitmap.  Set the clip mask and origin in the GC first.  And make sure
     * we restore the GC because it may be shared.
     */
    stylePtr->angle = angle;

    bitmap = Blt_Ts_Bitmap(tkwin, layoutPtr, stylePtr, &width, &height);
    if (bitmap == None) {
	return;
    }
    if ((bitmap != None) && (stylePtr->angle != 0.0)) {
	Pixmap rotated;

	rotated = Blt_RotateBitmap(tkwin, bitmap, width, height, 
		stylePtr->angle, &width, &height);
	Tk_FreePixmap(display, bitmap);
	bitmap = rotated;
    }
    Blt_TranslateAnchor(x, y, width, height, stylePtr->anchor, &x, &y);
    XSetClipMask(display, stylePtr->gc, bitmap);

    if (stylePtr->state & (STATE_DISABLED | STATE_EMPHASIS)) {
	TkBorder *borderPtr = (TkBorder *) Blt_BackgroundBorder(stylePtr->bg);
	XColor *color1, *color2;

	color1 = borderPtr->lightColor, color2 = borderPtr->darkColor;
	if (stylePtr->state & STATE_EMPHASIS) {
	    XColor *hold;

	    hold = color1, color1 = color2, color2 = hold;
	}
	if (color1 != NULL) {
	    XSetForeground(display, stylePtr->gc, color1->pixel);
	}
	XSetClipOrigin(display, stylePtr->gc, x + 1, y + 1);
	XCopyPlane(display, bitmap, drawable, stylePtr->gc, 0, 0, width, 
		height, x + 1, y + 1, 1);
	if (color2 != NULL) {
	    XSetForeground(display, stylePtr->gc, color2->pixel);
	}
	XSetClipOrigin(display, stylePtr->gc, x, y);
	XCopyPlane(display, bitmap, drawable, stylePtr->gc, 0, 0, width, 
		height, x, y, 1);
	XSetForeground(display, stylePtr->gc, stylePtr->color->pixel);
    } else {
	XSetForeground(display, stylePtr->gc, stylePtr->color->pixel);
	XSetClipOrigin(display, stylePtr->gc, x, y);
	XCopyPlane(display, bitmap, drawable, stylePtr->gc, 0, 0, width, height, 
		x, y, 1);
    }
    XSetClipMask(display, stylePtr->gc, None);
    Tk_FreePixmap(display, bitmap);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_DrawLayout --
 *
 *	Draw a text string, possibly rotated, using the the given window
 *	coordinates as an anchor for the text bounding box.  If the text is
 *	not rotated, simply use the X text drawing routines. Otherwise,
 *	generate a bitmap of the rotated text.
 *
 * Results:
 *	Returns the x-coordinate to the right of the text.
 *
 * Side Effects:
 *	Text string is drawn using the given font and GC at the the given
 *	window coordinates.
 *
 *      The Stipple, FillStyle, and TSOrigin fields of the GC are modified for
 *      rotated text.  This assumes the GC is private, *not* shared (via
 *      Tk_GetGC)
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ts_DrawLayout(
    Tk_Window tkwin,
    Drawable drawable,
    TextLayout *layoutPtr,
    TextStyle *stylePtr,		/* Text attribute information */
    int x, int y)			/* Window coordinates to draw text */
{
    float angle;

    if ((stylePtr->gc == NULL) || (stylePtr->flags & UPDATE_GC)) {
	Blt_Ts_ResetStyle(tkwin, stylePtr);
    }
    angle = (float)fmod(stylePtr->angle, 360.0);
    if (angle < 0.0) {
	angle += 360.0;
    }
    if (angle == 0.0) {
	/*
	 * This is the easy case of no rotation. Simply draw the text using
	 * the standard drawing routines.  Handle offset printing for engraved
	 * (disabled) text.
	 */
	DrawStandardLayout(tkwin, drawable, stylePtr, layoutPtr, x, y);
	return;
    }
    if (Blt_CanRotateFont(stylePtr->font, angle)) {
	if (Blt_DrawTextWithRotatedFont(tkwin, drawable, angle, stylePtr, 
		layoutPtr, x, y)) {
	    return;		/* Success. */
	}
    }
    /*Fallthru*/
    stylePtr->angle = (float)angle;
    Blt_DrawTextWithRotatedBitmap(tkwin, drawable, angle, stylePtr, layoutPtr,
		x, y);
}

void
Blt_Ts_UnderlineLayout(
    Tk_Window tkwin,
    Drawable drawable,
    TextLayout *layoutPtr,
    TextStyle *stylePtr,		/* Text attribute information */
    int x, int y)			/* Window coordinates to draw text */
{
    float angle;

    if ((stylePtr->gc == NULL) || (stylePtr->flags & UPDATE_GC)) {
	Blt_Ts_ResetStyle(tkwin, stylePtr);
    }
    angle = (float)fmod(stylePtr->angle, 360.0);
    if (angle < 0.0) {
	angle += 360.0;
    }
    if (angle == 0.0) {
	TextFragment *fp, *fend;

	for (fp = layoutPtr->fragments, fend = fp + layoutPtr->nFrags; 
	     fp < fend; fp++) {
	    int sx, sy;

	    sx = x + fp->sx, sy = y + fp->sy;
	    Blt_UnderlineChars(Tk_Display(tkwin), drawable, stylePtr->gc, 
		stylePtr->font, fp->text, fp->count, sx, sy, 0, fp->count, 
		stylePtr->maxLength);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_DrawLayout --
 *
 *	Draw a text string, possibly rotated, using the the given window
 *	coordinates as an anchor for the text bounding box.  If the text is
 *	not rotated, simply use the X text drawing routines. Otherwise,
 *	generate a bitmap of the rotated text.
 *
 * Results:
 *	Returns the x-coordinate to the right of the text.
 *
 * Side Effects:
 *	Text string is drawn using the given font and GC at the the given
 *	window coordinates.
 *
 *      The Stipple, FillStyle, and TSOrigin fields of the GC are modified for
 *      rotated text.  This assumes the GC is private, *not* shared (via
 *      Tk_GetGC)
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Ts_DrawText(
    Tk_Window tkwin,
    Drawable drawable,
    const char *text,
    int textLen,
    TextStyle *stylePtr,		/* Text attribute information */
    int x, int y)			/* Window coordinates to draw text */
{
    TextLayout *layoutPtr;

    layoutPtr = Blt_Ts_CreateLayout(text, textLen, stylePtr);
    Blt_Ts_DrawLayout(tkwin, drawable, layoutPtr, stylePtr, x, y);
    free(layoutPtr);
}

void
Blt_DrawText2(
    Tk_Window tkwin,
    Drawable drawable,
    const char *string,
    TextStyle *stylePtr,		/* Text attribute information */
    int x, int y,		/* Window coordinates to draw text */
    Dim2D *areaPtr)
{
    TextLayout *layoutPtr;
    int width, height;
    float angle;

    if ((string == NULL) || (*string == '\0')) {
	return;			/* Empty string, do nothing */
    }
    layoutPtr = Blt_Ts_CreateLayout(string, -1, stylePtr);
    Blt_Ts_DrawLayout(tkwin, drawable, layoutPtr, stylePtr, x, y);
    angle = fmod(stylePtr->angle, 360.0);
    if (angle < 0.0) {
	angle += 360.0;
    }
    width = layoutPtr->width;
    height = layoutPtr->height;
    if (angle != 0.0) {
	double rotWidth, rotHeight;

	Blt_GetBoundingBox(width, height, angle, &rotWidth, &rotHeight, 
	   (Point2d *)NULL);
	width = ROUND(rotWidth);
	height = ROUND(rotHeight);
    }
    areaPtr->width = width;
    areaPtr->height = height;
    free(layoutPtr);
}

void
Blt_DrawText(
    Tk_Window tkwin,
    Drawable drawable,
    const char *string,
    TextStyle *stylePtr,		/* Text attribute information */
    int x, int y)		/* Window coordinates to draw text */
{
    TextLayout *layoutPtr;

    if ((string == NULL) || (*string == '\0')) {
	return;			/* Empty string, do nothing */
    }
    layoutPtr = Blt_Ts_CreateLayout(string, -1, stylePtr);
    Blt_Ts_DrawLayout(tkwin, drawable, layoutPtr, stylePtr, x, y);
    free(layoutPtr);
}

void
Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *stylePtr)
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCFont;
    gcValues.font = Blt_FontId(stylePtr->font);
    if (stylePtr->color != NULL) {
	gcMask |= GCForeground;
	gcValues.foreground = stylePtr->color->pixel;
    }
    newGC = Tk_GetGC(tkwin, gcMask, &gcValues);
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(Tk_Display(tkwin), stylePtr->gc);
    }
    stylePtr->gc = newGC;
    stylePtr->flags &= ~UPDATE_GC;
}

void
Blt_Ts_FreeStyle(Display *display, TextStyle *stylePtr)
{
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(display, stylePtr->gc);
    }
}

/*
 * The following two structures are used to keep track of string
 * measurement information when using the text layout facilities.
 *
 * A LayoutChunk represents a contiguous range of text that can be measured
 * and displayed by low-level text calls.  In general, chunks will be
 * delimited by newlines and tabs.  Low-level, platform-specific things
 * like kerning and non-integer character widths may occur between the
 * characters in a single chunk, but not between characters in different
 * chunks.
 *
 * A TextLayout is a collection of LayoutChunks.  It can be displayed with
 * respect to any origin.  It is the implementation of the Tk_TextLayout
 * opaque token.
 */

typedef struct LayoutChunk {
    const char *start;		/* Pointer to simple string to be displayed.
				 * This is a pointer into the TkTextLayout's
				 * string. */
    int numBytes;		/* The number of bytes in this chunk. */
    int numChars;		/* The number of characters in this chunk. */
    int numDisplayChars;	/* The number of characters to display when
				 * this chunk is displayed.  Can be less than
				 * numChars if extra space characters were
				 * absorbed by the end of the chunk.  This
				 * will be < 0 if this is a chunk that is
				 * holding a tab or newline. */
    int x, y;			/* The origin of the first character in this
				 * chunk with respect to the upper-left hand
				 * corner of the TextLayout. */
    int totalWidth;		/* Width in pixels of this chunk.  Used
				 * when hit testing the invisible spaces at
				 * the end of a chunk. */
    int displayWidth;		/* Width in pixels of the displayable
				 * characters in this chunk.  Can be less than
				 * width if extra space characters were
				 * absorbed by the end of the chunk. */
} LayoutChunk;

typedef struct TkTextLayout {
    Blt_Font font;		/* The font used when laying out the text. */
    const char *string;		/* The string that was layed out. */
    int width;			/* The maximum width of all lines in the
				 * text layout. */
    int numChunks;		/* Number of chunks actually used in
				 * following array. */
    LayoutChunk chunks[1];	/* Array of chunks.  The actual size will
				 * be maxChunks.  THIS FIELD MUST BE THE LAST
				 * IN THE STRUCTURE. */
} TkTextLayout;


/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeTextLayout --
 *
 *	This procedure is called to release the storage associated with
 *	a Tk_TextLayout when it is no longer needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *---------------------------------------------------------------------------
 */

void
Blt_FreeTextLayout(Tk_TextLayout textLayout)
{
    TkTextLayout *layoutPtr = (TkTextLayout *) textLayout;

    if (layoutPtr != NULL) {
	free(layoutPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NewChunk --
 *
 *	Helper function for Blt_ComputeTextLayout().  Encapsulates a
 *	measured set of characters in a chunk that can be quickly
 *	drawn.
 *
 * Results:
 *	A pointer to the new chunk in the text layout.
 *
 * Side effects:
 *	The text layout is reallocated to hold more chunks as necessary.
 *
 *	Currently, Tk_ComputeTextLayout() stores contiguous ranges of
 *	"normal" characters in a chunk, along with individual tab
 *	and newline chars in their own chunks.  All characters in the
 *	text layout are accounted for.
 *
 *---------------------------------------------------------------------------
 */
static LayoutChunk *
NewChunk(TkTextLayout **layoutPtrPtr, int *maxPtr, const char *start, 
	 int numBytes, int curX, int newX, int y)
{
    TkTextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    int maxChunks, numChars;
    size_t s;
    
    layoutPtr = *layoutPtrPtr;
    maxChunks = *maxPtr;
    if (layoutPtr->numChunks == maxChunks) {
	maxChunks *= 2;
	s = sizeof(TkTextLayout) + ((maxChunks - 1) * sizeof(LayoutChunk));
	layoutPtr = realloc(layoutPtr, s);
	*layoutPtrPtr = layoutPtr;
	*maxPtr = maxChunks;
    }
    numChars = Tcl_NumUtfChars(start, numBytes);
    chunkPtr = &layoutPtr->chunks[layoutPtr->numChunks];
    chunkPtr->start		= start;
    chunkPtr->numBytes		= numBytes;
    chunkPtr->numChars		= numChars;
    chunkPtr->numDisplayChars	= numChars;
    chunkPtr->x			= curX;
    chunkPtr->y			= y;
    chunkPtr->totalWidth	= newX - curX;
    chunkPtr->displayWidth	= newX - curX;
    layoutPtr->numChunks++;

    return chunkPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ComputeTextLayout --
 *
 *	Computes the amount of screen space needed to display a multi-line,
 *	justified string of text.  Records all the measurements that were done
 *	to determine to size and positioning of the individual lines of text;
 *	this information can be used by the Tk_DrawTextLayout() procedure to
 *	display the text quickly (without remeasuring it).
 *
 *	This procedure is useful for simple widgets that want to display
 *	single-font, multi-line text and want Tk to handle the details.
 *
 * Results:
 *	The return value is a Tk_TextLayout token that holds the measurement
 *	information for the given string.  The token is only valid for the
 *	given string.  If the string is freed, the token is no longer valid
 *	and must also be freed.  To free the token, call Tk_FreeTextLayout().
 *
 *	The dimensions of the screen area needed to display the text are
 *	stored in *widthPtr and *heightPtr.
 *
 * Side effects:
 *	Memory is allocated to hold the measurement information.  
 *
 *---------------------------------------------------------------------------
 */

Tk_TextLayout
Blt_ComputeTextLayout(
    Blt_Font font,		/* Font that will be used to display text. */
    const char *string,		/* String whose dimensions are to be
				 * computed. */
    int numChars,		/* Number of characters to consider from
				 * string, or < 0 for strlen(). */
    int wrapLength,		/* Longest permissible line length, in
				 * pixels.  <= 0 means no automatic wrapping:
				 * just let lines get as long as needed. */
    Tk_Justify justify,		/* How to justify lines. */
    int flags,			/* Flag bits OR-ed together.
				 * TK_IGNORE_TABS means that tab characters
				 * should not be expanded.  TK_IGNORE_NEWLINES
				 * means that newline characters should not
				 * cause a line break. */
    int *widthPtr,		/* Filled with width of string. */
    int *heightPtr)		/* Filled with height of string. */
{
    const char *start, *end, *special;
    int n, y, bytesThisChunk, maxChunks;
    int baseline, height, curX, newX, maxWidth;
    TkTextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    Blt_FontMetrics fm;
    Tcl_DString lineBuffer;
    int *lineLengths;
    int curLine, layoutHeight;

    Tcl_DStringInit(&lineBuffer);
    
    if ((font == NULL) || (string == NULL)) {
	if (widthPtr != NULL) {
	    *widthPtr = 0;
	}
	if (heightPtr != NULL) {
	    *heightPtr = 0;
	}
	return NULL;
    }

    Blt_GetFontMetrics(font, &fm);
    height = fm.ascent + fm.descent;

    if (numChars < 0) {
	numChars = Tcl_NumUtfChars(string, -1);
    }
    if (wrapLength == 0) {
	wrapLength = -1;
    }

    maxChunks = 1;

    layoutPtr = malloc(sizeof(TkTextLayout) + (maxChunks - 1) * 
				sizeof(LayoutChunk));
    layoutPtr->font	    = font;
    layoutPtr->string	    = string;
    layoutPtr->numChunks    = 0;

    baseline = fm.ascent;
    maxWidth = 0;

    /*
     * Divide the string up into simple strings and measure each string.
     */

    curX = 0;

    end = Tcl_UtfAtIndex(string, numChars);
    special = string;

    flags &= TK_IGNORE_TABS | TK_IGNORE_NEWLINES;
    flags |= TK_WHOLE_WORDS | TK_AT_LEAST_ONE;	    
    for (start = string; start < end; ) {
	if (start >= special) {
	    /*
	     * Find the next special character in the string.
	     *
	     * INTL: Note that it is safe to increment by byte, because we are
	     * looking for 7-bit characters that will appear unchanged in
	     * UTF-8.  At some point we may need to support the full Unicode
	     * whitespace set.
	     */

	    for (special = start; special < end; special++) {
		if (!(flags & TK_IGNORE_NEWLINES)) {
		    if ((*special == '\n') || (*special == '\r')) {
			break;
		    }
		}
		if (!(flags & TK_IGNORE_TABS)) {
		    if (*special == '\t') {
			break;
		    }
		}
	    }
	}

	/*
	 * Special points at the next special character (or the end of the
	 * string).  Process characters between start and special.
	 */

	chunkPtr = NULL;
	if (start < special) {
	    bytesThisChunk = Blt_MeasureChars(font, start, special - start,
		    wrapLength - curX, flags, &newX);
	    newX += curX;
	    flags &= ~TK_AT_LEAST_ONE;
	    if (bytesThisChunk > 0) {
		chunkPtr = NewChunk(&layoutPtr, &maxChunks, start,
			bytesThisChunk, curX, newX, baseline);
			
		start += bytesThisChunk;
		curX = newX;
	    }
	}

	if ((start == special) && (special < end)) {
	    /*
	     * Handle the special character.
	     *
	     * INTL: Special will be pointing at a 7-bit character so we
	     * can safely treat it as a single byte.
	     */

	    chunkPtr = NULL;
	    if (*special == '\t') {
		newX = curX + fm.tabWidth;
		newX -= newX % fm.tabWidth;
		NewChunk(&layoutPtr, &maxChunks, start, 1, curX, newX,
			baseline)->numDisplayChars = -1;
		start++;
		if ((start < end) &&
			((wrapLength <= 0) || (newX <= wrapLength))) {
		    /*
		     * More chars can still fit on this line.
		     */

		    curX = newX;
		    flags &= ~TK_AT_LEAST_ONE;
		    continue;
		}
	    } else {	
		NewChunk(&layoutPtr, &maxChunks, start, 1, curX, curX,
			baseline)->numDisplayChars = -1;
		start++;
		goto wrapLine;
	    }
	}

	/*
	 * No more characters are going to go on this line, either because
	 * no more characters can fit or there are no more characters left.
	 * Consume all extra spaces at end of line.  
	 */

	while ((start < end) && isspace(UCHAR(*start))) { /* INTL: ISO space */
	    if (!(flags & TK_IGNORE_NEWLINES)) {
		if ((*start == '\n') || (*start == '\r')) {
		    break;
		}
	    }
	    if (!(flags & TK_IGNORE_TABS)) {
		if (*start == '\t') {
		    break;
		}
	    }
	    start++;
	}
	if (chunkPtr != NULL) {
	    const char *end;

	    /*
	     * Append all the extra spaces on this line to the end of the
	     * last text chunk.  This is a little tricky because we are
	     * switching back and forth between characters and bytes.
	     */

	    end = chunkPtr->start + chunkPtr->numBytes;
	    bytesThisChunk = start - end;
	    if (bytesThisChunk > 0) {
		bytesThisChunk = Blt_MeasureChars(font, end, bytesThisChunk,
			-1, 0, &chunkPtr->totalWidth);
		chunkPtr->numBytes += bytesThisChunk;
		chunkPtr->numChars += Tcl_NumUtfChars(end, bytesThisChunk);
		chunkPtr->totalWidth += curX;
	    }
	}

        wrapLine: 
	flags |= TK_AT_LEAST_ONE;

	/*
	 * Save current line length, then move current position to start of
	 * next line.
	 */

	if (curX > maxWidth) {
	    maxWidth = curX;
	}

	/*
	 * Remember width of this line, so that all chunks on this line
	 * can be centered or right justified, if necessary.
	 */

	Tcl_DStringAppend(&lineBuffer, (char *) &curX, sizeof(curX));

	curX = 0;
	baseline += height;
    }

    /*
     * If last line ends with a newline, then we need to make a 0 width
     * chunk on the next line.  Otherwise "Hello" and "Hello\n" are the
     * same height.
     */

    if ((layoutPtr->numChunks > 0) && ((flags & TK_IGNORE_NEWLINES) == 0)) {
	if (layoutPtr->chunks[layoutPtr->numChunks - 1].start[0] == '\n') {
	    chunkPtr = NewChunk(&layoutPtr, &maxChunks, start, 0, curX,
		    curX, baseline);
	    chunkPtr->numDisplayChars = -1;
	    Tcl_DStringAppend(&lineBuffer, (char *) &curX, sizeof(curX));
	    baseline += height;
	}
    }	    

    layoutPtr->width = maxWidth;
    layoutHeight = baseline - fm.ascent;
    if (layoutPtr->numChunks == 0) {
	layoutHeight = height;

	/*
	 * This fake chunk is used by the other procedures so that they can
	 * pretend that there is a chunk with no chars in it, which makes
	 * the coding simpler.
	 */

	layoutPtr->numChunks = 1;
	layoutPtr->chunks[0].start		= string;
	layoutPtr->chunks[0].numBytes		= 0;
	layoutPtr->chunks[0].numChars		= 0;
	layoutPtr->chunks[0].numDisplayChars	= -1;
	layoutPtr->chunks[0].x			= 0;
	layoutPtr->chunks[0].y			= fm.ascent;
	layoutPtr->chunks[0].totalWidth		= 0;
	layoutPtr->chunks[0].displayWidth	= 0;
    } else {
	/*
	 * Using maximum line length, shift all the chunks so that the lines
	 * are all justified correctly.
	 */
    
	curLine = 0;
	chunkPtr = layoutPtr->chunks;
	y = chunkPtr->y;
	lineLengths = (int *) Tcl_DStringValue(&lineBuffer);
	for (n = 0; n < layoutPtr->numChunks; n++) {
	    int extra;

	    if (chunkPtr->y != y) {
		curLine++;
		y = chunkPtr->y;
	    }
	    extra = maxWidth - lineLengths[curLine];
	    if (justify == TK_JUSTIFY_CENTER) {
		chunkPtr->x += extra / 2;
	    } else if (justify == TK_JUSTIFY_RIGHT) {
		chunkPtr->x += extra;
	    }
	    chunkPtr++;
	}
    }

    if (widthPtr != NULL) {
	*widthPtr = layoutPtr->width;
    }
    if (heightPtr != NULL) {
	*heightPtr = layoutHeight;
    }
    Tcl_DStringFree(&lineBuffer);

    return (Tk_TextLayout) layoutPtr;
}

void
Blt_DrawTextLayout(
    Display *display,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    GC gc,			/* Graphics context to use for drawing text. */
    Tk_TextLayout layout,	/* Layout information, from a previous call
				 * to Blt_ComputeTextLayout(). */
    int x, int y,		/* Upper-left hand corner of rectangle in
				 * which to draw (pixels). */
    int firstChar,		/* The index of the first character to draw
				 * from the given text item.  0 specfies the
				 * beginning. */
    int lastChar)		/* The index just after the last character
				 * to draw from the given text item.  A number
				 * < 0 means to draw all characters. */
{
    TkTextLayout *layoutPtr;
    int i, numDisplayChars, drawX;
    const char *firstByte;
    const char *lastByte;
    LayoutChunk *chunkPtr;
    int depth = 24;

    layoutPtr = (TkTextLayout *) layout;
    if (layoutPtr == NULL) {
	return;
    }

    if (lastChar < 0) {
	lastChar = 100000000;
    }
    chunkPtr = layoutPtr->chunks;
    for (i = 0; i < layoutPtr->numChunks; i++) {
	numDisplayChars = chunkPtr->numDisplayChars;
	if ((numDisplayChars > 0) && (firstChar < numDisplayChars)) {
	    if (firstChar <= 0) {
		drawX = 0;
		firstChar = 0;
		firstByte = chunkPtr->start;
	    } else {
		firstByte = Tcl_UtfAtIndex(chunkPtr->start, firstChar);
		Blt_MeasureChars(layoutPtr->font, chunkPtr->start,
			firstByte - chunkPtr->start, -1, 0, &drawX);
	    }
	    if (lastChar < numDisplayChars) {
		numDisplayChars = lastChar;
	    }
	    lastByte = Tcl_UtfAtIndex(chunkPtr->start, numDisplayChars);
	    Blt_DrawChars(display, drawable, gc, layoutPtr->font, depth, 0.0f,
		    firstByte, lastByte - firstByte,
		    x + chunkPtr->x + drawX, y + chunkPtr->y);
	}
	firstChar -= chunkPtr->numChars;
	lastChar -= chunkPtr->numChars;
	if (lastChar <= 0) {
	    break;
	}
	chunkPtr++;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_CharBbox --
 *
 *	Use the information in the Tk_TextLayout token to return the
 *	bounding box for the character specified by index.  
 *
 *	The width of the bounding box is the advance width of the
 *	character, and does not include and left- or right-bearing.
 *	Any character that extends partially outside of the
 *	text layout is considered to be truncated at the edge.  Any
 *	character which is located completely outside of the text
 *	layout is considered to be zero-width and pegged against
 *	the edge.
 *
 *	The height of the bounding box is the line height for this font,
 *	extending from the top of the ascent to the bottom of the
 *	descent.  Information about the actual height of the individual
 *	letter is not available.
 *
 *	A text layout that contains no characters is considered to
 *	contain a single zero-width placeholder character.
 * 
 * Results:
 *	The return value is 0 if the index did not specify a character
 *	in the text layout, or non-zero otherwise.  In that case,
 *	*bbox is filled with the bounding box of the character.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_CharBbox(
    Tk_TextLayout layout,   /* Layout information, from a previous call to
			     * Tk_ComputeTextLayout(). */
    int index,		    /* The index of the character whose bbox is
			     * desired. */
    int *xPtr, int *yPtr,    /* Filled with the upper-left hand corner, in
			     * pixels, of the bounding box for the character
			     * specified by index, if non-NULL. */
    int *widthPtr, 
    int *heightPtr)	    /* Filled with the width and height of the
			     * bounding box for the character specified by
			     * index, if non-NULL. */
{
    TkTextLayout *layoutPtr;
    LayoutChunk *chunkPtr;
    int i, x, w;
    Blt_Font font;
    const char *end;
    Blt_FontMetrics fm;

    if (index < 0) {
	return 0;
    }

    layoutPtr = (TkTextLayout *) layout;
    chunkPtr = layoutPtr->chunks;
    font = layoutPtr->font;

    Blt_GetFontMetrics(font, &fm);
    for (i = 0; i < layoutPtr->numChunks; i++) {
	if (chunkPtr->numDisplayChars < 0) {
	    if (index == 0) {
		x = chunkPtr->x;
		w = chunkPtr->totalWidth;
		goto check;
	    }
	} else if (index < chunkPtr->numChars) {
	    end = Tcl_UtfAtIndex(chunkPtr->start, index);
	    if (xPtr != NULL) {
		Blt_MeasureChars(font, chunkPtr->start,
			end -  chunkPtr->start, -1, 0, &x);
		x += chunkPtr->x;
	    }
	    if (widthPtr != NULL) {
		Blt_MeasureChars(font, end, Tcl_UtfNext(end) - end, -1, 0, &w);
	    }
	    goto check;
	}
	index -= chunkPtr->numChars;
	chunkPtr++;
    }
    if (index == 0) {
	/*
	 * Special case to get location just past last char in layout.
	 */

	chunkPtr--;
	x = chunkPtr->x + chunkPtr->totalWidth;
	w = 0;
    } else {
	return 0;
    }

    /*
     * Ensure that the bbox lies within the text layout.  This forces all
     * chars that extend off the right edge of the text layout to have
     * truncated widths, and all chars that are completely off the right
     * edge of the text layout to peg to the edge and have 0 width.
     */
    check:
    if (yPtr != NULL) {
	*yPtr = chunkPtr->y - fm.ascent;
    }
    if (heightPtr != NULL) {
	*heightPtr = fm.ascent + fm.descent;
    }

    if (x > layoutPtr->width) {
	x = layoutPtr->width;
    }
    if (xPtr != NULL) {
	*xPtr = x;
    }
    if (widthPtr != NULL) {
	if (x + w > layoutPtr->width) {
	    w = layoutPtr->width - x;
	}
	*widthPtr = w;
    }

    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UnderlineTextLayout --
 *
 *	Use the information in the Tk_TextLayout token to display an
 *	underline below an individual character.  This procedure does
 *	not draw the text, just the underline.
 *
 *	This procedure is useful for simple widgets that need to
 *	display single-font, multi-line text with an individual
 *	character underlined and want Tk to handle the details.
 *	To display larger amounts of underlined text, construct
 *	and use an underlined font.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Underline drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */

void
Blt_UnderlineTextLayout(
    Display *display,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    GC gc,			/* Graphics context to use for drawing text. */
    Tk_TextLayout layout,	/* Layout information, from a previous call
				 * to Tk_ComputeTextLayout(). */
    int x, int y,		/* Upper-left hand corner of rectangle in
				 * which to draw (pixels). */
    int underline)		/* Index of the single character to
				 * underline, or -1 for no underline. */
{
    TkTextLayout *layoutPtr;
    int xx, yy, width, height;

    if ((Blt_CharBbox(layout, underline, &xx, &yy, &width, &height) != 0)
	    && (width != 0)) {
	Blt_FontMetrics fm;
	layoutPtr = (TkTextLayout *) layout;
	Blt_GetFontMetrics(layoutPtr->font, &fm);
	XFillRectangle(display, drawable, gc, x + xx, 
		y + yy + fm.ascent + fm.underlinePos, 
		(unsigned int) width, fm.underlineHeight);
    }
}
