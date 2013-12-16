/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

#include <tk.h>

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

#include <assert.h>
#include <X11/Xutil.h>

#include <tkPort.h>
#include <tkInt.h>
#include <tk3d.h>

#include "bltInt.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltImage.h"
#include "bltBitmap.h"
#include "bltText.h"
#include "bltBgStyle.h"

static Blt_HashTable bitmapGCTable;
static int initialized;

void dump_Ts(TextStyle* ts)
{
  printf("  state: %d\n",ts->state);
  printf("  angle: %f\n",ts->angle);
  printf("  anchor: %d\n",ts->anchor);
  printf("  justify: %d\n",ts->justify);
  printf("  leader: %d\n",ts->leader);
  printf("  underline: %d\n",ts->underline);
  printf("  maxLength: %d\n",ts->maxLength);
  printf("  flags: %d\n",ts->flags);
}

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
		   Tk_Font font, 
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
    Tk_FontMetrics fm;

    Tk_GetFontMetrics(font, &fm);
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
		    
	  lineWidth = Tk_TextWidth(font, line, lineLen);
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
      lineWidth = Tk_TextWidth(font, line, lineLen);
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
 * Blt_AnchorPoint --
 *
 * 	Translates a position, using both the dimensions of the bounding box,
 * 	and the anchor direction, returning the coordinates of the upper-left
 * 	corner of the box. The anchor indicates where the given x-y position
 * 	is in relation to the bounding box.
 *
 *  		7 nw --- 0 n --- 1 ne
 *  		 |                |
 *  		6 w    8 center  2 e
 *  		 |                |
 *  		5 sw --- 4 s --- 3 se
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
  case TK_ANCHOR_NW:		/* 7 Upper left corner */
    break;
  case TK_ANCHOR_W:		/* 6 Left center */
    y -= (h * 0.5);
    break;
  case TK_ANCHOR_SW:		/* 5 Lower left corner */
    y -= h;
    break;
  case TK_ANCHOR_N:		/* 0 Top center */
    x -= (w * 0.5);
    break;
  case TK_ANCHOR_CENTER:	/* 8 Center */
    x -= (w * 0.5);
    y -= (h * 0.5);
    break;
  case TK_ANCHOR_S:		/* 4 Bottom center */
    x -= (w * 0.5);
    y -= h;
    break;
  case TK_ANCHOR_NE:		/* 1 Upper right corner */
    x -= w;
    break;
  case TK_ANCHOR_E:		/* 2 Right center */
    x -= w;
    y -= (h * 0.5);
    break;
  case TK_ANCHOR_SE:		/* 3 Lower right corner */
    x -= w;
    y -= h;
    break;
  }
  t.x = x;
  t.y = y;
  return t;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Ts_DrawText --
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
  printf("Blt_Ts_DrawText: %s\n",text);
  dump_Ts(stylePtr);

  if ((text == NULL) || (*text == '\0'))
    return;

  if ((stylePtr->gc == NULL) || (stylePtr->flags & UPDATE_GC))
    Blt_Ts_ResetStyle(tkwin, stylePtr);

  int w1, h1;
  Tk_TextLayout layout = Tk_ComputeTextLayout(stylePtr->font, text, textLen,-1,
					      stylePtr->justify, 0,
					      &w1, &h1);

  //  Matrix t0 = Translate(-x,-y);
  //  Matrix t1 = Translate(-w1/2,-h1/2);
  //  Matrix rr = Rotate(angle);
  //  Matrix t2 = Translate(w2/2,h2/2);
  //  Matrix t3 = Translate(x,y);

  float angle = stylePtr->angle;
  float ccos = cos(M_PI*angle/180.);
  float ssin = sin(M_PI*angle/180.);

  double w2,h2;
  Blt_GetBoundingBox(w1, h1, angle, &w2, &h2, (Point2d *)NULL);

  float x1 = x+w1/2.;
  float y1 = y+h1/2.;
  float x2 = w2/2.+x;
  float y2 = h2/2.+y;

  int rx =  x*ccos + y*ssin + (-x1*ccos -y1*ssin +x2);
  int ry = -x*ssin + y*ccos + ( x1*ssin -y1*ccos +y2);

  Point2d rr = Blt_AnchorPoint(rx, ry, w2, h2, stylePtr->anchor);

  TkDrawAngledTextLayout(Tk_Display(tkwin), drawable, stylePtr->gc, layout,
  			 rr.x, rr.y, angle, 0, textLen);
}

void
Blt_DrawText2(
	      Tk_Window tkwin,
	      Drawable drawable,
	      const char *text,
	      TextStyle *stylePtr,		/* Text attribute information */
	      int x, int y,		/* Window coordinates to draw text */
	      Dim2D *areaPtr)
{
  //  printf("Blt_DrawText2: %s\n",text);
  //    dump_Ts(stylePtr);

  if ((text == NULL) || (*text == '\0'))
    return;

  if ((stylePtr->gc == NULL) || (stylePtr->flags & UPDATE_GC))
    Blt_Ts_ResetStyle(tkwin, stylePtr);

  int width, height;
  Tk_TextLayout layout = Tk_ComputeTextLayout(stylePtr->font, text, -1, -1, 
					      stylePtr->justify, 0,
					      &width, &height);
  Point2d vv = Blt_AnchorPoint(x, y, width, height, stylePtr->anchor);
  TkDrawAngledTextLayout(Tk_Display(tkwin), drawable, stylePtr->gc, layout,
			 vv.x, vv.y, stylePtr->angle, 0, -1);

  float angle = fmod(stylePtr->angle, 360.0);
  if (angle < 0.0) {
    angle += 360.0;
  }
  if (angle != 0.0) {
    double rotWidth, rotHeight;

    Blt_GetBoundingBox(width, height, angle, &rotWidth, &rotHeight, 
		       (Point2d *)NULL);
    width = ROUND(rotWidth);
    height = ROUND(rotHeight);
  }
  areaPtr->width = width;
  areaPtr->height = height;
}

void
Blt_DrawText(
	     Tk_Window tkwin,
	     Drawable drawable,
	     const char *text,
	     TextStyle *stylePtr,		/* Text attribute information */
	     int x, int y)		/* Window coordinates to draw text */
{
  //  printf("Blt_DrawText: %s\n",text);
  //    dump_Ts(stylePtr);

  if ((text == NULL) || (*text == '\0'))
    return;

  if ((stylePtr->gc == NULL) || (stylePtr->flags & UPDATE_GC))
    Blt_Ts_ResetStyle(tkwin, stylePtr);

  int width, height;
  Tk_TextLayout layout = Tk_ComputeTextLayout(stylePtr->font, text, -1, -1, 
					      stylePtr->justify, 0,
					      &width, &height);
  Point2d vv = Blt_AnchorPoint(x, y, width, height, stylePtr->anchor);
  TkDrawAngledTextLayout(Tk_Display(tkwin), drawable, stylePtr->gc, layout, 
			 vv.x, vv.y, stylePtr->angle, 0, -1);
}

void
Blt_Ts_ResetStyle(Tk_Window tkwin, TextStyle *stylePtr)
{
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;
 
  gcMask = GCFont;
  gcValues.font = Tk_FontId(stylePtr->font);
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
