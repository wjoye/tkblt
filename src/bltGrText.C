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

#include <math.h>

extern "C" {
#include <tk.h>
#include <tkPort.h>
#include <tkInt.h>
};

#include "bltText.h"
#include "bltGraph.h"
#include "bltPs.h"

#define ROTATE_0	0
#define ROTATE_90	1
#define ROTATE_180	2
#define ROTATE_270	3

TextStyle::TextStyle(Graph* graphPtr)
{
  ops_ = (TextStyleOptions*)calloc(1, sizeof(TextStyleOptions));
  TextStyleOptions* ops = (TextStyleOptions*)ops_;
  graphPtr_ = graphPtr;
  manageOptions_ = 1;

  ops->anchor =TK_ANCHOR_NW;
  ops->color =NULL;
  ops->font =NULL;
  ops->angle =0;
  ops->justify =TK_JUSTIFY_LEFT;

  xPad_ = 0;
  yPad_ = 0;
  gc_ = NULL;
}

TextStyle::TextStyle(Graph* graphPtr, TextStyleOptions* ops)
{
  ops_ = (TextStyleOptions*)ops;
  graphPtr_ = graphPtr;
  manageOptions_ = 0;

  xPad_ = 0;
  yPad_ = 0;
  gc_ = NULL;
}

TextStyle::~TextStyle()
{
  //  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  if (gc_)
    Tk_FreeGC(graphPtr_->display_, gc_);

  if (manageOptions_)
    free(ops_);
}

void TextStyle::drawText(Drawable drawable, const char *text, int x, int y)
{
  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  if (!text || !(*text))
    return;

  if (!gc_)
    resetStyle();

  int w1, h1;
  Tk_TextLayout layout = Tk_ComputeTextLayout(ops->font, text, -1, -1,
					      ops->justify, 0, &w1, &h1);
  Point2d rr = rotateText(x, y, w1, h1);
  TkDrawAngledTextLayout(graphPtr_->display_, drawable, gc_, layout,
  			 rr.x, rr.y, ops->angle, 0, -1);
}

void TextStyle::drawText2(Drawable drawable, const char *text,
			  int x, int y, int* ww, int* hh)
{
  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  if (!text || !(*text))
    return;

  if (!gc_)
    resetStyle();

  int w1, h1;
  Tk_TextLayout layout = Tk_ComputeTextLayout(ops->font, text, -1, -1, 
					      ops->justify, 0, &w1, &h1);
  Point2d rr = rotateText(x, y, w1, h1);
  TkDrawAngledTextLayout(graphPtr_->display_, drawable, gc_, layout,
  			 rr.x, rr.y, ops->angle, 0, -1);

  float angle = fmod(ops->angle, 360.0);
  if (angle < 0.0)
    angle += 360.0;

  if (angle != 0.0) {
    double rotWidth, rotHeight;
    Blt_GetBoundingBox(w1, h1, angle, &rotWidth, &rotHeight, (Point2d*)NULL);
    w1 = rotWidth;
    h1 = rotHeight;
  }

  *ww = w1;
  *hh = h1;
}

void TextStyle::printText(Blt_Ps ps, const char *text, int x, int y)
{
  //  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  if (!text || !(*text))
    return;
}

void TextStyle::resetStyle()
{
  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  unsigned long gcMask;
  gcMask = GCFont;

  XGCValues gcValues;
  gcValues.font = Tk_FontId(ops->font);
  if (ops->color) {
    gcMask |= GCForeground;
    gcValues.foreground = ops->color->pixel;
  }
  GC newGC = Tk_GetGC(graphPtr_->tkwin_, gcMask, &gcValues);
  if (gc_)
    Tk_FreeGC(graphPtr_->display_, gc_);

  gc_ = newGC;
}

Point2d TextStyle::rotateText(int x, int y, int w1, int h1)
{
  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  //  Matrix t0 = Translate(-x,-y);
  //  Matrix t1 = Translate(-w1/2,-h1/2);
  //  Matrix rr = Rotate(angle);
  //  Matrix t2 = Translate(w2/2,h2/2);
  //  Matrix t3 = Translate(x,y);

  double angle = ops->angle;
  double ccos = cos(M_PI*angle/180.);
  double ssin = sin(M_PI*angle/180.);
  double w2, h2;
  Blt_GetBoundingBox(w1, h1, angle, &w2, &h2, (Point2d *)NULL);

  double x1 = x+w1/2.;
  double y1 = y+h1/2.;
  double x2 = w2/2.+x;
  double y2 = h2/2.+y;

  double rx =  x*ccos + y*ssin + (-x1*ccos -y1*ssin +x2);
  double ry = -x*ssin + y*ccos + ( x1*ssin -y1*ccos +y2);

  return Blt_AnchorPoint(rx, ry, w2, h2, ops->anchor);
}

void TextStyle::getExtents(const char *text, int* ww, int* hh)
{
  TextStyleOptions* ops = (TextStyleOptions*)ops_;

  int w, h;
  Blt_GetTextExtents(ops->font, text, -1, &w, &h);
  *ww = w + 2*xPad_;
  *hh = h + 2*yPad_;
}

void Blt_GetTextExtents(Tk_Font font, const char *text, int textLen,
			int* ww, int* hh)
{
  if (!text) {
    *ww =0;
    *hh =0;
    return;
  }

  Tk_FontMetrics fm;
  Tk_GetFontMetrics(font, &fm);
  int lineHeight = fm.linespace;

  if (textLen < 0)
    textLen = strlen(text);

  int maxWidth =0;
  int maxHeight =0;
  int lineLen =0;
  const char *line =NULL;
  const char *p, *pend;
  for (p = line = text, pend = text + textLen; p < pend; p++) {
    if (*p == '\n') {
      if (lineLen > 0) {
	int lineWidth = Tk_TextWidth(font, line, lineLen);
	if (lineWidth > maxWidth)
	  maxWidth = lineWidth;
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
    maxHeight += lineHeight;
    int lineWidth = Tk_TextWidth(font, line, lineLen);
    if (lineWidth > maxWidth)
      maxWidth = lineWidth;
  }

  *ww = maxWidth;
  *hh = maxHeight;
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
void Blt_GetBoundingBox(int width, int height, float angle,
			double *rotWidthPtr, double *rotHeightPtr,
			Point2d *bbox)
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

    /* Handle right-angle rotations specially. */

    int quadrant = (int)(angle / 90.0);
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
    if (bbox) {
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
    if (x > xMax)
      xMax = x;

    if (y > yMax)
      yMax = y;

    if (bbox) {
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
Point2d Blt_AnchorPoint(double x, double y, double w, double h,	
			Tk_Anchor anchor)
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

