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
 *
 */

#include <stdarg.h>

extern "C" {
#include <tkPort.h>
#include <tkInt.h>
#include <tk3d.h>
};

#include "bltMath.h"

extern "C" {
#include "bltInt.h"
};

#include "bltGrMisc.h"
#include "bltGrPageSetup.h"
#include "bltPs.h"

#ifdef TCL_UTF_MAX
#  define HAVE_UTF	1
#else
#  define HAVE_UTF	0
#endif

#define FONT_ITALIC	(1<<0)
#define FONT_BOLD	(1<<1)

#define PS_MAXPATH	1500		/* Maximum number of components in a
					 * PostScript (level 1) path. */

static Tcl_Interp *psInterp = NULL;

int Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int width, int height)
{
  PageSetupOptions* pops = (PageSetupOptions*)setupPtr->ops_;
  int paperWidth, paperHeight;
  float hScale, vScale, scale;

  int x = pops->xPad;
  int y = pops->yPad;

  int hBorder = 2*pops->xPad;
  int vBorder = 2*pops->yPad;

  int hSize, vSize;
  if (pops->landscape) {
    hSize = height;
    vSize = width;
  } else {
    hSize = width;
    vSize = height;
  }
  /*
   * If the paper size wasn't specified, set it to the graph size plus the
   * paper border.
   */
  paperWidth = (pops->reqPaperWidth > 0) ? pops->reqPaperWidth :
    hSize + hBorder;
  paperHeight = (pops->reqPaperHeight > 0) ? pops->reqPaperHeight :
    vSize + vBorder;

  /*
   * Scale the plot size (the graph itself doesn't change size) if it's
   * bigger than the paper or if -maxpect was set.
   */
  hScale = vScale = 1.0f;
  if ((setupPtr->flags & PS_MAXPECT) || ((hSize + hBorder) > paperWidth)) {
    hScale = (float)(paperWidth - hBorder) / (float)hSize;
  }
  if ((setupPtr->flags & PS_MAXPECT) || ((vSize + vBorder) > paperHeight)) {
    vScale = (float)(paperHeight - vBorder) / (float)vSize;
  }
  scale = MIN(hScale, vScale);
  if (scale != 1.0f) {
    hSize = (int)((hSize * scale) + 0.5f);
    vSize = (int)((vSize * scale) + 0.5f);
  }
  setupPtr->scale = scale;
  if (pops->center) {
    if (paperWidth > hSize) {
      x = (paperWidth - hSize) / 2;
    }
    if (paperHeight > vSize) {
      y = (paperHeight - vSize) / 2;
    }
  }
  setupPtr->left = x;
  setupPtr->bottom = y;
  setupPtr->right = x + hSize - 1;
  setupPtr->top = y + vSize - 1;
  setupPtr->paperHeight = paperHeight;
  setupPtr->paperWidth = paperWidth;
  return paperHeight;
}

PostScript *Blt_Ps_Create(Tcl_Interp* interp, PageSetup *setupPtr)
{
  PostScript* psPtr = (PostScript*)malloc(sizeof(PostScript));
  psPtr->setupPtr = setupPtr;
  psPtr->interp = interp;
  Tcl_DStringInit(&psPtr->dString);
  return psPtr;
}

void Blt_Ps_SetPrinting(PostScript *psPtr, int state)
{
  psInterp = ((state) && (psPtr != NULL)) ? psPtr->interp : NULL;
}

void Blt_Ps_Free(PostScript *psPtr)
{
  Tcl_DStringFree(&psPtr->dString);
  free(psPtr);
}

const char *Blt_Ps_GetValue(PostScript *psPtr, int *lengthPtr)
{
  *lengthPtr = strlen(Tcl_DStringValue(&psPtr->dString));
  return Tcl_DStringValue(&psPtr->dString);
}

char *Blt_Ps_GetScratchBuffer(PostScript *psPtr)
{
  return psPtr->scratchArr;
}

