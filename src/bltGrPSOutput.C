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

#define PS_MAXPECT	(1<<4)

#ifdef TCL_UTF_MAX
#  define HAVE_UTF	1
#else
#  define HAVE_UTF	0
#endif

PostScript::PostScript(Graph* graphPtr)
{
  graphPtr_ = graphPtr;

  Tcl_DStringInit(&dString_);
}

PostScript::~PostScript()
{
  Tcl_DStringFree(&dString_);
}

void PostScript::printPolyline(Point2d* screenPts, int nScreenPts)
{
  Point2d* pp = screenPts;
  append("newpath\n");
  format("  %g %g moveto\n", pp->x, pp->y);

  Point2d* pend;
  for (pp++, pend = screenPts + nScreenPts; pp < pend; pp++)
    format("  %g %g lineto\n", pp->x, pp->y);
}

void PostScript::printMaxPolyline(Point2d* points, int nPoints)
{
  if (nPoints <= 0)
    return;

  for (int nLeft = nPoints; nLeft > 0; nLeft -= 1500) {
    int length = MIN(1500, nLeft);
    printPolyline(points, length);
    append("DashesProc stroke\n");
    points += length;
  }
}

void PostScript::printSegments(Segment2d* segments, int nSegments)
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
  *lengthPtr = strlen(Tcl_DStringValue(&dString_));
  return Tcl_DStringValue(&dString_);
}

void PostScript::append(const char* string)
{
  Tcl_DStringAppend(&dString_, string, -1);
}

void PostScript::format(const char* fmt, ...)
{
  va_list argList;

  va_start(argList, fmt);
  vsnprintf(scratchArr_, POSTSCRIPT_BUFSIZ, fmt, argList);
  va_end(argList);
  Tcl_DStringAppend(&dString_, scratchArr_, -1);
}

void PostScript::varAppend(const char* fmt, ...)
{
  va_list argList;

  va_start(argList, fmt);
  for (;;) {
    char* str = va_arg(argList, char *);
    if (!str)
      break;
    Tcl_DStringAppend(&dString_, str, -1);
  }
  va_end(argList);
}

void PostScript::setLineWidth(int lineWidth)
{
  if (lineWidth < 1)
    lineWidth = 1;
  format("%d setlinewidth\n", lineWidth);
}

void PostScript::printRectangle(int x, int y, int width, int height)
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
  printRectangle((int)x, (int)y, width, height);
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
  printXColor(colorPtr);
  append(" setrgbcolor\n");
  if (pops->greyscale)
    append(" currentgray setgray\n");
}

void PostScript::setForeground(XColor* colorPtr)
{
  PageSetupOptions* pops = (PageSetupOptions*)graphPtr_->pageSetup_->ops_;
  printXColor(colorPtr);
  append(" setrgbcolor\n");
  if (pops->greyscale)
    append(" currentgray setgray\n");
}

void PostScript::setFont(Tk_Font font) 
{
  Tcl_DString psdstr;
  Tcl_DStringInit(&psdstr);
  int psSize = Tk_PostscriptFontName(font, &psdstr);
  format("%d /%s SetFont\n", psSize, Tcl_DStringValue(&psdstr));
  Tcl_DStringFree(&psdstr);
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
  print3DRectangle(border, x, y, width, height, borderWidth, relief);
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
  printPolygon(screenPts, nScreenPts);
  append("fill\n");
}

