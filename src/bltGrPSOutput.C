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

#include <math.h>
#include <stdarg.h>

extern "C" {
#include <tkPort.h>
#include <tk3d.h>
};

#include "bltGraph.h"
#include "bltGrPageSetup.h"
#include "bltGrPs.h"

using namespace Blt;

#ifdef TCL_UTF_MAX
#  define HAVE_UTF	1
#else
#  define HAVE_UTF	0
#endif

PostScript::PostScript(Graph* graphPtr)
{
  graphPtr_ = graphPtr;

  Tcl_DStringInit(&dString);
}

PostScript::~PostScript()
{
  Tcl_DStringFree(&dString);
}

void PostScript::drawPolyline(Point2d* screenPts, int nScreenPts)
{
  Point2d* pp = screenPts;
  append("newpath\n");
  format("  %g %g moveto\n", pp->x, pp->y);

  Point2d* pend;
  for (pp++, pend = screenPts + nScreenPts; pp < pend; pp++)
    format("  %g %g lineto\n", pp->x, pp->y);
}

void PostScript::drawMaxPolyline(Point2d* points, int nPoints)
{
  if (nPoints <= 0)
    return;

  for (int nLeft = nPoints; nLeft > 0; nLeft -= 1500) {
    int length = MIN(1500, nLeft);
    drawPolyline(points, length);
    append("DashesProc stroke\n");
    points += length;
  }
}

void PostScript::drawSegments(Segment2d* segments, int nSegments)
{
  append("newpath\n");

  Segment2d* sp;
  Segment2d* send;
  for (sp = segments, send = sp + nSegments; sp < send; sp++) {
    format("  %g %g moveto %g %g lineto\n", sp->p.x, sp->p.y, sp->q.x, sp->q.y);
    append("DashesProc stroke\n");
  }
}