void Blt_Ps_VarAppend TCL_VARARGS_DEF(PostScript *, arg1)
{
  va_list argList;
  PostScript *psPtr;
  const char *string;

  psPtr = TCL_VARARGS_START(PostScript, arg1, argList);
  for (;;) {
    string = va_arg(argList, char *);
    if (string == NULL) {
      break;
    }
    Tcl_DStringAppend(&psPtr->dString, string, -1);
  }
}

void Blt_Ps_Append(PostScript *psPtr, const char *string)
{
  Tcl_DStringAppend(&psPtr->dString, string, -1);
}

void Blt_Ps_Format TCL_VARARGS_DEF(PostScript *, arg1)
{
  va_list argList;
  PostScript *psPtr;
  char *fmt;

  psPtr = TCL_VARARGS_START(PostScript, arg1, argList);
  fmt = va_arg(argList, char *);
  vsnprintf(psPtr->scratchArr, POSTSCRIPT_BUFSIZ, fmt, argList);
  va_end(argList);
  Tcl_DStringAppend(&psPtr->dString, psPtr->scratchArr, -1);
}

int Blt_Ps_IncludeFile(Tcl_Interp* interp, Blt_Ps ps, const char *fileName)
{
  Tcl_Channel channel;
  Tcl_DString dString;
  char *buf;
  char *libDir;
  int nBytes;

  buf = Blt_Ps_GetScratchBuffer(ps);

  /*
   * Read in a standard prolog file from file and append it to the
   * PostScript output stored in the Tcl_DString in psPtr.
   */

  libDir = (char *)Tcl_GetVar(interp, "tkblt_library", TCL_GLOBAL_ONLY);
  if (libDir == NULL) {
    Tcl_AppendResult(interp, "couldn't find TKBLT script library:",
		     "global variable \"tkblt_library\" doesn't exist", (char *)NULL);
    return TCL_ERROR;
  }
  Tcl_DStringInit(&dString);
  Tcl_DStringAppend(&dString, libDir, -1);
  Tcl_DStringAppend(&dString, "/", -1);
  Tcl_DStringAppend(&dString, fileName, -1);
  fileName = Tcl_DStringValue(&dString);
  Blt_Ps_VarAppend(ps, "\n% including file \"", fileName, "\"\n\n", 
		   (char *)NULL);
  channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
  if (channel == NULL) {
    Tcl_AppendResult(interp, "couldn't open prologue file \"", fileName,
		     "\": ", Tcl_PosixError(interp), (char *)NULL);
    return TCL_ERROR;
  }
  for(;;) {
    nBytes = Tcl_Read(channel, buf, POSTSCRIPT_BUFSIZ);
    if (nBytes < 0) {
      Tcl_AppendResult(interp, "error reading prologue file \"", 
		       fileName, "\": ", Tcl_PosixError(interp), 
		       (char *)NULL);
      Tcl_Close(interp, channel);
      Tcl_DStringFree(&dString);
      return TCL_ERROR;
    }
    if (nBytes == 0) {
      break;
    }
    buf[nBytes] = '\0';
    Blt_Ps_Append(ps, buf);
  }
  Tcl_DStringFree(&dString);
  Tcl_Close(interp, channel);
  return TCL_OK;
}

static void XColorToPostScript(Blt_Ps ps, XColor* colorPtr)
{
  Blt_Ps_Format(ps, "%g %g %g",
		((double)(colorPtr->red >> 8) / 255.0),
		((double)(colorPtr->green >> 8) / 255.0),
		((double)(colorPtr->blue >> 8) / 255.0));
}

void Blt_Ps_XSetBackground(PostScript *psPtr, XColor* colorPtr)
{
  PageSetupOptions* pops = (PageSetupOptions*)psPtr->setupPtr->ops_;
  XColorToPostScript(psPtr, colorPtr);
  Blt_Ps_Append(psPtr, " setrgbcolor\n");
  if (pops->greyscale) {
    Blt_Ps_Append(psPtr, " currentgray setgray\n");
  }
}