void PostScript::printBitmap(Display *display, Pixmap bitmap,
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

void PostScript::printPolygon(Point2d *screenPts, int nScreenPts)
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

void PostScript::print3DRectangle(Tk_3DBorder border, double x, double y,
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
    print3DRectangle(border, (double)x, (double)y, width, height, halfWidth, 
		    (relief == TK_RELIEF_GROOVE) ? 
		    TK_RELIEF_SUNKEN : TK_RELIEF_RAISED);
    print3DRectangle(border, (double)(x + insideOffset), 
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

void PostScript::printXColor(XColor* colorPtr)
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

  prolog();

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
  if (!comments)
    return;

  for (const char** pp = comments; *pp; pp+=2) {
    if (*(pp+1) == NULL)
      break;
    format("%% %s: %s\n", *pp, *(pp+1));
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

void PostScript::printText(const char* string, double x, double y)
{
  if (!string || !(*string))
    return;
}

void PostScript::prolog()
{
  append(
"%%BeginProlog\n"
"%\n"
"% PostScript prolog file of the BLT graph widget.\n"
"%\n"
"% Copyright 1989-1992 Regents of the University of California.\n"
"% Permission to use, copy, modify, and distribute this\n"
"% software and its documentation for any purpose and without\n"
"% fee is hereby granted, provided that the above copyright\n"
"% notice appear in all copies.  The University of California\n"
"% makes no representations about the suitability of this\n"
"% software for any purpose.  It is provided 'as is' without\n"
"% express or implied warranty.\n"
"%\n"
"% Copyright 1991-1997 Bell Labs Innovations for Lucent Technologies.\n"
"%\n"
"% Permission to use, copy, modify, and distribute this software and its\n"
"% documentation for any purpose and without fee is hereby granted, provided\n"
"% that the above copyright notice appear in all copies and that both that the\n"
"% copyright notice and warranty disclaimer appear in supporting documentation,\n"
"% and that the names of Lucent Technologies any of their entities not be used\n"
"% in advertising or publicity pertaining to distribution of the software\n"
"% without specific, written prior permission.\n"
"%\n"
"% Lucent Technologies disclaims all warranties with regard to this software,\n"
"% including all implied warranties of merchantability and fitness.  In no event\n"
"% shall Lucent Technologies be liable for any special, indirect or\n"
"% consequential damages or any damages whatsoever resulting from loss of use,\n"
"% data or profits, whether in an action of contract, negligence or other\n"
"% tortuous action, arising out of or in connection with the use or performance\n"
"% of this software.\n"
"%\n"
"\n"
"200 dict begin\n"
"\n"
"/BaseRatio 1.3467736870885982 def	% Ratio triangle base / symbol size\n"
"/BgColorProc 0 def			% Background color routine (symbols)\n"
"/DrawSymbolProc 0 def			% Routine to draw symbol outline/fill\n"
"/StippleProc 0 def			% Stipple routine (bar segments)\n"
"/DashesProc 0 def			% Dashes routine (line segments)\n"
"\n"
"% Define the array ISOLatin1Encoding (which specifies how characters are \n"
"% encoded for ISO-8859-1 fonts), if it isn't already present (Postscript \n"
"% level 2 is supposed to define it, but level 1 doesn't). \n"
"\n"
"systemdict /ISOLatin1Encoding known not { \n"
"  /ISOLatin1Encoding [ \n"
"    /space /space /space /space /space /space /space /space \n"
"    /space /space /space /space /space /space /space /space \n"
"    /space /space /space /space /space /space /space /space \n"
"    /space /space /space /space /space /space /space /space \n"
"    /space /exclam /quotedbl /numbersign /dollar /percent /ampersand \n"
"    /quoteright \n"
"    /parenleft /parenright /asterisk /plus /comma /minus /period /slash \n"
"    /zero /one /two /three /four /five /six /seven \n"
"    /eight /nine /colon /semicolon /less /equal /greater /question \n"
"    /at /A /B /C /D /E /F /G \n"
"    /H /I /J /K /L /M /N /O \n"
"    /P /Q /R /S /T /U /V /W \n"
"    /X /Y /Z /bracketleft /backslash /bracketright /asciicircum /underscore \n"
"    /quoteleft /a /b /c /d /e /f /g \n"
"    /h /i /j /k /l /m /n /o \n"
"    /p /q /r /s /t /u /v /w \n"
"    /x /y /z /braceleft /bar /braceright /asciitilde /space \n"
"    /space /space /space /space /space /space /space /space \n"
"    /space /space /space /space /space /space /space /space \n"
"    /dotlessi /grave /acute /circumflex /tilde /macron /breve /dotaccent \n"
"    /dieresis /space /ring /cedilla /space /hungarumlaut /ogonek /caron \n"
"    /space /exclamdown /cent /sterling /currency /yen /brokenbar /section \n"
"    /dieresis /copyright /ordfeminine /guillemotleft /logicalnot /hyphen \n"
"    /registered /macron \n"
"    /degree /plusminus /twosuperior /threesuperior /acute /mu /paragraph \n"
"    /periodcentered \n"
"    /cedillar /onesuperior /ordmasculine /guillemotright /onequarter \n"
"    /onehalf /threequarters /questiondown \n"
"    /Agrave /Aacute /Acircumflex /Atilde /Adieresis /Aring /AE /Ccedilla \n"
"    /Egrave /Eacute /Ecircumflex /Edieresis /Igrave /Iacute /Icircumflex \n"
"    /Idieresis \n"
"    /Eth /Ntilde /Ograve /Oacute /Ocircumflex /Otilde /Odieresis /multiply \n"
"    /Oslash /Ugrave /Uacute /Ucircumflex /Udieresis /Yacute /Thorn \n"
"    /germandbls \n"
"    /agrave /aacute /acircumflex /atilde /adieresis /aring /ae /ccedilla \n"
"    /egrave /eacute /ecircumflex /edieresis /igrave /iacute /icircumflex \n"
"    /idieresis \n"
"    /eth /ntilde /ograve /oacute /ocircumflex /otilde /odieresis /divide \n"
"    /oslash /ugrave /uacute /ucircumflex /udieresis /yacute /thorn \n"
"    /ydieresis \n"
"  ] def \n"
"} if \n"
"\n"
"% font ISOEncode font \n"
"% This procedure changes the encoding of a font from the default \n"
"% Postscript encoding to ISOLatin1.  It is typically invoked just \n"
"% before invoking 'setfont'.  The body of this procedure comes from \n"
"% Section 5.6.1 of the Postscript book. \n"
"\n"
"/ISOEncode { \n"
"  dup length dict\n"
"  begin \n"
"    {1 index /FID ne {def} {pop pop} ifelse} forall \n"
"    /Encoding ISOLatin1Encoding def \n"
"    currentdict \n"
"  end \n"
"\n"
"  % I'm not sure why it's necessary to use 'definefont' on this new \n"
"  % font, but it seems to be important; just use the name 'Temporary' \n"
"  % for the font. \n"
"\n"
"  /Temporary exch definefont \n"
"} bind def \n"
"\n"
"/Stroke {\n"
"  gsave\n"
"    stroke\n"
"  grestore\n"
"} def\n"
"\n"
"/Fill {\n"
"  gsave\n"
"    fill\n"
"  grestore\n"
"} def\n"
"\n"
"/SetFont { 	\n"
"  % Stack: pointSize fontName\n"
"  findfont exch scalefont ISOEncode setfont\n"
"} def\n"
"\n"
"/Box {\n"
"  % Stack: x y width height\n"
"  newpath\n"
"    exch 4 2 roll moveto\n"
"    dup 0 rlineto\n"
"    exch 0 exch rlineto\n"
"    neg 0 rlineto\n"
"  closepath\n"
"} def\n"
"\n"
"% The next two definitions are taken from '$tk_library/prolog.ps'\n"
"\n"
"% desiredSize EvenPixels closestSize\n"
"%\n"
"% The procedure below is used for stippling.  Given the optimal size\n"
"% of a dot in a stipple pattern in the current user coordinate system,\n"
"% compute the closest size that is an exact multiple of the device's\n"
"% pixel size.  This allows stipple patterns to be displayed without\n"
"% aliasing effects.\n"
"\n"
"/EvenPixels {\n"
"  % Compute exact number of device pixels per stipple dot.\n"
"  dup 0 matrix currentmatrix dtransform\n"
"  dup mul exch dup mul add sqrt\n"
"\n"
"  % Round to an integer, make sure the number is at least 1, and compute\n"
"  % user coord distance corresponding to this.\n"
"  dup round dup 1 lt {pop 1} if\n"
"  exch div mul\n"
"} bind def\n"
"\n"
"% width height string filled StippleFill --\n"
"%\n"
"% Given a path and other graphics information already set up, this\n"
"% procedure will fill the current path in a stippled fashion.  'String'\n"
"% contains a proper image description of the stipple pattern and\n"
"% 'width' and 'height' give its dimensions.  If 'filled' is true then\n"
"% it means that the area to be stippled is gotten by filling the\n"
"% current path (e.g. the interior of a polygon); if it's false, the\n"
"% area is gotten by stroking the current path (e.g. a wide line).\n"
"% Each stipple dot is assumed to be about one unit across in the\n"
"% current user coordinate system.\n"
"\n"
"% width height string StippleFill --\n"
"%\n"
"% Given a path already set up and a clipping region generated from\n"
"% it, this procedure will fill the clipping region with a stipple\n"
"% pattern.  'String' contains a proper image description of the\n"
"% stipple pattern and 'width' and 'height' give its dimensions.  Each\n"
"% stipple dot is assumed to be about one unit across in the current\n"
"% user coordinate system.  This procedure trashes the graphics state.\n"
"\n"
"/StippleFill {\n"
"    % The following code is needed to work around a NeWSprint bug.\n"
"\n"
"    /tmpstip 1 index def\n"
"\n"
"    % Change the scaling so that one user unit in user coordinates\n"
"    % corresponds to the size of one stipple dot.\n"
"    1 EvenPixels dup scale\n"
"\n"
"    % Compute the bounding box occupied by the path (which is now\n"
"    % the clipping region), and round the lower coordinates down\n"
"    % to the nearest starting point for the stipple pattern.  Be\n"
"    % careful about negative numbers, since the rounding works\n"
"    % differently on them.\n"
"\n"
"    pathbbox\n"
"    4 2 roll\n"
"    5 index div dup 0 lt {1 sub} if cvi 5 index mul 4 1 roll\n"
"    6 index div dup 0 lt {1 sub} if cvi 6 index mul 3 2 roll\n"
"\n"
"    % Stack now: width height string y1 y2 x1 x2\n"
"    % Below is a doubly-nested for loop to iterate across this area\n"
"    % in units of the stipple pattern size, going up columns then\n"
"    % across rows, blasting out a stipple-pattern-sized rectangle at\n"
"    % each position\n"
"\n"
"    6 index exch {\n"
"	2 index 5 index 3 index {\n"
"	    % Stack now: width height string y1 y2 x y\n"
"\n"
"	    gsave\n"
"	    1 index exch translate\n"
"	    5 index 5 index true matrix tmpstip imagemask\n"
"	    grestore\n"
"	} for\n"
"	pop\n"
"    } for\n"
"    pop pop pop pop pop\n"
"} bind def\n"
"\n"
"\n"
"/LS {	% Stack: x1 y1 x2 y2\n"
"  newpath \n"
"    4 2 roll moveto \n"
"    lineto \n"
"  closepath\n"
"  stroke\n"
"} def\n"
"\n"
"/baseline 0 def\n"
"/height 0 def\n"
"/justify 0 def\n"
"/lineLength 0 def\n"
"/spacing 0 def\n"
"/strings 0 def\n"
"/xoffset 0 def\n"
"/yoffset 0 def\n"
"/baselineSampler ( TXygqPZ) def\n"
"% Put an extra-tall character in; done this way to avoid encoding trouble\n"
"baselineSampler 0 196 put\n"
"\n"
"/cstringshow {\n"
"  {\n"
"    dup type /stringtype eq\n"
"    gsave\n"
"    1 -1 scale\n"
"    { show } { glyphshow }\n"
"    ifelse\n"
"    grestore\n"
"  } forall\n"
"} bind def\n"
"\n"
"/cstringwidth {\n"
"  0 exch 0 exch\n"
"  {\n"
"    dup type /stringtype eq\n"
"    { stringwidth } {\n"
"      currentfont /Encoding get exch 1 exch put (\001)\n"
"      stringwidth\n"
"    }\n"
"    ifelse\n"
"    exch 3 1 roll add 3 1 roll add exch\n"
"  } forall\n"
"} bind def\n" 
"\n"
"/DrawText {\n"
"  gsave\n"
"  /justify exch def\n"
"  /yoffset exch def\n"
"  /xoffset exch def\n"
"  /spacing exch def\n"
"  /strings exch def\n"
"  % First scan through all of the text to find the widest line.\n"
"  /lineLength 0 def\n"
"  strings {\n"
"    cstringwidth pop\n"
"    dup lineLength gt {/lineLength exch def} {pop} ifelse\n"
"    newpath\n"
"  } forall\n"
"  % Compute the baseline offset and the actual font height.\n"
"  0 0 moveto baselineSampler false charpath\n"
"  pathbbox dup /baseline exch def\n"
"  exch pop exch sub /height exch def pop\n"
"  newpath\n"
"  % Translate and rotate coordinates first so that the origin is at\n"
"  % the upper-left corner of the text's bounding box. Remember that\n"
"  % angle for rotating, and x and y for positioning are still on the\n"
"  % stack.\n"
"  translate\n"
"  neg rotate\n"
"  lineLength xoffset mul\n"
"  strings length 1 sub spacing mul height add yoffset mul translate\n"
"  % Now use the baseline and justification information to translate\n"
"  % so that the origin is at the baseline and positioning point for\n"
"  % the first line of text.\n"
"  justify lineLength mul baseline neg translate\n"
"  % Iterate over each of the lines to output it.  For each line,\n"
"  % compute its width again so it can be properly justified, then\n"
"  % display it.\n"
"  strings {\n"
"    dup cstringwidth pop\n"
"    justify neg mul 0 moveto\n"
"    cstringshow\n"
"    0 spacing neg translate\n"
"  } forall\n"
"  grestore\n"
"} bind def \n"
"\n"
"/DrawBitmap {\n"
"  % Stack: ?bgColorProc? boolean centerX centerY width height theta imageStr\n"
"  gsave\n"
"    6 -2 roll translate			% Translate to center of bounding box\n"
"    4 1 roll neg rotate			% Rotate by theta\n"
"    \n"
"    % Find upperleft corner of bounding box\n"
"    \n"
"    2 copy -.5 mul exch -.5 mul exch translate\n"
"    2 copy scale			% Make pixel unit scale\n"
"    newpath\n"
"      0 0 moveto \n"
"      0 1 lineto \n"
"      1 1 lineto \n"
"      1 0 lineto\n"
"    closepath\n"
"    \n"
"    % Fill rectangle with background color\n"
"    \n"
"    4 -1 roll { \n"
"      gsave \n"
"	4 -1 roll exec fill \n"
"      grestore \n"
"    } if\n"
"    \n"
"    % Paint the image string into the unit rectangle\n"
"    \n"
"    2 copy true 3 -1 roll 0 0 5 -1 roll 0 0 6 array astore 5 -1 roll\n"
"    imagemask\n"
"  grestore\n"
"} def\n"
"\n"
"% Symbols:\n"
"\n"
"% Skinny-cross\n"
"/Sc {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 -2 roll translate 45 rotate\n"
"    0 0 3 -1 roll Sp\n"
"  grestore\n"
"} def\n"
"\n"
"% Skinny-plus\n"
"/Sp {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 -2 roll translate\n"
"    2 idiv\n"
"    dup 2 copy\n"
"    newpath \n"
"      neg 0 \n"
"      moveto 0 \n"
"      lineto\n"
"    DrawSymbolProc\n"
"    newpath \n"
"      neg 0 \n"
"      exch moveto 0 \n"
"      exch lineto\n"
"    DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"% Cross\n"
"/Cr {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 -2 roll translate 45 rotate\n"
"    0 0 3 -1 roll Pl\n"
"  grestore\n"
"} def\n"
"\n"
"% Plus\n"
"/Pl {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 -2 roll translate\n"
"    dup 2 idiv\n"
"    exch 6 idiv\n"
"\n"
"    %\n"
"    %          2   3		The plus/cross symbol is a\n"
"    %				closed polygon of 12 points.\n"
"    %      0   1   4    5	The diagram to the left\n"
"    %           x,y		represents the positions of\n"
"    %     11  10   7    6	the points which are computed\n"
"    %				below.\n"
"    %          9   8\n"
"    %\n"
"\n"
"    newpath\n"
"      2 copy exch neg exch neg moveto \n"
"      dup neg dup lineto\n"
"      2 copy neg exch neg lineto\n"
"      2 copy exch neg lineto\n"
"      dup dup neg lineto \n"
"      2 copy neg lineto 2 copy lineto\n"
"      dup dup lineto \n"
"      2 copy exch lineto \n"
"      2 copy neg exch lineto\n"
"      dup dup neg exch lineto \n"
"      exch neg exch lineto\n"
"    closepath\n"
"    DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"% Circle\n"
"/Ci {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 copy pop moveto \n"
"    newpath\n"
"      2 div 0 360 arc\n"
"    closepath \n"
"    DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"% Square\n"
"/Sq {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    dup dup 2 div dup\n"
"    6 -1 roll exch sub exch\n"
"    5 -1 roll exch sub 4 -2 roll Box\n"
"    DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"% Line\n"
"/Li {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 1 roll exch 3 -1 roll 2 div 3 copy\n"
"    newpath\n"
"      sub exch moveto \n"
"      add exch lineto\n"
"    closepath\n"
"    stroke\n"
"  grestore\n"
"} def\n"
"\n"
"% Diamond\n"
"/Di {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 1 roll translate 45 rotate 0 0 3 -1 roll Sq\n"
"  grestore\n"
"} def\n"
"    \n"
"% Triangle\n"
"/Tr {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 -2 roll translate\n"
"    BaseRatio mul 0.5 mul		% Calculate 1/2 base\n"
"    dup 0 exch 30 cos mul		% h1 = height above center point\n"
"    neg				% b2 0 -h1\n"
"    newpath \n"
"      moveto				% point 1;  b2\n"
"      dup 30 sin 30 cos div mul	% h2 = height below center point\n"
"      2 copy lineto			% point 2;  b2 h2\n"
"      exch neg exch lineto		% \n"
"    closepath\n"
"    DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"% Arrow\n"
"/Ar {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 -2 roll translate\n"
"    BaseRatio mul 0.5 mul		% Calculate 1/2 base\n"
"    dup 0 exch 30 cos mul		% h1 = height above center point\n"
"					% b2 0 h1\n"
"    newpath moveto			% point 1;  b2\n"
"    dup 30 sin 30 cos div mul		% h2 = height below center point\n"
"    neg				% -h2 b2\n"
"    2 copy lineto			% point 2;  b2 h2\n"
"    exch neg exch lineto		% \n"
"    closepath\n"
"    DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"% Bitmap\n"
"/Bm {\n"
"  % Stack: x y symbolSize\n"
"  gsave\n"
"    3 1 roll translate pop DrawSymbolProc\n"
"  grestore\n"
"} def\n"
"\n"
"%%EndProlog\n"
"\n"
"%%BeginSetup\n"
"gsave					% Save the graphics state\n"
"\n"
"% Default line/text style parameters\n"
"\n"
"1 setlinewidth				% width\n"
"1 setlinejoin				% join\n"
"0 setlinecap				% cap\n"
"[] 0 setdash				% dashes\n"
"\n"
"0 0 0 setrgbcolor			% color\n"
);
}