int PostScript::computeBBox(int width, int height)
{
  PageSetup* setupPtr = graphPtr_->pageSetup_;
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

  // If the paper size wasn't specified, set it to the graph size plus the
  // paper border.
  paperWidth = (pops->reqPaperWidth > 0) ? pops->reqPaperWidth :
    hSize + hBorder;
  paperHeight = (pops->reqPaperHeight > 0) ? pops->reqPaperHeight :
    vSize + vBorder;

  // Scale the plot size (the graph itself doesn't change size) if it's
  // bigger than the paper or if -maxpect was set.
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

const char* PostScript::getValue(int* lengthPtr)
{
  *lengthPtr = strlen(Tcl_DStringValue(&dString));
  return Tcl_DStringValue(&dString);
}

void PostScript::append(const char* string)
{
  Tcl_DStringAppend(&dString, string, -1);
}

void PostScript::format(const char* fmt, ...)
{
  va_list argList;

  va_start(argList, fmt);
  vsnprintf(scratchArr, POSTSCRIPT_BUFSIZ, fmt, argList);
  va_end(argList);
  Tcl_DStringAppend(&dString, scratchArr, -1);
}

void PostScript::varAppend(const char* fmt, ...)
{
  va_list argList;

  va_start(argList, fmt);
  for (;;) {
    char* str = va_arg(argList, char *);
    if (!str)
      break;
    Tcl_DStringAppend(&dString, str, -1);
  }
  va_end(argList);
}

void PostScript::setLineWidth(int lineWidth)
{
  if (lineWidth < 1)
    lineWidth = 1;
  format("%d setlinewidth\n", lineWidth);
}

void PostScript::drawRectangle(int x, int y, int width, int height)
{
  append("newpath\n");
  format("  %d %d moveto\n", x, y);
  format("  %d %d rlineto\n", width, 0);
  format("  %d %d rlineto\n", 0, height);
  format("  %d %d rlineto\n", -width, 0);
  append("closepath\n");
}

void PostScript::fillRectangle(double x, double y, int width, int height)
{
  drawRectangle((int)x, (int)y, width, height);
  append("fill\n");
}

void PostScript::fillRectangles(XRectangle* rectangles, int nRectangles)
{
  XRectangle *rp, *rend;
  for (rp = rectangles, rend = rp + nRectangles; rp < rend; rp++)
    fillRectangle((double)rp->x, (double)rp->y, (int)rp->width,(int)rp->height);
}

void PostScript::setBackground(XColor* colorPtr)
{
  PageSetupOptions* pops = (PageSetupOptions*)graphPtr_->pageSetup_->ops_;
  XColorToPostScript(colorPtr);
  append(" setrgbcolor\n");
  if (pops->greyscale)
    append(" currentgray setgray\n");
}

void PostScript::setForeground(XColor* colorPtr)
{
  PageSetupOptions* pops = (PageSetupOptions*)graphPtr_->pageSetup_->ops_;
  XColorToPostScript(colorPtr);
  append(" setrgbcolor\n");
  if (pops->greyscale)
    append(" currentgray setgray\n");
}

void PostScript::setFont(Tk_Font font) 
{
#if 0
  const char* family = FamilyToPsFamily(Blt_FamilyOfFont(font));
  if (family != NULL) {
    double pointSize;
	
    Tcl_DString dString;
    Tcl_DStringInit(&dString);
    pointSize = (double)Blt_PostscriptFontName(font, &dString);
    format("%g /%s SetFont\n", pointSize, Tcl_DStringValue(&dString));
    Tcl_DStringFree(&dString);
    return;
  }
  append("12.0 /Helvetica-Bold SetFont\n");
#endif
}

void PostScript::setLineAttributes(XColor* colorPtr,int lineWidth,
				   Dashes* dashesPtr, int capStyle, 
				   int joinStyle)
{
  setJoinStyle(joinStyle);
  setCapStyle(capStyle);
  setForeground(colorPtr);
  setLineWidth(lineWidth);
  setDashes(dashesPtr);
  append("/DashesProc {} def\n");
}

void PostScript::fill3DRectangle(Tk_3DBorder border, double x, double y,
				 int width, int height, int borderWidth, 
				 int relief)
{
  TkBorder* borderPtr = (TkBorder*)border;

  setBackground(borderPtr->bgColorPtr);
  fillRectangle(x, y, width, height);
  draw3DRectangle(border, x, y, width, height, borderWidth, relief);
}

void PostScript::setClearBackground()
{
  append("1 1 1 setrgbcolor\n");
}

void PostScript::setDashes(Dashes* dashesPtr)
{

  append("[ ");
  if (dashesPtr) {
    for (unsigned char* vp = dashesPtr->values; *vp != 0; vp++)
      format(" %d", *vp);
  }
  append("] 0 setdash\n");
}

void PostScript::fillPolygon(Point2d *screenPts, int nScreenPts)
{
  drawPolygon(screenPts, nScreenPts);
  append("fill\n");
}

void PostScript::drawBitmap(Display *display, Pixmap bitmap,
			    double xScale, double yScale)
{
  int width, height;
  Tk_SizeOfBitmap(display, bitmap, &width, &height);

  double sw = (double)width * xScale;
  double sh = (double)height * yScale;
  append("  gsave\n");
  format("    %g %g translate\n", sw * -0.5, sh * 0.5);
  format("    %g %g scale\n", sw, -sh);
  format("    %d %d true [%d 0 0 %d 0 %d] {", 
		width, height, width, -height, height);
  setBitmap(display, bitmap, width, height);
  append("    } imagemask\n  grestore\n");
}

void PostScript::setJoinStyle(int joinStyle)
{
  // miter = 0, round = 1, bevel = 2
  format("%d setlinejoin\n", joinStyle);
}

void PostScript::setCapStyle(int capStyle)
{
  // X11:not last = 0, butt = 1, round = 2, projecting = 3
  // PS: butt = 0, round = 1, projecting = 2
  if (capStyle > 0)
    capStyle--;

  format("%d setlinecap\n", capStyle);
}

void PostScript::drawPolygon(Point2d *screenPts, int nScreenPts)
{
  Point2d* pp = screenPts;
  append("newpath\n");
  format("  %g %g moveto\n", pp->x, pp->y);

  Point2d* pend;
  for (pp++, pend = screenPts + nScreenPts; pp < pend; pp++) 
    format("  %g %g lineto\n", pp->x, pp->y);

  format("  %g %g lineto\n", screenPts[0].x, screenPts[0].y);
  append("closepath\n");
}

void PostScript::draw3DRectangle(Tk_3DBorder border, double x, double y,
				 int width, int height, int borderWidth,
				 int relief)
{
  TkBorder* borderPtr = (TkBorder*)border;
  XColor* lightPtr, *darkPtr;
  XColor* topPtr, *bottomPtr;
  XColor light, dark;
  int twiceWidth = (borderWidth * 2);

  if ((width < twiceWidth) || (height < twiceWidth))
    return;

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
  }
  else {
    lightPtr = borderPtr->lightColorPtr;
    darkPtr = borderPtr->darkColorPtr;
  }

  // Handle grooves and ridges with recursive calls
  if ((relief == TK_RELIEF_GROOVE) || (relief == TK_RELIEF_RIDGE)) {
    int halfWidth = borderWidth / 2;
    int insideOffset = borderWidth - halfWidth;
    draw3DRectangle(border, (double)x, (double)y, width, height, halfWidth, 
		    (relief == TK_RELIEF_GROOVE) ? 
		    TK_RELIEF_SUNKEN : TK_RELIEF_RAISED);
    draw3DRectangle(border, (double)(x + insideOffset), 
		    (double)(y + insideOffset), width - insideOffset * 2, 
		    height - insideOffset * 2, halfWidth,
		    (relief == TK_RELIEF_GROOVE) ? 
		    TK_RELIEF_RAISED : TK_RELIEF_SUNKEN);
    return;
  }

  if (relief == TK_RELIEF_RAISED) {
    topPtr = lightPtr;
    bottomPtr = darkPtr;
  }
  else if (relief == TK_RELIEF_SUNKEN) {
    topPtr = darkPtr;
    bottomPtr = lightPtr;
  }
  else
    topPtr = bottomPtr = borderPtr->bgColorPtr;

  setBackground(bottomPtr);
  fillRectangle(x, y + height - borderWidth, width, borderWidth);
  fillRectangle(x + width - borderWidth, y, borderWidth, height);

  Point2d points[7];
  points[0].x = points[1].x = points[6].x = x;
  points[0].y = points[6].y = y + height;
  points[1].y = points[2].y = y;
  points[2].x = x + width;
  points[3].x = x + width - borderWidth;
  points[3].y = points[4].y = y + borderWidth;
  points[4].x = points[5].x = x + borderWidth;
  points[5].y = y + height - borderWidth;
  if (relief != TK_RELIEF_FLAT)
    setBackground(topPtr);

  fillPolygon(points, 7);
}