void Blt_Ps_XSetForeground(PostScript *psPtr, XColor* colorPtr)
{
  PageSetupOptions* pops = (PageSetupOptions*)psPtr->setupPtr->ops_;
  XColorToPostScript(psPtr, colorPtr);
  Blt_Ps_Append(psPtr, " setrgbcolor\n");
  if (pops->greyscale) {
    Blt_Ps_Append(psPtr, " currentgray setgray\n");
  }
}

static unsigned char ReverseBits(unsigned char byte)
{
  byte = ((byte >> 1) & 0x55) | ((byte << 1) & 0xaa);
  byte = ((byte >> 2) & 0x33) | ((byte << 2) & 0xcc);
  byte = ((byte >> 4) & 0x0f) | ((byte << 4) & 0xf0);
  return byte;
}

static void ByteToHex(unsigned char byte, char *string)
{
  static char hexDigits[] = "0123456789ABCDEF";

  string[0] = hexDigits[byte >> 4];
  string[1] = hexDigits[byte & 0x0F];
}

void Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display, Pixmap bitmap, 
			   int w, int h)
{
  XImage *imagePtr;
  int byteCount;
  int y, bitPos;

  imagePtr = XGetImage(display, bitmap, 0, 0, w, h, 1, ZPixmap);
  Blt_Ps_Append(ps, "\t<");
  byteCount = bitPos = 0;	/* Suppress compiler warning */
  for (y = 0; y < h; y++) {
    unsigned char byte;
    char string[10];
    int x;

    byte = 0;
    for (x = 0; x < w; x++) {
      unsigned long pixel;

      pixel = XGetPixel(imagePtr, x, y);
      bitPos = x % 8;
      byte |= (unsigned char)(pixel << bitPos);
      if (bitPos == 7) {
	byte = ReverseBits(byte);
	ByteToHex(byte, string);
	string[2] = '\0';
	byteCount++;
	byte = 0;
	if (byteCount >= 30) {
	  string[2] = '\n';
	  string[3] = '\t';
	  string[4] = '\0';
	  byteCount = 0;
	}
	Blt_Ps_Append(ps, string);
      }
    }			/* x */
    if (bitPos != 7) {
      byte = ReverseBits(byte);
      ByteToHex(byte, string);
      string[2] = '\0';
      Blt_Ps_Append(ps, string);
      byteCount++;
    }
  }				/* y */
  Blt_Ps_Append(ps, ">\n");
  XDestroyImage(imagePtr);
}

void Blt_Ps_SetClearBackground(Blt_Ps ps)
{
  Blt_Ps_Append(ps, "1 1 1 setrgbcolor\n");
}

void Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle)
{
  /*
   * X11:not last = 0, butt = 1, round = 2, projecting = 3
   * PS: butt = 0, round = 1, projecting = 2
   */
  if (capStyle > 0) {
    capStyle--;
  }
  Blt_Ps_Format(ps, "%d setlinecap\n", capStyle);
}

void Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle)
{
  /*
   * miter = 0, round = 1, bevel = 2
   */
  Blt_Ps_Format(ps, "%d setlinejoin\n", joinStyle);
}

void Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth)
{
  if (lineWidth < 1) {
    lineWidth = 1;
  }
  Blt_Ps_Format(ps, "%d setlinewidth\n", lineWidth);
}

void Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr)
{

  Blt_Ps_Append(ps, "[ ");
  if (dashesPtr != NULL) {
    unsigned char *vp;

    for (vp = dashesPtr->values; *vp != 0; vp++) {
      Blt_Ps_Format(ps, " %d", *vp);
    }
  }
  Blt_Ps_Append(ps, "] 0 setdash\n");
}

void Blt_Ps_XSetLineAttributes(
			       Blt_Ps ps,
			       XColor* colorPtr,
			       int lineWidth,
			       Blt_Dashes *dashesPtr,
			       int capStyle, 
			       int joinStyle)
{
  Blt_Ps_XSetJoinStyle(ps, joinStyle);
  Blt_Ps_XSetCapStyle(ps, capStyle);
  Blt_Ps_XSetForeground(ps, colorPtr);
  Blt_Ps_XSetLineWidth(ps, lineWidth);
  Blt_Ps_XSetDashes(ps, dashesPtr);
  Blt_Ps_Append(ps, "/DashesProc {} def\n");
}

void Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int width, int height)
{
  Blt_Ps_Append(ps, "newpath\n");
  Blt_Ps_Format(ps, "  %d %d moveto\n", x, y);
  Blt_Ps_Format(ps, "  %d %d rlineto\n", width, 0);
  Blt_Ps_Format(ps, "  %d %d rlineto\n", 0, height);
  Blt_Ps_Format(ps, "  %d %d rlineto\n", -width, 0);
  Blt_Ps_Append(ps, "closepath\n");
}

void Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y, int width, int height)
{
  Blt_Ps_Rectangle(ps, (int)x, (int)y, width, height);
  Blt_Ps_Append(ps, "fill\n");
}

void Blt_Ps_PolylineFromXPoints(Blt_Ps ps, XPoint *points, int n)
{
  XPoint *pp, *pend;

  pp = points;
  Blt_Ps_Append(ps, "newpath\n");
  Blt_Ps_Format(ps, "  %d %d moveto\n", pp->x, pp->y);
  pp++;
  for (pend = points + n; pp < pend; pp++) {
    Blt_Ps_Format(ps, "  %d %d lineto\n", pp->x, pp->y);
  }
}

void Blt_Ps_Polyline(Blt_Ps ps, Point2d *screenPts, int nScreenPts)
{
  Point2d *pp, *pend;

  pp = screenPts;
  Blt_Ps_Append(ps, "newpath\n");
  Blt_Ps_Format(ps, "  %g %g moveto\n", pp->x, pp->y);
  for (pp++, pend = screenPts + nScreenPts; pp < pend; pp++) {
    Blt_Ps_Format(ps, "  %g %g lineto\n", pp->x, pp->y);
  }
}

void Blt_Ps_Polygon(Blt_Ps ps, Point2d *screenPts, int nScreenPts)
{
  Point2d *pp, *pend;

  pp = screenPts;
  Blt_Ps_Append(ps, "newpath\n");
  Blt_Ps_Format(ps, "  %g %g moveto\n", pp->x, pp->y);
  for (pp++, pend = screenPts + nScreenPts; pp < pend; pp++) {
    Blt_Ps_Format(ps, "  %g %g lineto\n", pp->x, pp->y);
  }
  Blt_Ps_Format(ps, "  %g %g lineto\n", screenPts[0].x, screenPts[0].y);
  Blt_Ps_Append(ps, "closepath\n");
}

void Blt_Ps_XFillPolygon(Blt_Ps ps, Point2d *screenPts, int nScreenPts)
{
  Blt_Ps_Polygon(ps, screenPts, nScreenPts);
  Blt_Ps_Append(ps, "fill\n");
}

void Blt_Ps_XDrawSegments(Blt_Ps ps, XSegment *segments, int nSegments)
{
  XSegment *sp, *send;

  for (sp = segments, send = sp + nSegments; sp < send; sp++) {
    Blt_Ps_Format(ps, "%d %d moveto %d %d lineto\n", sp->x1, sp->y1, 
		  sp->x2, sp->y2);
    Blt_Ps_Append(ps, "DashesProc stroke\n");
  }
}

void Blt_Ps_XFillRectangles(Blt_Ps ps, XRectangle *rectangles, int nRectangles)
{
  XRectangle *rp, *rend;

  for (rp = rectangles, rend = rp + nRectangles; rp < rend; rp++) {
    Blt_Ps_XFillRectangle(ps, (double)rp->x, (double)rp->y, 
			  (int)rp->width, (int)rp->height);
  }
}

void Blt_Ps_Draw3DRectangle(
			    Blt_Ps ps,
			    Tk_3DBorder border,		/* Token for border to draw. */
			    double x, double y,		/* Coordinates of rectangle */
			    int width, int height,	/* Region to be drawn. */
			    int borderWidth,		/* Desired width for border, in pixels. */
			    int relief)			/* Should be either TK_RELIEF_RAISED or
							 * TK_RELIEF_SUNKEN; indicates position of
							 * interior of window relative to exterior. */
{
  Point2d points[7];
  TkBorder *borderPtr = (TkBorder *) border;
  XColor* lightPtr, *darkPtr;
  XColor* topPtr, *bottomPtr;
  XColor light, dark;
  int twiceWidth = (borderWidth * 2);

  if ((width < twiceWidth) || (height < twiceWidth)) {
    return;
  }
  if ((relief == TK_RELIEF_SOLID) ||
      (borderPtr->lightColorPtr == NULL) || (borderPtr->darkColorPtr == NULL)) {
    if (relief == TK_RELIEF_SOLID) {
      dark.red = dark.blue = dark.green = 0x00;
      light.red = light.blue = light.green = 0x00;
      relief = TK_RELIEF_SUNKEN;
    } else {
      light = *borderPtr->bgColorPtr;
      dark.red = dark.blue = dark.green = 0xFF;
    }
    lightPtr = &light;
    darkPtr = &dark;
  } else {
    lightPtr = borderPtr->lightColorPtr;
    darkPtr = borderPtr->darkColorPtr;
  }


  /* Handle grooves and ridges with recursive calls. */

  if ((relief == TK_RELIEF_GROOVE) || (relief == TK_RELIEF_RIDGE)) {
    int halfWidth, insideOffset;

    halfWidth = borderWidth / 2;
    insideOffset = borderWidth - halfWidth;
    Blt_Ps_Draw3DRectangle(ps, border, (double)x, (double)y,
			   width, height, halfWidth, 
			   (relief == TK_RELIEF_GROOVE) ? TK_RELIEF_SUNKEN : TK_RELIEF_RAISED);
    Blt_Ps_Draw3DRectangle(ps, border, 
			   (double)(x + insideOffset), (double)(y + insideOffset), 
			   width - insideOffset * 2, height - insideOffset * 2, halfWidth,
			   (relief == TK_RELIEF_GROOVE) ? TK_RELIEF_RAISED : TK_RELIEF_SUNKEN);
    return;
  }
  if (relief == TK_RELIEF_RAISED) {
    topPtr = lightPtr;
    bottomPtr = darkPtr;
  } else if (relief == TK_RELIEF_SUNKEN) {
    topPtr = darkPtr;
    bottomPtr = lightPtr;
  } else {
    topPtr = bottomPtr = borderPtr->bgColorPtr;
  }
  Blt_Ps_XSetBackground(ps, bottomPtr);
  Blt_Ps_XFillRectangle(ps, x, y + height - borderWidth, width, borderWidth);
  Blt_Ps_XFillRectangle(ps, x + width - borderWidth, y, borderWidth, height);
  points[0].x = points[1].x = points[6].x = x;
  points[0].y = points[6].y = y + height;
  points[1].y = points[2].y = y;
  points[2].x = x + width;
  points[3].x = x + width - borderWidth;
  points[3].y = points[4].y = y + borderWidth;
  points[4].x = points[5].x = x + borderWidth;
  points[5].y = y + height - borderWidth;
  if (relief != TK_RELIEF_FLAT) {
    Blt_Ps_XSetBackground(ps, topPtr);
  }
  Blt_Ps_XFillPolygon(ps, points, 7);
}

void Blt_Ps_Fill3DRectangle(
			    Blt_Ps ps,
			    Tk_3DBorder border,		/* Token for border to draw. */
			    double x, double y,		/* Coordinates of top-left of border area */
			    int width, int height,	/* Dimension of border to be drawn. */
			    int borderWidth,		/* Desired width for border, in pixels. */
			    int relief)			/* Should be either TK_RELIEF_RAISED or
							 * TK_RELIEF_SUNKEN;  indicates position of
							 * interior of window relative to exterior. */
{
  TkBorder *borderPtr = (TkBorder *) border;

  Blt_Ps_XSetBackground(ps, borderPtr->bgColorPtr);
  Blt_Ps_XFillRectangle(ps, x, y, width, height);
  Blt_Ps_Draw3DRectangle(ps, border, x, y, width, height, borderWidth,
			 relief);
}