void PostScript::setBitmap(Display *display, Pixmap bitmap, int w, int h)
{
  XImage* imagePtr = XGetImage(display, bitmap, 0, 0, w, h, 1, ZPixmap);
  append("\t<");
  int byteCount =0;
  int bitPos =0;
  for (int y=0; y<h; y++) {
    char string[10];
    unsigned char byte = 0;
    for (int x=0; x<w; x++) {
      unsigned long pixel = XGetPixel(imagePtr, x, y);
      bitPos = x % 8;
      byte |= (unsigned char)(pixel << bitPos);
      if (bitPos == 7) {
	byte = reverseBits(byte);
	byteToHex(byte, string);
	string[2] = '\0';
	byteCount++;
	byte = 0;
	if (byteCount >= 30) {
	  string[2] = '\n';
	  string[3] = '\t';
	  string[4] = '\0';
	  byteCount = 0;
	}
	append(string);
      }
    }			/* x */
    if (bitPos != 7) {
      byte = reverseBits(byte);
      byteToHex(byte, string);
      string[2] = '\0';
      append(string);
      byteCount++;
    }
  }				/* y */
  append(">\n");
  XDestroyImage(imagePtr);
}

int PostScript::includeFile(const char *fileName)
{
  Tcl_Channel channel;
  Tcl_DString dString;
  int nBytes;

  Tcl_Interp* interp = graphPtr_->interp_;
  char* buf = scratchArr;

  // Read in a standard prolog file from file and append it to the
  // PostScript output stored in the Tcl_DString in psPtr.
  char* libDir = (char *)Tcl_GetVar(interp, "tkblt_library", TCL_GLOBAL_ONLY);
  if (libDir == NULL) {
    Tcl_AppendResult(interp, "couldn't find TKBLT script library:",
		     "global variable \"tkblt_library\" doesn't exist",
		     (char *)NULL);
    return TCL_ERROR;
  }
  Tcl_DStringInit(&dString);
  Tcl_DStringAppend(&dString, libDir, -1);
  Tcl_DStringAppend(&dString, "/", -1);
  Tcl_DStringAppend(&dString, fileName, -1);
  fileName = Tcl_DStringValue(&dString);
  varAppend("\n% including file \"", fileName, "\"\n\n", NULL);
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
    append(buf);
  }
  Tcl_DStringFree(&dString);
  Tcl_Close(interp, channel);
  return TCL_OK;
}