void Blt_Ps_XSetStipple(Blt_Ps ps, Display *display, Pixmap bitmap)
{
  int width, height;

  Tk_SizeOfBitmap(display, bitmap, &width, &height);
  Blt_Ps_Format(ps, 
		"gsave\n"
		"  clip\n"
		"  %d %d\n", 
		width, height);
  Blt_Ps_XSetBitmapData(ps, display, bitmap, width, height);
  Blt_Ps_VarAppend(ps, 
		   "  StippleFill\n"
		   "grestore\n", (char *)NULL);
}

void Blt_Ps_XSetFont(PostScript *psPtr, Tk_Font font) 
{
#if 0
  Tcl_Interp* interp = psPtr->interp;
  const char *family;

  /*
   * Check to see if it's a PostScript font. Tk_PostScriptFontName silently
   * generates a bogus PostScript font name, so we have to check to see if
   * this is really a PostScript font first before we call it.
   */
  family = FamilyToPsFamily(Blt_FamilyOfFont(font));
  if (family != NULL) {
    Tcl_DString dString;
    double pointSize;
	
    Tcl_DStringInit(&dString);
    pointSize = (double)Blt_PostscriptFontName(font, &dString);
    Blt_Ps_Format(psPtr, "%g /%s SetFont\n", pointSize, 
		  Tcl_DStringValue(&dString));
    Tcl_DStringFree(&dString);
    return;
  }
  Blt_Ps_Append(psPtr, "12.0 /Helvetica-Bold SetFont\n");
#endif
}

#if 0
static void TextLayoutToPostScript(Blt_Ps ps, int x, int y, TextLayout *textPtr)
{
  char *bp, *dst;
  int count;			/* Counts the # of bytes written to the
				 * intermediate scratch buffer. */
  const char *src, *end;
  TextFragment *fragPtr;
  int i;
  unsigned char c;
#if HAVE_UTF
  Tcl_UniChar ch;
#endif
  int limit;

  limit = POSTSCRIPT_BUFSIZ - 4; /* High water mark for scratch buffer. */
  fragPtr = textPtr->fragments;
  for (i = 0; i < textPtr->nFrags; i++, fragPtr++) {
    if (fragPtr->count < 1) {
      continue;
    }
    Blt_Ps_Append(ps, "(");
    count = 0;
    dst = Blt_Ps_GetScratchBuffer(ps);
    src = fragPtr->text;
    end = fragPtr->text + fragPtr->count;
    while (src < end) {
      if (count > limit) {
	/* Don't let the scatch buffer overflow */
	dst = Blt_Ps_GetScratchBuffer(ps);
	dst[count] = '\0';
	Blt_Ps_Append(ps, dst);
	count = 0;
      }
#if HAVE_UTF
      /*
       * INTL: For now we just treat the characters as binary data and
       * display the lower byte.  Eventually this should be revised to
       * handle international postscript fonts.
       */
      src += Tcl_UtfToUniChar(src, &ch);
      c = (unsigned char)(ch & 0xff);
#else 
      c = *src++;
#endif

      if ((c == '\\') || (c == '(') || (c == ')')) {
	/*
	 * If special PostScript characters characters "\", "(", and
	 * ")" are contained in the text string, prepend backslashes
	 * to them.
	 */
	*dst++ = '\\';
	*dst++ = c;
	count += 2;
      } else if ((c < ' ') || (c > '~')) {
	/* Convert non-printable characters into octal. */
	sprintf_s(dst, 5, "\\%03o", c);
	dst += 4;
	count += 4;
      } else {
	*dst++ = c;
	count++;
      }
    }
    bp = Blt_Ps_GetScratchBuffer(ps);
    bp[count] = '\0';
    Blt_Ps_Append(ps, bp);
    Blt_Ps_Format(ps, ") %d %d %d DrawAdjText\n",
		  fragPtr->width, x + fragPtr->x, y + fragPtr->y);
  }
}
#endif

void Blt_Ps_DrawText(
		     Blt_Ps ps,
		     const char *string,		/* String to convert to PostScript */
		     TextStyle *tsPtr,		/* Text attribute information */
		     double x, double y)		/* Window coordinates where to print text */
{
#if 0
  TextLayout *textPtr;
  Point2d t;

  if ((string == NULL) || (*string == '\0')) { /* Empty string, do nothing */
    return;
  }
  textPtr = Blt_Ts_CreateLayout(string, -1, tsPtr);
  {
    float angle;
    double rw, rh;
	
    angle = fmod(tsPtr->angle, (double)360.0);
    Blt_GetBoundingBox(textPtr->width, textPtr->height, angle, &rw, &rh, 
		       (Point2d *)NULL);
    /*
     * Find the center of the bounding box
     */
    t = Blt_AnchorPoint(x, y, rw, rh, tsPtr->anchor); 
    t.x += rw * 0.5;
    t.y += rh * 0.5;
  }

  /* Initialize text (sets translation and rotation) */
  Blt_Ps_Format(ps, "%d %d %g %g %g BeginText\n", textPtr->width, 
		textPtr->height, tsPtr->angle, t.x, t.y);

  Blt_Ps_XSetFont(ps, tsPtr->font);

  Blt_Ps_XSetForeground(ps, tsPtr->color);
  TextLayoutToPostScript(ps, 0, 0, textPtr);
  free(textPtr);
  Blt_Ps_Append(ps, "EndText\n");
#endif
}

void Blt_Ps_XDrawLines(Blt_Ps ps, XPoint *points, int n)
{
  int nLeft;

  if (n <= 0) {
    return;
  }
  for (nLeft = n; nLeft > 0; nLeft -= PS_MAXPATH) {
    int length;

    length = MIN(PS_MAXPATH, nLeft);
    Blt_Ps_PolylineFromXPoints(ps, points, length);
    Blt_Ps_Append(ps, "DashesProc stroke\n");
    points += length;
  }
}

void Blt_Ps_DrawPolyline(Blt_Ps ps, Point2d *points, int nPoints)
{
  int nLeft;

  if (nPoints <= 0) {
    return;
  }
  for (nLeft = nPoints; nLeft > 0; nLeft -= PS_MAXPATH) {
    int length;

    length = MIN(PS_MAXPATH, nLeft);
    Blt_Ps_Polyline(ps, points, length);
    Blt_Ps_Append(ps, "DashesProc stroke\n");
    points += length;
  }
}

void Blt_Ps_DrawBitmap(
		       Blt_Ps ps,
		       Display *display,
		       Pixmap bitmap,		/* Bitmap to be converted to PostScript */
		       double xScale, double yScale)
{
  int width, height;
  double sw, sh;

  Tk_SizeOfBitmap(display, bitmap, &width, &height);
  sw = (double)width * xScale;
  sh = (double)height * yScale;
  Blt_Ps_Append(ps, "  gsave\n");
  Blt_Ps_Format(ps, "    %g %g translate\n", sw * -0.5, sh * 0.5);
  Blt_Ps_Format(ps, "    %g %g scale\n", sw, -sh);
  Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {", 
		width, height, width, -height, height);
  Blt_Ps_XSetBitmapData(ps, display, bitmap, width, height);
  Blt_Ps_Append(ps, "    } imagemask\n  grestore\n");
}

void Blt_Ps_Draw2DSegments(Blt_Ps ps, Segment2d *segments, int nSegments)
{
  Segment2d *sp, *send;

  Blt_Ps_Append(ps, "newpath\n");
  for (sp = segments, send = sp + nSegments; sp < send; sp++) {
    Blt_Ps_Format(ps, "  %g %g moveto %g %g lineto\n", 
		  sp->p.x, sp->p.y, sp->q.x, sp->q.y);
    Blt_Ps_Append(ps, "DashesProc stroke\n");
  }
}