void PostScript::XColorToPostScript(XColor* colorPtr)
{
  format("%g %g %g",
		((double)(colorPtr->red >> 8) / 255.0),
		((double)(colorPtr->green >> 8) / 255.0),
		((double)(colorPtr->blue >> 8) / 255.0));
}

int PostScript::preamble(const char* fileName)
{
  PageSetup* setupPtr = graphPtr_->pageSetup_;
  PageSetupOptions* ops = (PageSetupOptions*)setupPtr->ops_;
  time_t ticks;
  char date[200];			/* Holds the date string from ctime() */
  char *newline;

  if (!fileName)
    fileName = Tk_PathName(graphPtr_->tkwin_);

  append("%!PS-Adobe-3.0 EPSF-3.0\n");

  // The "BoundingBox" comment is required for EPS files. The box
  // coordinates are integers, so we need round away from the center of the
  // box.
  format("%%%%BoundingBox: %d %d %d %d\n",
	 setupPtr->left, setupPtr->paperHeight - setupPtr->top,
	 setupPtr->right, setupPtr->paperHeight - setupPtr->bottom);
	
  append("%%Pages: 0\n");

  format("%%%%Creator: (%s %s %s)\n", PACKAGE_NAME, PACKAGE_VERSION,
	 Tk_Class(graphPtr_->tkwin_));

  ticks = time((time_t *) NULL);
  strcpy(date, ctime(&ticks));
  newline = date + strlen(date) - 1;
  if (*newline == '\n')
    *newline = '\0';

  format("%%%%CreationDate: (%s)\n", date);
  format("%%%%Title: (%s)\n", fileName);
  append("%%DocumentData: Clean7Bit\n");
  if (ops->landscape)
    append("%%Orientation: Landscape\n");
  else
    append("%%Orientation: Portrait\n");

  append("%%DocumentNeededResources: font Helvetica Courier\n");
  addComments(ops->comments);
  append("%%EndComments\n\n");
  if (includeFile("bltGraph.pro") != TCL_OK)
    return TCL_ERROR;

  if (ops->footer) {
    const char* who = getenv("LOGNAME");
    if (!who)
      who = "???";

    varAppend("8 /Helvetica SetFont\n",
		     "10 30 moveto\n",
		     "(Date: ", date, ") show\n",
		     "10 20 moveto\n",
		     "(File: ", fileName, ") show\n",
		     "10 10 moveto\n",
		     "(Created by: ", who, "@", Tcl_GetHostName(), ") show\n",
		     "0 0 moveto\n",
		     (char *)NULL);
  }

  // Set the conversion from PostScript to X11 coordinates.  Scale pica to
  // pixels and flip the y-axis (the origin is the upperleft corner).
  varAppend("% Transform coordinate system to use X11 coordinates\n\n",
		   "% 1. Flip y-axis over by reversing the scale,\n",
		   "% 2. Translate the origin to the other side of the page,\n",
		   "%    making the origin the upper left corner\n", NULL);
  format("1 -1 scale\n");

  // Papersize is in pixels. Translate the new origin *after* changing the scale
  format("0 %d translate\n\n", -setupPtr->paperHeight);
  varAppend("% User defined page layout\n\n", "% Set color level\n", NULL);
  format("%% Set origin\n%d %d translate\n\n", setupPtr->left,setupPtr->bottom);
  if (ops->landscape)
    format("%% Landscape orientation\n0 %g translate\n-90 rotate\n",
		  ((double)graphPtr_->width_ * setupPtr->scale));

  append("\n%%EndSetup\n\n");

  return TCL_OK;
}

void PostScript::addComments(const char** comments)
{
  for (const char** p = comments; *p; p += 2) {
    if (*(p+1) == NULL)
      break;
    format("%% %s: %s\n", *p, *(p+1));
  }
}

unsigned char PostScript::reverseBits(unsigned char byte)
{
  byte = ((byte >> 1) & 0x55) | ((byte << 1) & 0xaa);
  byte = ((byte >> 2) & 0x33) | ((byte << 2) & 0xcc);
  byte = ((byte >> 4) & 0x0f) | ((byte << 4) & 0xf0);
  return byte;
}

void PostScript::byteToHex(unsigned char byte, char* string)
{
  static char hexDigits[] = "0123456789ABCDEF";

  string[0] = hexDigits[byte >> 4];
  string[1] = hexDigits[byte & 0x0F];
}

void PostScript::drawText(const char* string, double x, double y)
{
  if (!string || !(*string))
    return;
}

