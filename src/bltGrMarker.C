/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 * bltGrMarker.c --
 *
 * This module implements markers for the BLT graph widget.
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
#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltGrElem.h"
#include "bltBitmap.h"
#include "bltConfig.h"
#include "bltGrMarker.h"
#include "bltGrMarkerBitmap.h"

typedef int (GraphMarkerProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			      Tcl_Obj* const objv[]);

#define NORMALIZE(A,x) 	(((x) - (A)->axisRange.min) * (A)->axisRange.scale)

static Blt_OptionParseProc ObjToCoordsProc;
static Blt_OptionPrintProc CoordsToObjProc;
static Blt_OptionFreeProc FreeCoordsProc;
Blt_CustomOption coordsOption =
  {
    ObjToCoordsProc, CoordsToObjProc, FreeCoordsProc, NULL
  };

static Blt_OptionFreeProc FreeColorPairProc;
static Blt_OptionParseProc ObjToColorPairProc;
static Blt_OptionPrintProc ColorPairToObjProc;
Blt_CustomOption colorPairOption =
  {
    ObjToColorPairProc, ColorPairToObjProc, FreeColorPairProc, (ClientData)0
  };

static Tcl_FreeProc FreeMarker;

typedef struct {
  GraphObj obj;			/* Must be first field in marker. */

  MarkerClass *classPtr;

  Tcl_HashEntry *hashPtr;

  Blt_ChainLink link;

  const char* elemName;		/* Element associated with marker. Let's
				 * you link a marker to an element. The
				 * marker is drawn only if the element
				 * is also visible. */
  Axis2d axes;

  Point2d *worldPts;			/* Coordinate array to position
					 * marker. */

  int nWorldPts;			/* Number of points in above array */

  int drawUnder;			/* If non-zero, draw the marker
					 * underneath any elements. This can be
					 * a performance penalty because the
					 * graph must be redraw entirely each
					 * time the marker is redrawn. */

  int clipped;			/* Indicates if the marker is totally
				 * clipped by the plotting area. */

  int hide;
  unsigned int flags;		


  int xOffset, yOffset;		/* Pixel offset from graph position */

  int state;

  XColor* fillColor;
  XColor* outlineColor;		/* Foreground and background colors */

  int lineWidth;			/* Line width. */
  int capStyle;			/* Cap style. */
  int joinStyle;			/* Join style.*/
  Blt_Dashes dashes;			/* Dash list values (max 11) */

  GC gc;				/* Private graphic context */

  Segment2d *segments;		/* Malloc'ed array of points.
				 * Represents individual line segments
				 * (2 points per segment) comprising the
				 * mapped line.  The segments may not
				 * necessarily be connected after
				 * clipping. */
  int nSegments;			/* # segments in the above array. */
  int xor;
  int xorState;			/* State of the XOR drawing. Indicates
				 * if the marker is currently drawn. */
} LineMarker;

static Blt_ConfigSpec lineConfigSpecs[] = {
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", "Line all", 
   Tk_Offset(LineMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", "butt", 
   Tk_Offset(LineMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", NULL, 
   Tk_Offset(LineMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
  {BLT_CONFIG_CUSTOM, "-dashes", "dashes", "Dashes", NULL, 
   Tk_Offset(LineMarker, dashes), BLT_CONFIG_NULL_OK, &dashesOption},
  {BLT_CONFIG_PIXELS, "-dashoffset", "dashOffset", "DashOffset",
   "0", Tk_Offset(LineMarker, dashes.offset),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-element", "element", "Element", NULL, 
   Tk_Offset(LineMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-fill", "fill", "Fill", (char*)NULL, 
   Tk_Offset(LineMarker, fillColor), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", "miter", 
   Tk_Offset(LineMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", Tk_Offset(LineMarker, lineWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", "no", 
   Tk_Offset(LineMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", "x", 
   Tk_Offset(LineMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", "y", 
   Tk_Offset(LineMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, NULL, 
   Tk_Offset(LineMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-outline", "outline", "Outline",
   "black", Tk_Offset(LineMarker, outlineColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", "normal", 
   Tk_Offset(LineMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", "no", 
   Tk_Offset(LineMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", "0", 
   Tk_Offset(LineMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", "no", 
   Tk_Offset(LineMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", "0", 
   Tk_Offset(LineMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc ConfigureLineProc;
static MarkerCreateProc CreateLineProc;
static MarkerDrawProc DrawLineProc;
static MarkerFreeProc FreeLineProc;
static MarkerMapProc MapLineProc;
static MarkerPointProc PointInLineProc;
static MarkerPostscriptProc LineToPostscriptProc;
static MarkerRegionProc RegionInLineProc;

static MarkerClass lineMarkerClass = {
  lineConfigSpecs,
  ConfigureLineProc,
  DrawLineProc,
  FreeLineProc,
  MapLineProc,
  PointInLineProc,
  RegionInLineProc,
  LineToPostscriptProc,
};

typedef struct {
  GraphObj obj;			/* Must be first field in marker. */

  MarkerClass *classPtr;

  Tcl_HashEntry *hashPtr;

  Blt_ChainLink link;

  const char* elemName;		/* Element associated with marker. Let's
				 * you link a marker to an element. The
				 * marker is drawn only if the element
				 * is also visible. */
  Axis2d axes;

  Point2d *worldPts;			/* Coordinate array to position
					 * marker. */

  int nWorldPts;			/* Number of points in above array */

  int drawUnder;			/* If non-zero, draw the marker
					 * underneath any elements. This can be
					 * a performance penalty because the
					 * graph must be redraw entirely each
					 * time the marker is redrawn. */

  int clipped;			/* Indicates if the marker is totally
				 * clipped by the plotting area. */

  int hide;
  unsigned int flags;		


  int xOffset, yOffset;		/* Pixel offset from graph position */

  int state;

  Point2d *screenPts;			/* Array of points representing the
					 * polygon in screen coordinates. It's
					 * not used for drawing, but to generate
					 * the outlinePts and fillPts arrays
					 * that are the coordinates of the
					 * possibly clipped outline and filled
					 * polygon. */

  ColorPair outline;
  ColorPair fill;

  Pixmap stipple;			/* Stipple pattern to fill the
					 * polygon. */
  int lineWidth;			/* Width of polygon outline. */
  int capStyle;
  int joinStyle;
  Blt_Dashes dashes;			/* List of dash values.  Indicates how
					 * to draw the dashed line.  If no dash
					 * values are provided, or the first
					 * value is zero, then the line is drawn
					 * solid. */

  GC outlineGC;			/* Graphics context to draw the outline
				 * of the polygon. */
  GC fillGC;				/* Graphics context to draw the filled
					 * polygon. */

  Point2d *fillPts;			/* Malloc'ed array of points used to
					 * draw the filled polygon. These points
					 * may form a degenerate polygon after
					 * clipping. */
  int nFillPts;			/* # points in the above array. */
  Segment2d *outlinePts;		/* Malloc'ed array of points.
					 * Represents individual line segments
					 * (2 points per segment) comprising the
					 * outline of the polygon.  The segments
					 * may not necessarily be closed or
					 * connected after clipping. */
  int nOutlinePts;			/* # points in the above array. */
  int xor;
  int xorState;			/* State of the XOR drawing. Indicates
				 * if the marker is visible. We have to
				 * drawn it again to erase it. */
} PolygonMarker;


static Blt_ConfigSpec polygonConfigSpecs[] = {
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", "Polygon all", 
   Tk_Offset(PolygonMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", "butt", 
   Tk_Offset(PolygonMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", NULL, 
   Tk_Offset(PolygonMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
  {BLT_CONFIG_CUSTOM, "-dashes", "dashes", "Dashes", NULL, 
   Tk_Offset(PolygonMarker, dashes), BLT_CONFIG_NULL_OK, &dashesOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element", NULL, 
   Tk_Offset(PolygonMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", "rred", 
   Tk_Offset(PolygonMarker, fill), BLT_CONFIG_NULL_OK, &colorPairOption},
  {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", "miter", 
   Tk_Offset(PolygonMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   "1", Tk_Offset(PolygonMarker, lineWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", "no", 
   Tk_Offset(PolygonMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", "x", 
   Tk_Offset(PolygonMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", "y", 
   Tk_Offset(PolygonMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, NULL, 
   Tk_Offset(PolygonMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", 
   "black", Tk_Offset(PolygonMarker, outline),
   BLT_CONFIG_NULL_OK, &colorPairOption},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", "normal", 
   Tk_Offset(PolygonMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BITMAP, "-stipple", "stipple", "Stipple", NULL, 
   Tk_Offset(PolygonMarker, stipple), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", "no", 
   Tk_Offset(PolygonMarker, drawUnder), 
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", "0", 
   Tk_Offset(PolygonMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", "no", 
   Tk_Offset(PolygonMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", "0", 
   Tk_Offset(PolygonMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc ConfigurePolygonProc;
static MarkerCreateProc CreatePolygonProc;
static MarkerDrawProc DrawPolygonProc;
static MarkerFreeProc FreePolygonProc;
static MarkerMapProc MapPolygonProc;
static MarkerPointProc PointInPolygonProc;
static MarkerPostscriptProc PolygonToPostscriptProc;
static MarkerRegionProc RegionInPolygonProc;

static MarkerClass polygonMarkerClass = {
  polygonConfigSpecs,
  ConfigurePolygonProc,
  DrawPolygonProc,
  FreePolygonProc,
  MapPolygonProc,
  PointInPolygonProc,
  RegionInPolygonProc,
  PolygonToPostscriptProc,
};

typedef struct {
  GraphObj obj;			/* Must be first field in marker. */
  MarkerClass *classPtr;
  Tcl_HashEntry *hashPtr;
  Blt_ChainLink link;
  const char* elemName;		/* Element associated with marker. Let's
				 * you link a marker to an element. The
				 * marker is drawn only if the element
				 * is also visible. */
  Axis2d axes;
  Point2d *worldPts;			/* Coordinate array to position
					 * marker. */
  int nWorldPts;			/* # of points in above array */
  int drawUnder;			/* If non-zero, draw the marker
					 * underneath any elements. This can be
					 * a performance penalty because the
					 * graph must be redraw entirely each
					 * time the marker is redrawn. */
  int clipped;			/* Indicates if the marker is totally
				 * clipped by the plotting area. */
  int hide;
  unsigned int flags;		


  int xOffset, yOffset;		/* Pixel offset from graph position */
  int state;

  /* Fields specific to text markers. */
  const char* string;			/* Text string to be display.  The
					 * string make contain newlines. */
  Tk_Anchor anchor;			/* Indicates how to translate the given
					 * marker position. */
  Point2d anchorPt;			/* Translated anchor point. */
  int width, height;			/* Dimension of bounding box. */
  TextStyle style;			/* Text attributes (font, fg, anchor,
					 * etc) */
  Point2d outline[5];
  XColor* fillColor;
  GC fillGC;
} TextMarker;

static Blt_ConfigSpec textConfigSpecs[] = {
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", "center", 
   Tk_Offset(TextMarker, anchor), 0},
  {BLT_CONFIG_COLOR, "-background", "background", "MarkerBackground",
   (char*)NULL, Tk_Offset(TextMarker, fillColor), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-bg", "background", "Background", (char*)NULL, 0, 0},
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", "Text all",
   Tk_Offset(TextMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", NULL, 
   Tk_Offset(TextMarker, worldPts), BLT_CONFIG_NULL_OK, 
   &coordsOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element",
   NULL, Tk_Offset(TextMarker, elemName), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-fg", "foreground", "Foreground", (char*)NULL, 0, 0},
  {BLT_CONFIG_SYNONYM, "-fill", "background", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_FONT, "-font", "font", "Font", STD_FONT_NORMAL, 
   Tk_Offset(TextMarker, style.font), 0},
  {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
   "black", Tk_Offset(TextMarker, style.color), 0},
  {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
   "left", Tk_Offset(TextMarker, style.justify),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", "no", 
   Tk_Offset(TextMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", "x", 
   Tk_Offset(TextMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", "y", 
   Tk_Offset(TextMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, NULL, 
   Tk_Offset(TextMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-outline", "foreground", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_PIXELS, "-padx", "padX", "PadX", "4", 
   Tk_Offset(TextMarker, style.xPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-pady", "padY", "PadY", "4", 
   Tk_Offset(TextMarker, style.yPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_DOUBLE, "-rotate", "rotate", "Rotate", "0", 
   Tk_Offset(TextMarker, style.angle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", "normal", 
   Tk_Offset(TextMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_STRING, "-text", "text", "Text", NULL, 
   Tk_Offset(TextMarker, string), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", "no", 
   Tk_Offset(TextMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", "0", 
   Tk_Offset(TextMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", "0", 
   Tk_Offset(TextMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc ConfigureTextProc;
static MarkerCreateProc CreateTextProc;
static MarkerDrawProc DrawTextProc;
static MarkerFreeProc FreeTextProc;
static MarkerMapProc MapTextProc;
static MarkerPointProc PointInTextProc;
static MarkerPostscriptProc TextToPostscriptProc;
static MarkerRegionProc RegionInTextProc;

static MarkerClass textMarkerClass = {
  textConfigSpecs,
  ConfigureTextProc,
  DrawTextProc,
  FreeTextProc,
  MapTextProc,
  PointInTextProc,
  RegionInTextProc,
  TextToPostscriptProc,
};

typedef struct {
  GraphObj obj;			/* Must be first field in marker. */

  MarkerClass *classPtr;

  Tcl_HashEntry *hashPtr;

  Blt_ChainLink link;

  const char* elemName;		/* Element associated with marker. Let's
				 * you link a marker to an element. The
				 * marker is drawn only if the element
				 * is also visible. */
  Axis2d axes;

  Point2d *worldPts;			/* Coordinate array to position
					 * marker */

  int nWorldPts;			/* # of points in above array */

  int drawUnder;			/* If non-zero, draw the marker
					 * underneath any elements. This can be
					 * a performance penalty because the
					 * graph must be redraw entirely each
					 * time the marker is redrawn. */

  int clipped;			/* Indicates if the marker is totally
				 * clipped by the plotting area. */

  int hide;
  unsigned int flags;		


  int xOffset, yOffset;		/* Pixel offset from graph position */

  int state;

  /* Fields specific to window markers. */

  const char* childName;		/* Name of child widget. */
  Tk_Window child;			/* Window to display. */
  int reqWidth, reqHeight;		/* If non-zero, this overrides the size
					 * requested by the child widget. */

  Tk_Anchor anchor;			/* Indicates how to translate the given
					 * marker position. */

  Point2d anchorPt;			/* Translated anchor point. */
  int width, height;			/* Current size of the child window. */

} WindowMarker;

static Blt_ConfigSpec windowConfigSpecs[] = {
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", "center", 
   Tk_Offset(WindowMarker, anchor), 0},
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", "Window all", 
   Tk_Offset(WindowMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", NULL, 
   Tk_Offset(WindowMarker, worldPts), BLT_CONFIG_NULL_OK, 
   &coordsOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element", NULL, 
   Tk_Offset(WindowMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_PIXELS, "-height", "height", "Height", "0", 
   Tk_Offset(WindowMarker, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", "no", 
   Tk_Offset(WindowMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", "x", 
   Tk_Offset(WindowMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", "y", 
   Tk_Offset(WindowMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, NULL, 
   Tk_Offset(WindowMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", "normal", 
   Tk_Offset(WindowMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", "no", 
   Tk_Offset(WindowMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-width", "width", "Width", "0", 
   Tk_Offset(WindowMarker, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-window", "window", "Window", NULL, 
   Tk_Offset(WindowMarker, childName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", "0", 
   Tk_Offset(WindowMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", "0", 
   Tk_Offset(WindowMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc ConfigureWindowProc;
static MarkerCreateProc CreateWindowProc;
static MarkerDrawProc DrawWindowProc;
static MarkerFreeProc FreeWindowProc;
static MarkerMapProc MapWindowProc;
static MarkerPointProc PointInWindowProc;
static MarkerPostscriptProc WindowToPostscriptProc;
static MarkerRegionProc RegionInWindowProc;

static MarkerClass windowMarkerClass = {
  windowConfigSpecs,
  ConfigureWindowProc,
  DrawWindowProc,
  FreeWindowProc,
  MapWindowProc,
  PointInWindowProc,
  RegionInWindowProc,
  WindowToPostscriptProc,
};

int Blt_BoxesDontOverlap(Graph* graphPtr, Region2d *extsPtr)
{
  return (((double)graphPtr->right < extsPtr->left) ||
	  ((double)graphPtr->bottom < extsPtr->top) ||
	  (extsPtr->right < (double)graphPtr->left) ||
	  (extsPtr->bottom < (double)graphPtr->top));
}

static int GetCoordinate(Tcl_Interp* interp, Tcl_Obj *objPtr, double *valuePtr)
{
  char c;
  const char* expr;
    
  expr = Tcl_GetString(objPtr);
  c = expr[0];
  if ((c == 'I') && (strcmp(expr, "Inf") == 0)) {
    *valuePtr = DBL_MAX;		/* Elastic upper bound */
  } else if ((c == '-') && (expr[1] == 'I') && (strcmp(expr, "-Inf") == 0)) {
    *valuePtr = -DBL_MAX;		/* Elastic lower bound */
  } else if ((c == '+') && (expr[1] == 'I') && (strcmp(expr, "+Inf") == 0)) {
    *valuePtr = DBL_MAX;		/* Elastic upper bound */
  } else if (Blt_ExprDoubleFromObj(interp, objPtr, valuePtr) != TCL_OK) {
    return TCL_ERROR;
  }
  return TCL_OK;
}

static Tcl_Obj* PrintCoordinate(double x)
{
  if (x == DBL_MAX) {
    return Tcl_NewStringObj("+Inf", -1);
  } else if (x == -DBL_MAX) {
    return Tcl_NewStringObj("-Inf", -1);
  } else {
    return Tcl_NewDoubleObj(x);
  }
}

static int ParseCoordinates(Tcl_Interp* interp, Marker *markerPtr,
			    int objc, Tcl_Obj* const objv[])
{
  int nWorldPts;
  int minArgs, maxArgs;
  Point2d *worldPts;
  int i;

  if (objc == 0) {
    return TCL_OK;
  }
  if (objc & 1) {
    Tcl_AppendResult(interp, "odd number of marker coordinates specified",
		     (char*)NULL);
    return TCL_ERROR;
  }
  switch (markerPtr->obj.classId) {
  case CID_MARKER_LINE:
    minArgs = 4, maxArgs = 0;
    break;
  case CID_MARKER_POLYGON:
    minArgs = 6, maxArgs = 0;
    break;
  case CID_MARKER_WINDOW:
  case CID_MARKER_TEXT:
    minArgs = 2, maxArgs = 2;
    break;
  case CID_MARKER_IMAGE:
  case CID_MARKER_BITMAP:
    minArgs = 2, maxArgs = 4;
    break;
  default:
    Tcl_AppendResult(interp, "unknown marker type", (char*)NULL);
    return TCL_ERROR;
  }

  if (objc < minArgs) {
    Tcl_AppendResult(interp, "too few marker coordinates specified",
		     (char*)NULL);
    return TCL_ERROR;
  }
  if ((maxArgs > 0) && (objc > maxArgs)) {
    Tcl_AppendResult(interp, "too many marker coordinates specified",
		     (char*)NULL);
    return TCL_ERROR;
  }
  nWorldPts = objc / 2;
  worldPts = malloc(nWorldPts * sizeof(Point2d));
  if (worldPts == NULL) {
    Tcl_AppendResult(interp, "can't allocate new coordinate array",
		     (char*)NULL);
    return TCL_ERROR;
  }

  {
    Point2d *pp;

    pp = worldPts;
    for (i = 0; i < objc; i += 2) {
      double x, y;
	    
      if ((GetCoordinate(interp, objv[i], &x) != TCL_OK) ||
	  (GetCoordinate(interp, objv[i + 1], &y) != TCL_OK)) {
	free(worldPts);
	return TCL_ERROR;
      }
      pp->x = x, pp->y = y, pp++;
    }
  }
  /* Don't free the old coordinate array until we've parsed the new
   * coordinates without errors.  */
  if (markerPtr->worldPts != NULL) {
    free(markerPtr->worldPts);
  }
  markerPtr->worldPts = worldPts;
  markerPtr->nWorldPts = nWorldPts;
  markerPtr->flags |= MAP_ITEM;
  return TCL_OK;
}

static void FreeCoordsProc(ClientData clientData, Display *display,
			   char* widgRec, int offset)
{
  Marker *markerPtr = (Marker *)widgRec;
  Point2d **pointsPtr = (Point2d **)(widgRec + offset);

  if (*pointsPtr != NULL) {
    free(*pointsPtr);
    *pointsPtr = NULL;
  }
  markerPtr->nWorldPts = 0;
}

static int ObjToCoordsProc(ClientData clientData, Tcl_Interp* interp,
			   Tk_Window tkwin, Tcl_Obj *objPtr,
			   char* widgRec, int offset, int flags)
{
  Marker *markerPtr = (Marker *)widgRec;
  int objc;
  Tcl_Obj **objv;

  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
    return TCL_ERROR;
  }
  if (objc == 0) {
    return TCL_OK;
  }
  return ParseCoordinates(interp, markerPtr, objc, objv);
}

static Tcl_Obj* CoordsToObjProc(ClientData clientData, Tcl_Interp* interp, 
				Tk_Window tkwin, char* widgRec, 
				int offset, int flags)
{
  Marker *markerPtr = (Marker *)widgRec;
  Tcl_Obj *listObjPtr;
  Point2d *pp, *pend;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  for (pp = markerPtr->worldPts, pend = pp + markerPtr->nWorldPts; pp < pend;
       pp++) {
    Tcl_ListObjAppendElement(interp, listObjPtr, PrintCoordinate(pp->x));
    Tcl_ListObjAppendElement(interp, listObjPtr, PrintCoordinate(pp->y));
  }
  return listObjPtr;
}

static int GetColorPair(Tcl_Interp* interp, Tk_Window tkwin,
			Tcl_Obj *fgObjPtr, Tcl_Obj *bgObjPtr,
			ColorPair *pairPtr, int allowDefault)
{
  XColor* fgColor, *bgColor;
  const char* string;

  fgColor = bgColor = NULL;
  if (fgObjPtr != NULL) {
    int length;

    string = Tcl_GetStringFromObj(fgObjPtr, &length);
    if (string[0] == '\0') {
      fgColor = NULL;
    } else if ((allowDefault) && (string[0] == 'd') &&
	       (strncmp(string, "defcolor", length) == 0)) {
      fgColor = COLOR_DEFAULT;
    } else {
      fgColor = Tk_AllocColorFromObj(interp, tkwin, fgObjPtr);
      if (fgColor == NULL) {
	return TCL_ERROR;
      }
    }
  }
  if (bgObjPtr != NULL) {
    int length;

    string = Tcl_GetStringFromObj(bgObjPtr, &length);
    if (string[0] == '\0') {
      bgColor = NULL;
    } else if ((allowDefault) && (string[0] == 'd') &&
	       (strncmp(string, "defcolor", length) == 0)) {
      bgColor = COLOR_DEFAULT;
    } else {
      bgColor = Tk_AllocColorFromObj(interp, tkwin, bgObjPtr);
      if (bgColor == NULL) {
	return TCL_ERROR;
      }
    }
  }
  if (pairPtr->fgColor != NULL) {
    Tk_FreeColor(pairPtr->fgColor);
  }
  if (pairPtr->bgColor != NULL) {
    Tk_FreeColor(pairPtr->bgColor);
  }
  pairPtr->fgColor = fgColor;
  pairPtr->bgColor = bgColor;
  return TCL_OK;
}

void Blt_FreeColorPair(ColorPair *pairPtr)
{
  if ((pairPtr->bgColor != NULL) && (pairPtr->bgColor != COLOR_DEFAULT)) {
    Tk_FreeColor(pairPtr->bgColor);
  }
  if ((pairPtr->fgColor != NULL) && (pairPtr->fgColor != COLOR_DEFAULT)) {
    Tk_FreeColor(pairPtr->fgColor);
  }
  pairPtr->bgColor = pairPtr->fgColor = NULL;
}

static void FreeColorPairProc(ClientData clientData, Display *display,
			      char* widgRec, int offset)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
  Blt_FreeColorPair(pairPtr);
}

static int ObjToColorPairProc(ClientData clientData, Tcl_Interp* interp,
			      Tk_Window tkwin, Tcl_Obj *objPtr, char* widgRec,
			      int offset, int flags)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
  long longValue = (long)clientData;
  int bool;
  int objc;
  Tcl_Obj **objv;

  if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
    return TCL_ERROR;
  }
  if (objc > 2) {
    Tcl_AppendResult(interp, "too many names in colors list", 
		     (char*)NULL);
    return TCL_ERROR;
  }
  if (objc == 0) {
    Blt_FreeColorPair(pairPtr);
    return TCL_OK;
  }
  bool = (int)longValue;
  if (objc == 1) {
    if (GetColorPair(interp, tkwin, objv[0], NULL, pairPtr, bool) 
	!= TCL_OK) {
      return TCL_ERROR;
    }
  } else {
    if (GetColorPair(interp, tkwin, objv[0], objv[1], pairPtr, bool) 
	!= TCL_OK) {
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}

static const char* NameOfColor(XColor* colorPtr)
{
  if (colorPtr == NULL) {
    return "";
  } else if (colorPtr == COLOR_DEFAULT) {
    return "defcolor";
  } else {
    return Tk_NameOfColor(colorPtr);
  }
}

static Tcl_Obj* ColorPairToObjProc(ClientData clientData, Tcl_Interp* interp,
				   Tk_Window tkwin, char* widgRec,
				   int offset, int flags)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
  Tcl_Obj *listObjPtr;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  Tcl_ListObjAppendElement(interp, listObjPtr, 
			   Tcl_NewStringObj(NameOfColor(pairPtr->fgColor), -1));
  Tcl_ListObjAppendElement(interp, listObjPtr,
			   Tcl_NewStringObj(NameOfColor(pairPtr->bgColor), -1));
  return listObjPtr;
}

static int IsElementHidden(Marker *markerPtr)
{
  Tcl_HashEntry *hPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  /* Look up the named element and see if it's hidden */
  hPtr = Tcl_FindHashEntry(&graphPtr->elements.table, markerPtr->elemName);
  if (hPtr != NULL) {
    Element* elemPtr;
	
    elemPtr = Tcl_GetHashValue(hPtr);
    if ((elemPtr->link == NULL) || (elemPtr->flags & HIDE)) {
      return TRUE;
    }
  }
  return FALSE;
}

static double HMap(Axis *axisPtr, double x)
{
  if (x == DBL_MAX) {
    x = 1.0;
  } else if (x == -DBL_MAX) {
    x = 0.0;
  } else {
    if (axisPtr->logScale) {
      if (x > 0.0) {
	x = log10(x);
      } else if (x < 0.0) {
	x = 0.0;
      }
    }
    x = NORMALIZE(axisPtr, x);
  }
  if (axisPtr->descending) {
    x = 1.0 - x;
  }
  /* Horizontal transformation */
  return (x * axisPtr->screenRange + axisPtr->screenMin);
}

static double VMap(Axis *axisPtr, double y)
{
  if (y == DBL_MAX) {
    y = 1.0;
  } else if (y == -DBL_MAX) {
    y = 0.0;
  } else {
    if (axisPtr->logScale) {
      if (y > 0.0) {
	y = log10(y);
      } else if (y < 0.0) {
	y = 0.0;
      }
    }
    y = NORMALIZE(axisPtr, y);
  }
  if (axisPtr->descending) {
    y = 1.0 - y;
  }
  /* Vertical transformation. */
  return (((1.0 - y) * axisPtr->screenRange) + axisPtr->screenMin);
}

Point2d Blt_MapPoint(Point2d *pointPtr, Axis2d *axesPtr)
{
  Point2d result;
  Graph* graphPtr = axesPtr->y->obj.graphPtr;

  if (graphPtr->inverted) {
    result.x = HMap(axesPtr->y, pointPtr->y);
    result.y = VMap(axesPtr->x, pointPtr->x);
  } else {
    result.x = HMap(axesPtr->x, pointPtr->x);
    result.y = VMap(axesPtr->y, pointPtr->y);
  }
  return result;
}

static Marker* CreateMarker(Graph* graphPtr, const char* name, ClassId classId)
{    
  Marker *markerPtr;
  switch (classId) {
  case CID_MARKER_BITMAP:
    markerPtr = Blt_CreateBitmapProc(); /* bitmap */
    break;
  case CID_MARKER_LINE:
    markerPtr = CreateLineProc();	/* line */
    break;
  case CID_MARKER_IMAGE:
    return NULL; /* not supported */
    break;
  case CID_MARKER_TEXT:
    markerPtr = CreateTextProc();	/* text */
    break;
  case CID_MARKER_POLYGON:
    markerPtr = CreatePolygonProc(); /* polygon */
    break;
  case CID_MARKER_WINDOW:
    markerPtr = CreateWindowProc(); /* window */
    break;
  default:
    return NULL;
  }
  markerPtr->obj.graphPtr = graphPtr;
  markerPtr->drawUnder = FALSE;
  markerPtr->flags |= MAP_ITEM;
  markerPtr->obj.name = Blt_Strdup(name);
  Blt_GraphSetObjectClass(&markerPtr->obj, classId);
  return markerPtr;
}

static void DestroyMarker(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (markerPtr->drawUnder) {
    /* If the marker to be deleted is currently displayed below the
     * elements, then backing store needs to be repaired. */
    graphPtr->flags |= CACHE_DIRTY;
  }
  /* 
   * Call the marker's type-specific deallocation routine. We do it first
   * while all the marker fields are still valid.
   */
  (*markerPtr->classPtr->freeProc)(markerPtr);

  /* Dump any bindings that might be registered for the marker. */
  Blt_DeleteBindings(graphPtr->bindTable, markerPtr);

  /* Release all the X resources associated with the marker. */
  Blt_FreeOptions(markerPtr->classPtr->configSpecs, (char*)markerPtr,
		  graphPtr->display, 0);

  if (markerPtr->hashPtr != NULL) {
    Tcl_DeleteHashEntry(markerPtr->hashPtr);
  }
  if (markerPtr->link != NULL) {
    Blt_Chain_DeleteLink(graphPtr->markers.displayList, markerPtr->link);
  }
  if (markerPtr->obj.name != NULL) {
    free((void*)(markerPtr->obj.name));
  }
  free(markerPtr);
}

static void FreeMarker(char* dataPtr) 
{
  Marker *markerPtr = (Marker *)dataPtr;
  DestroyMarker(markerPtr);
}

static int ConfigureTextProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;

  tmPtr->style.angle = (float)fmod(tmPtr->style.angle, 360.0);
  if (tmPtr->style.angle < 0.0f) {
    tmPtr->style.angle += 360.0f;
  }
  newGC = NULL;
  if (tmPtr->fillColor != NULL) {
    gcMask = GCForeground;
    gcValues.foreground = tmPtr->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  }
  if (tmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, tmPtr->fillGC);
  }
  tmPtr->fillGC = newGC;

  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void MapTextProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Region2d extents;
  Point2d anchorPt;
  int i;
  unsigned int w, h;
  double rw, rh;

  tmPtr->width = tmPtr->height = 0;
  if (tmPtr->string == NULL) {
    return;
  }
  Blt_Ts_GetExtents(&tmPtr->style, tmPtr->string, &w, &h);
  Blt_GetBoundingBox(w, h, tmPtr->style.angle, &rw, &rh, tmPtr->outline);
  tmPtr->width = ROUND(rw);
  tmPtr->height = ROUND(rh);
  for (i = 0; i < 4; i++) {
    tmPtr->outline[i].x += ROUND(rw * 0.5);
    tmPtr->outline[i].y += ROUND(rh * 0.5);
  }
  tmPtr->outline[4].x = tmPtr->outline[0].x;
  tmPtr->outline[4].y = tmPtr->outline[0].y;
  anchorPt = Blt_MapPoint(markerPtr->worldPts, &markerPtr->axes);
  anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, (double)(tmPtr->width), 
			     (double)(tmPtr->height), tmPtr->anchor);
  anchorPt.x += markerPtr->xOffset;
  anchorPt.y += markerPtr->yOffset;
  /*
   * Determine the bounding box of the text and test to see if it is at
   * least partially contained within the plotting area.
   */
  extents.left = anchorPt.x;
  extents.top = anchorPt.y;
  extents.right = anchorPt.x + tmPtr->width - 1;
  extents.bottom = anchorPt.y + tmPtr->height - 1;
  markerPtr->clipped = Blt_BoxesDontOverlap(graphPtr, &extents);
  tmPtr->anchorPt = anchorPt;

}

static int PointInTextProc(Marker *markerPtr, Point2d *samplePtr)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;

  if (tmPtr->string == NULL) {
    return 0;
  }
  if (tmPtr->style.angle != 0.0f) {
    Point2d points[5];
    int i;

    /* 
     * Figure out the bounding polygon (isolateral) for the text and see
     * if the point is inside of it.
     */
    for (i = 0; i < 5; i++) {
      points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
      points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
    }
    return Blt_PointInPolygon(samplePtr, points, 5);
  } 
  return ((samplePtr->x >= tmPtr->anchorPt.x) && 
	  (samplePtr->x < (tmPtr->anchorPt.x + tmPtr->width)) &&
	  (samplePtr->y >= tmPtr->anchorPt.y) && 
	  (samplePtr->y < (tmPtr->anchorPt.y + tmPtr->height)));
}

static int RegionInTextProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;

  if (markerPtr->nWorldPts < 1) {
    return FALSE;
  }
  if (tmPtr->style.angle != 0.0f) {
    Point2d points[5];
    int i;
	
    /*  
     * Generate the bounding polygon (isolateral) for the bitmap and see
     * if the point is inside of it.
     */
    for (i = 0; i < 4; i++) {
      points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
      points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
    }
    return Blt_RegionInPolygon(extsPtr, points, 4, enclosed);
  } 
  if (enclosed) {
    return ((tmPtr->anchorPt.x >= extsPtr->left) &&
	    (tmPtr->anchorPt.y >= extsPtr->top) && 
	    ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->right) &&
	    ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->bottom));
  }
  return !((tmPtr->anchorPt.x >= extsPtr->right) ||
	   (tmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->left) ||
	   ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->top));
}

static void DrawTextProc(Marker *markerPtr, Drawable drawable) 
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (tmPtr->string == NULL) {
    return;
  }
  if (tmPtr->fillGC != NULL) {
    XPoint points[4];
    int i;

    /*
     * Simulate the rotated background of the bitmap by filling a bounding
     * polygon with the background color.
     */
    for (i = 0; i < 4; i++) {
      points[i].x = (short int)(tmPtr->outline[i].x + tmPtr->anchorPt.x);
      points[i].y = (short int)(tmPtr->outline[i].y + tmPtr->anchorPt.y);
    }
    XFillPolygon(graphPtr->display, drawable, tmPtr->fillGC, points, 4,
		 Convex, CoordModeOrigin);
  }
  if (tmPtr->style.color != NULL) {
    Blt_Ts_DrawText(graphPtr->tkwin, drawable, tmPtr->string, -1,
		    &tmPtr->style, (int)tmPtr->anchorPt.x, (int)tmPtr->anchorPt.y);
  }
}

static void TextToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;

  if (tmPtr->string == NULL) {
    return;
  }
  if (tmPtr->fillGC != NULL) {
    Point2d points[4];
    int i;

    /*
     * Simulate the rotated background of the bitmap by filling a bounding
     * polygon with the background color.
     */
    for (i = 0; i < 4; i++) {
      points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
      points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
    }
    Blt_Ps_XSetBackground(ps, tmPtr->fillColor);
    Blt_Ps_XFillPolygon(ps, points, 4);
  }
  Blt_Ps_DrawText(ps, tmPtr->string, &tmPtr->style, tmPtr->anchorPt.x, 
		  tmPtr->anchorPt.y);
}

static void FreeTextProc(Marker *markerPtr)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  Blt_Ts_FreeStyle(graphPtr->display, &tmPtr->style);
}

static Marker* CreateTextProc(void)
{
  TextMarker *tmPtr;

  tmPtr = calloc(1, sizeof(TextMarker));
  tmPtr->classPtr = &textMarkerClass;
  Blt_Ts_InitStyle(tmPtr->style);
  tmPtr->style.anchor = TK_ANCHOR_NW;
  tmPtr->style.xPad = 4;
  tmPtr->style.yPad = 4;
  return (Marker *)tmPtr;
}

static Tk_EventProc ChildEventProc;
static Tk_GeomRequestProc ChildGeometryProc;
static Tk_GeomLostSlaveProc ChildCustodyProc;
static Tk_GeomMgr winMarkerMgrInfo =
  {
    (char*)"graph",			/* Name of geometry manager used by
					 * winfo */
    ChildGeometryProc,			/* Procedure to for new geometry
					 * requests. */
    ChildCustodyProc,			/* Procedure when window is taken
					 * away. */
  };

static int ConfigureWindowProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;
  Tk_Window tkwin;

  if (wmPtr->childName == NULL) {
    return TCL_OK;
  }
  tkwin = Tk_NameToWindow(graphPtr->interp, wmPtr->childName, 
			  graphPtr->tkwin);
  if (tkwin == NULL) {
    return TCL_ERROR;
  }
  if (Tk_Parent(tkwin) != graphPtr->tkwin) {
    Tcl_AppendResult(graphPtr->interp, "\"", wmPtr->childName,
		     "\" is not a child of \"", Tk_PathName(graphPtr->tkwin), "\"",
		     (char*)NULL);
    return TCL_ERROR;
  }
  if (tkwin != wmPtr->child) {
    if (wmPtr->child != NULL) {
      Tk_DeleteEventHandler(wmPtr->child, StructureNotifyMask,
			    ChildEventProc, wmPtr);
      Tk_ManageGeometry(wmPtr->child, (Tk_GeomMgr *) 0, (ClientData)0);
      Tk_UnmapWindow(wmPtr->child);
    }
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, ChildEventProc, 
			  wmPtr);
    Tk_ManageGeometry(tkwin, &winMarkerMgrInfo, wmPtr);
  }
  wmPtr->child = tkwin;
  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void MapWindowProc(Marker *markerPtr)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;
  Point2d anchorPt;
  Region2d extents;
  int width, height;

  if (wmPtr->child == (Tk_Window)NULL) {
    return;
  }
  anchorPt = Blt_MapPoint(markerPtr->worldPts, &markerPtr->axes);

  width = Tk_ReqWidth(wmPtr->child);
  height = Tk_ReqHeight(wmPtr->child);
  if (wmPtr->reqWidth > 0) {
    width = wmPtr->reqWidth;
  }
  if (wmPtr->reqHeight > 0) {
    height = wmPtr->reqHeight;
  }
  wmPtr->anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, (double)width, 
				    (double)height, wmPtr->anchor);
  wmPtr->anchorPt.x += markerPtr->xOffset;
  wmPtr->anchorPt.y += markerPtr->yOffset;
  wmPtr->width = width;
  wmPtr->height = height;

  /*
   * Determine the bounding box of the window and test to see if it is at
   * least partially contained within the plotting area.
   */
  extents.left = wmPtr->anchorPt.x;
  extents.top = wmPtr->anchorPt.y;
  extents.right = wmPtr->anchorPt.x + wmPtr->width - 1;
  extents.bottom = wmPtr->anchorPt.y + wmPtr->height - 1;
  markerPtr->clipped = Blt_BoxesDontOverlap(graphPtr, &extents);
}

static int PointInWindowProc(Marker *markerPtr, Point2d *samplePtr)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  return ((samplePtr->x >= wmPtr->anchorPt.x) && 
	  (samplePtr->x < (wmPtr->anchorPt.x + wmPtr->width)) &&
	  (samplePtr->y >= wmPtr->anchorPt.y) && 
	  (samplePtr->y < (wmPtr->anchorPt.y + wmPtr->height)));
}

static int RegionInWindowProc(Marker *markerPtr, Region2d *extsPtr, 
			      int enclosed)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  if (markerPtr->nWorldPts < 1) {
    return FALSE;
  }
  if (enclosed) {
    return ((wmPtr->anchorPt.x >= extsPtr->left) &&
	    (wmPtr->anchorPt.y >= extsPtr->top) && 
	    ((wmPtr->anchorPt.x + wmPtr->width) <= extsPtr->right) &&
	    ((wmPtr->anchorPt.y + wmPtr->height) <= extsPtr->bottom));
  }
  return !((wmPtr->anchorPt.x >= extsPtr->right) ||
	   (wmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((wmPtr->anchorPt.x + wmPtr->width) <= extsPtr->left) ||
	   ((wmPtr->anchorPt.y + wmPtr->height) <= extsPtr->top));
}

static void DrawWindowProc(Marker *markerPtr, Drawable drawable)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  if (wmPtr->child == NULL) {
    return;
  }
  if ((wmPtr->height != Tk_Height(wmPtr->child)) ||
      (wmPtr->width != Tk_Width(wmPtr->child)) ||
      ((int)wmPtr->anchorPt.x != Tk_X(wmPtr->child)) ||
      ((int)wmPtr->anchorPt.y != Tk_Y(wmPtr->child))) {
    Tk_MoveResizeWindow(wmPtr->child, (int)wmPtr->anchorPt.x, 
			(int)wmPtr->anchorPt.y, wmPtr->width, wmPtr->height);
  }
  if (!Tk_IsMapped(wmPtr->child)) {
    Tk_MapWindow(wmPtr->child);
  }
}

static void WindowToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  if (wmPtr->child == NULL) {
    return;
  }
  if (Tk_IsMapped(wmPtr->child)) {
    //	Blt_Ps_XDrawWindow(ps, wmPtr->child, wmPtr->anchorPt.x, wmPtr->anchorPt.y);
  }
}

static void FreeWindowProc(Marker *markerPtr)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  if (wmPtr->child != NULL) {
    Tk_DeleteEventHandler(wmPtr->child, StructureNotifyMask,
			  ChildEventProc, wmPtr);
    Tk_ManageGeometry(wmPtr->child, (Tk_GeomMgr *) 0, (ClientData)0);
    Tk_DestroyWindow(wmPtr->child);
  }
}

static Marker * CreateWindowProc(void)
{
  WindowMarker *wmPtr;

  wmPtr = calloc(1, sizeof(WindowMarker));
  wmPtr->classPtr = &windowMarkerClass;
  return (Marker *)wmPtr;
}

static void ChildEventProc(ClientData clientData, XEvent *eventPtr)
{
  WindowMarker *wmPtr = clientData;

  if (eventPtr->type == DestroyNotify) {
    wmPtr->child = NULL;
  }
}

static void
ChildGeometryProc(ClientData clientData, Tk_Window tkwin)
{
  WindowMarker *wmPtr = clientData;

  if (wmPtr->reqWidth == 0) {
    wmPtr->width = Tk_ReqWidth(tkwin);
  }
  if (wmPtr->reqHeight == 0) {
    wmPtr->height = Tk_ReqHeight(tkwin);
  }
}

static void ChildCustodyProc(ClientData clientData, Tk_Window tkwin)
{
  Marker *markerPtr = clientData;
  Graph* graphPtr;

  graphPtr = markerPtr->obj.graphPtr;
  markerPtr->flags |= DELETE_PENDING;
  Tcl_EventuallyFree(markerPtr, FreeMarker);
  /*
   * Not really needed. We should get an Expose event when the child window
   * is unmapped.
   */
  Blt_EventuallyRedrawGraph(graphPtr);
}

static void MapLineProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker *)markerPtr;
  Point2d *srcPtr, *pend;
  Segment2d *segments, *segPtr;
  Point2d p, q;
  Region2d extents;

  lmPtr->nSegments = 0;
  if (lmPtr->segments != NULL) {
    free(lmPtr->segments);
  }
  if (markerPtr->nWorldPts < 2) {
    return;				/* Too few points */
  }
  Blt_GraphExtents(graphPtr, &extents);

  /* 
   * Allow twice the number of world coordinates. The line will represented
   * as series of line segments, not one continous polyline.  This is
   * because clipping against the plot area may chop the line into several
   * disconnected segments.
   */
  segments = malloc(markerPtr->nWorldPts * sizeof(Segment2d));
  srcPtr = markerPtr->worldPts;
  p = Blt_MapPoint(srcPtr, &markerPtr->axes);
  p.x += markerPtr->xOffset;
  p.y += markerPtr->yOffset;

  segPtr = segments;
  for (srcPtr++, pend = markerPtr->worldPts + markerPtr->nWorldPts; 
       srcPtr < pend; srcPtr++) {
    Point2d next;

    next = Blt_MapPoint(srcPtr, &markerPtr->axes);
    next.x += markerPtr->xOffset;
    next.y += markerPtr->yOffset;
    q = next;
    if (Blt_LineRectClip(&extents, &p, &q)) {
      segPtr->p = p;
      segPtr->q = q;
      segPtr++;
    }
    p = next;
  }
  lmPtr->nSegments = segPtr - segments;
  lmPtr->segments = segments;
  markerPtr->clipped = (lmPtr->nSegments == 0);
}

static int
PointInLineProc(Marker *markerPtr, Point2d *samplePtr)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;

  return Blt_PointInSegments(samplePtr, lmPtr->segments, lmPtr->nSegments, 
			     (double)markerPtr->obj.graphPtr->search.halo);
}

static int RegionInLineProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  if (markerPtr->nWorldPts < 2) {
    return FALSE;
  }
  if (enclosed) {
    Point2d *pp, *pend;

    for (pp = markerPtr->worldPts, pend = pp + markerPtr->nWorldPts; 
	 pp < pend; pp++) {
      Point2d p;

      p = Blt_MapPoint(pp, &markerPtr->axes);
      if ((p.x < extsPtr->left) && (p.x > extsPtr->right) &&
	  (p.y < extsPtr->top) && (p.y > extsPtr->bottom)) {
	return FALSE;
      }
    }
    return TRUE;			/* All points inside bounding box. */
  } else {
    int count;
    Point2d *pp, *pend;

    count = 0;
    for (pp = markerPtr->worldPts, pend = pp + (markerPtr->nWorldPts - 1); 
	 pp < pend; pp++) {
      Point2d p, q;

      p = Blt_MapPoint(pp, &markerPtr->axes);
      q = Blt_MapPoint(pp + 1, &markerPtr->axes);
      if (Blt_LineRectClip(extsPtr, &p, &q)) {
	count++;
      }
    }
    return (count > 0);		/* At least 1 segment passes through
				 * region. */
  }
}

static void DrawLineProc(Marker *markerPtr, Drawable drawable)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;

  if (lmPtr->nSegments > 0) {
    Graph* graphPtr = markerPtr->obj.graphPtr;

    Blt_Draw2DSegments(graphPtr->display, drawable, lmPtr->gc, 
		       lmPtr->segments, lmPtr->nSegments);
    if (lmPtr->xor) {		/* Toggle the drawing state */
      lmPtr->xorState = (lmPtr->xorState == 0);
    }
  }
}

static int ConfigureLineProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  LineMarker *lmPtr = (LineMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;
  Drawable drawable;

  drawable = Tk_WindowId(graphPtr->tkwin);
  gcMask = (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle);
  if (lmPtr->outlineColor != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = lmPtr->outlineColor->pixel;
  }
  if (lmPtr->fillColor != NULL) {
    gcMask |= GCBackground;
    gcValues.background = lmPtr->fillColor->pixel;
  }
  gcValues.cap_style = lmPtr->capStyle;
  gcValues.join_style = lmPtr->joinStyle;
  gcValues.line_width = LineWidth(lmPtr->lineWidth);
  gcValues.line_style = LineSolid;
  if (LineIsDashed(lmPtr->dashes)) {
    gcValues.line_style = 
      (gcMask & GCBackground) ? LineDoubleDash : LineOnOffDash;
  }
  if (lmPtr->xor) {
    unsigned long pixel;
    gcValues.function = GXxor;

    gcMask |= GCFunction;
    pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
    if (gcMask & GCBackground) {
      gcValues.background ^= pixel;
    }
    gcValues.foreground ^= pixel;
    if (drawable != None) {
      DrawLineProc(markerPtr, drawable);
    }
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (lmPtr->gc != NULL) {
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);
  }
  if (LineIsDashed(lmPtr->dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &lmPtr->dashes);
  }
  lmPtr->gc = newGC;
  if (lmPtr->xor) {
    if (drawable != None) {
      MapLineProc(markerPtr);
      DrawLineProc(markerPtr, drawable);
    }
    return TCL_OK;
  }
  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void LineToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;

  if (lmPtr->nSegments > 0) {
    Blt_Ps_XSetLineAttributes(ps, lmPtr->outlineColor, 
			      lmPtr->lineWidth, &lmPtr->dashes, lmPtr->capStyle,
			      lmPtr->joinStyle);
    if ((LineIsDashed(lmPtr->dashes)) && (lmPtr->fillColor != NULL)) {
      Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
      Blt_Ps_XSetBackground(ps, lmPtr->fillColor);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
      Blt_Ps_VarAppend(ps,
		       "stroke\n",
		       "  grestore\n",
		       "} def\n", (char*)NULL);
    } else {
      Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
    Blt_Ps_Draw2DSegments(ps, lmPtr->segments, lmPtr->nSegments);
  }
}

static void FreeLineProc(Marker *markerPtr)
{
  LineMarker *lmPtr = (LineMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (lmPtr->gc != NULL) {
    Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);
  }
  if (lmPtr->segments != NULL) {
    free(lmPtr->segments);
  }
}

static Marker* CreateLineProc(void)
{
  LineMarker *lmPtr;

  lmPtr = calloc(1, sizeof(LineMarker));
  lmPtr->classPtr = &lineMarkerClass;
  lmPtr->xor = FALSE;
  lmPtr->capStyle = CapButt;
  lmPtr->joinStyle = JoinMiter;
  return (Marker *)lmPtr;
}

static void MapPolygonProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  Point2d *screenPts;
  Region2d extents;
  int nScreenPts;

  if (pmPtr->outlinePts != NULL) {
    free(pmPtr->outlinePts);
    pmPtr->outlinePts = NULL;
    pmPtr->nOutlinePts = 0;
  }
  if (pmPtr->fillPts != NULL) {
    free(pmPtr->fillPts);
    pmPtr->fillPts = NULL;
    pmPtr->nFillPts = 0;
  }
  if (pmPtr->screenPts != NULL) {
    free(pmPtr->screenPts);
    pmPtr->screenPts = NULL;
  }
  if (markerPtr->nWorldPts < 3) {
    return;				/* Too few points */
  }

  /* 
   * Allocate and fill a temporary array to hold the screen coordinates of
   * the polygon.
   */
  nScreenPts = markerPtr->nWorldPts + 1;
  screenPts = malloc((nScreenPts + 1) * sizeof(Point2d));
  {
    Point2d *sp, *dp, *send;

    dp = screenPts;
    for (sp = markerPtr->worldPts, send = sp + markerPtr->nWorldPts; 
	 sp < send; sp++) {
      *dp = Blt_MapPoint(sp, &markerPtr->axes);
      dp->x += markerPtr->xOffset;
      dp->y += markerPtr->yOffset;
      dp++;
    }
    *dp = screenPts[0];
  }
  Blt_GraphExtents(graphPtr, &extents);
  markerPtr->clipped = TRUE;
  if (pmPtr->fill.fgColor != NULL) {	/* Polygon fill required. */
    Point2d *fillPts;
    int n;

    fillPts = malloc(sizeof(Point2d) * nScreenPts * 3);
    n = Blt_PolyRectClip(&extents, screenPts, markerPtr->nWorldPts,fillPts);
    if (n < 3) { 
      free(fillPts);
    } else {
      pmPtr->nFillPts = n;
      pmPtr->fillPts = fillPts;
      markerPtr->clipped = FALSE;
    }
  }
  if ((pmPtr->outline.fgColor != NULL) && (pmPtr->lineWidth > 0)) { 
    Segment2d *outlinePts;
    Segment2d *segPtr;
    Point2d *sp, *send;

    /* 
     * Generate line segments representing the polygon outline.  The
     * resulting outline may or may not be closed from viewport clipping.
     */
    outlinePts = malloc(nScreenPts * sizeof(Segment2d));
    if (outlinePts == NULL) {
      return;			/* Can't allocate point array */
    }
    /* 
     * Note that this assumes that the point array contains an extra point
     * that closes the polygon.
     */
    segPtr = outlinePts;
    for (sp = screenPts, send = sp + (nScreenPts - 1); sp < send; sp++) {
      segPtr->p = sp[0];
      segPtr->q = sp[1];
      if (Blt_LineRectClip(&extents, &segPtr->p, &segPtr->q)) {
	segPtr++;
      }
    }
    pmPtr->nOutlinePts = segPtr - outlinePts;
    pmPtr->outlinePts = outlinePts;
    if (pmPtr->nOutlinePts > 0) {
      markerPtr->clipped = FALSE;
    }
  }
  pmPtr->screenPts = screenPts;
}

static int PointInPolygonProc(Marker *markerPtr, Point2d *samplePtr)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  if ((markerPtr->nWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
    return Blt_PointInPolygon(samplePtr, pmPtr->screenPts, 
			      markerPtr->nWorldPts + 1);
  }
  return FALSE;
}

static int RegionInPolygonProc(Marker *markerPtr, Region2d *extsPtr, 
			       int enclosed)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    
  if ((markerPtr->nWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
    return Blt_RegionInPolygon(extsPtr, pmPtr->screenPts, 
			       markerPtr->nWorldPts, enclosed);
  }
  return FALSE;
}

static void DrawPolygonProc(Marker *markerPtr, Drawable drawable)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  /* Draw polygon fill region */
  if ((pmPtr->nFillPts > 0) && (pmPtr->fill.fgColor != NULL)) {
    XPoint *dp, *points;
    Point2d *sp, *send;
	
    points = malloc(pmPtr->nFillPts * sizeof(XPoint));
    if (points == NULL) {
      return;
    }
    dp = points;
    for (sp = pmPtr->fillPts, send = sp + pmPtr->nFillPts; sp < send; 
	 sp++) {
      dp->x = (short int)sp->x;
      dp->y = (short int)sp->y;
      dp++;
    }

    XFillPolygon(graphPtr->display, drawable, pmPtr->fillGC, points, 
		 pmPtr->nFillPts, Complex, CoordModeOrigin);
    free(points);
  }
  /* and then the outline */
  if ((pmPtr->nOutlinePts > 0) && (pmPtr->lineWidth > 0) && 
      (pmPtr->outline.fgColor != NULL)) {
    Blt_Draw2DSegments(graphPtr->display, drawable, pmPtr->outlineGC,
		       pmPtr->outlinePts, pmPtr->nOutlinePts);
  }
}

static void PolygonToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  if (pmPtr->fill.fgColor != NULL) {

    /*
     * Options:  fg bg
     *			Draw outline only.
     *	     x          Draw solid or stipple.
     *	     x  x       Draw solid or stipple.
     */

    /* Create a path to use for both the polygon and its outline. */
    Blt_Ps_Polyline(ps, pmPtr->fillPts, pmPtr->nFillPts);

    /* If the background fill color was specified, draw the polygon in a
     * solid fashion with that color.  */
    if (pmPtr->fill.bgColor != NULL) {
      /* Draw the solid background as the background layer of the opaque
       * stipple  */
      Blt_Ps_XSetBackground(ps, pmPtr->fill.bgColor);
      /* Retain the path. We'll need it for the foreground layer. */
      Blt_Ps_Append(ps, "gsave fill grestore\n");
    }
    Blt_Ps_XSetForeground(ps, pmPtr->fill.fgColor);
    if (pmPtr->stipple != None) {
      /* Draw the stipple in the foreground color. */
      Blt_Ps_XSetStipple(ps, graphPtr->display, pmPtr->stipple);
    } else {
      Blt_Ps_Append(ps, "fill\n");
    }
  }

  /* Draw the outline in the foreground color.  */
  if ((pmPtr->lineWidth > 0) && (pmPtr->outline.fgColor != NULL)) {

    /*  Set up the line attributes.  */
    Blt_Ps_XSetLineAttributes(ps, pmPtr->outline.fgColor,
			      pmPtr->lineWidth, &pmPtr->dashes, pmPtr->capStyle,
			      pmPtr->joinStyle);

    /*  
     * Define on-the-fly a PostScript macro "DashesProc" that will be
     * executed for each call to the Polygon drawing routine.  If the line
     * isn't dashed, simply make this an empty definition.
     */
    if ((pmPtr->outline.bgColor != NULL) && (LineIsDashed(pmPtr->dashes))) {
      Blt_Ps_Append(ps, "/DashesProc {\ngsave\n    ");
      Blt_Ps_XSetBackground(ps, pmPtr->outline.bgColor);
      Blt_Ps_Append(ps, "    ");
      Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
      Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
    } else {
      Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
    Blt_Ps_Draw2DSegments(ps, pmPtr->outlinePts, pmPtr->nOutlinePts);
  }
}

static int ConfigurePolygonProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;
  Drawable drawable;

  drawable = Tk_WindowId(graphPtr->tkwin);
  gcMask = (GCLineWidth | GCLineStyle);
  if (pmPtr->outline.fgColor != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = pmPtr->outline.fgColor->pixel;
  }
  if (pmPtr->outline.bgColor != NULL) {
    gcMask |= GCBackground;
    gcValues.background = pmPtr->outline.bgColor->pixel;
  }
  gcMask |= (GCCapStyle | GCJoinStyle);
  gcValues.cap_style = pmPtr->capStyle;
  gcValues.join_style = pmPtr->joinStyle;
  gcValues.line_style = LineSolid;
  gcValues.dash_offset = 0;
  gcValues.line_width = LineWidth(pmPtr->lineWidth);
  if (LineIsDashed(pmPtr->dashes)) {
    gcValues.line_style = (pmPtr->outline.bgColor == NULL)
      ? LineOnOffDash : LineDoubleDash;
  }
  if (pmPtr->xor) {
    unsigned long pixel;
    gcValues.function = GXxor;

    gcMask |= GCFunction;
    pixel = Tk_3DBorderColor(graphPtr->plotBg)->pixel;
    if (gcMask & GCBackground) {
      gcValues.background ^= pixel;
    }
    gcValues.foreground ^= pixel;
    if (drawable != None) {
      DrawPolygonProc(markerPtr, drawable);
    }
  }
  newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
  if (LineIsDashed(pmPtr->dashes)) {
    Blt_SetDashes(graphPtr->display, newGC, &pmPtr->dashes);
  }
  if (pmPtr->outlineGC != NULL) {
    Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
  }
  pmPtr->outlineGC = newGC;

  gcMask = 0;
  if (pmPtr->fill.fgColor != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = pmPtr->fill.fgColor->pixel;
  }
  if (pmPtr->fill.bgColor != NULL) {
    gcMask |= GCBackground;
    gcValues.background = pmPtr->fill.bgColor->pixel;
  }
  if (pmPtr->stipple != None) {
    gcValues.stipple = pmPtr->stipple;
    gcValues.fill_style = (pmPtr->fill.bgColor != NULL)
      ? FillOpaqueStippled : FillStippled;
    gcMask |= (GCStipple | GCFillStyle);
  }
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (pmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
  }
  pmPtr->fillGC = newGC;

  if ((gcMask == 0) && !(graphPtr->flags & RESET_AXES) && (pmPtr->xor)) {
    if (drawable != None) {
      MapPolygonProc(markerPtr);
      DrawPolygonProc(markerPtr, drawable);
    }
    return TCL_OK;
  }
  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  graphPtr->flags |= RESET_WORLD;
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static void FreePolygonProc(Marker *markerPtr)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (pmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
  }
  if (pmPtr->outlineGC != NULL) {
    Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
  }
  if (pmPtr->fillPts != NULL) {
    free(pmPtr->fillPts);
  }
  if (pmPtr->outlinePts != NULL) {
    free(pmPtr->outlinePts);
  }
  if (pmPtr->screenPts != NULL) {
    free(pmPtr->screenPts);
  }
}

static Marker* CreatePolygonProc(void)
{
  PolygonMarker *pmPtr;

  pmPtr = calloc(1, sizeof(PolygonMarker));
  pmPtr->classPtr = &polygonMarkerClass;
  pmPtr->capStyle = CapButt;
  pmPtr->joinStyle = JoinMiter;
  return (Marker *)pmPtr;
}

static int GetMarkerFromObj(Tcl_Interp* interp, Graph* graphPtr, 
			    Tcl_Obj *objPtr, Marker **markerPtrPtr)
{
  Tcl_HashEntry *hPtr;
  const char* string;

  string = Tcl_GetString(objPtr);
  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, string);
  if (hPtr != NULL) {
    *markerPtrPtr = Tcl_GetHashValue(hPtr);
    return TCL_OK;
  }
  if (interp != NULL) {
    Tcl_AppendResult(interp, "can't find marker \"", string, 
		     "\" in \"", Tk_PathName(graphPtr->tkwin), (char*)NULL);
  }
  return TCL_ERROR;
}

static int RenameMarker(Graph* graphPtr, Marker *markerPtr, 
			const char* oldName, const char* newName)
{
  int isNew;
  Tcl_HashEntry *hPtr;

  /* Rename the marker only if no marker already exists by that name */
  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.table, newName, &isNew);
  if (!isNew) {
    Tcl_AppendResult(graphPtr->interp, "can't rename marker: \"", newName,
		     "\" already exists", (char*)NULL);
    return TCL_ERROR;
  }
  markerPtr->obj.name = Blt_Strdup(newName);
  markerPtr->hashPtr = hPtr;
  Tcl_SetHashValue(hPtr, (char*)markerPtr);

  /* Delete the old hash entry */
  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, oldName);
  Tcl_DeleteHashEntry(hPtr);
  if (oldName != NULL) {
    free((void*)oldName);
  }
  return TCL_OK;
}

static int NamesOp(Graph* graphPtr, Tcl_Interp* interp, 
		   int objc, Tcl_Obj* const objv[])
{
  Tcl_Obj *listObjPtr;

  listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
  if (objc == 3) {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Marker *markerPtr;

      markerPtr = Blt_Chain_GetValue(link);
      Tcl_ListObjAppendElement(interp, listObjPtr,
			       Tcl_NewStringObj(markerPtr->obj.name, -1));
    }
  } else {
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
      Marker *markerPtr;
      int i;

      markerPtr = Blt_Chain_GetValue(link);
      for (i = 3; i < objc; i++) {
	const char* pattern;

	pattern = Tcl_GetString(objv[i]);
	if (Tcl_StringMatch(markerPtr->obj.name, pattern)) {
	  Tcl_ListObjAppendElement(interp, listObjPtr,
				   Tcl_NewStringObj(markerPtr->obj.name, -1));
	  break;
	}
      }
    }
  }
  Tcl_SetObjResult(interp, listObjPtr);
  return TCL_OK;
}

static int BindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  if (objc == 3) {
    Tcl_HashEntry *hp;
    Tcl_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hp = Tcl_FirstHashEntry(&graphPtr->markers.tagTable, &iter);
	 hp != NULL; hp = Tcl_NextHashEntry(&iter)) {
      const char* tag;
      Tcl_Obj *objPtr;

      tag = Tcl_GetHashKey(&graphPtr->markers.tagTable, hp);
      objPtr = Tcl_NewStringObj(tag, -1);
      Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
  }
  return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
				      Blt_MakeMarkerTag(graphPtr, Tcl_GetString(objv[3])),
				      objc - 4, objv + 4);
}

static int CgetOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;

  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, 
				markerPtr->classPtr->configSpecs, (char*)markerPtr, objv[4], 0) 
      != TCL_OK) {
    return TCL_ERROR;
  }
  return TCL_OK;
}

static int ConfigureOp(Graph* graphPtr, Tcl_Interp* interp, 
		       int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  Tcl_Obj *const *options;
  const char* oldName;
  const char* string;
  int flags = BLT_CONFIG_OBJV_ONLY;
  int nNames, nOpts;
  int i;
  int under;

  markerPtr = NULL;			/* Suppress compiler warning. */

  /* Figure out where the option value pairs begin */
  objc -= 3;
  objv += 3;
  for (i = 0; i < objc; i++) {
    string = Tcl_GetString(objv[i]);
    if (string[0] == '-') {
      break;
    }
    if (GetMarkerFromObj(interp, graphPtr, objv[i], &markerPtr) != TCL_OK) {
      return TCL_ERROR;
    }
  }
  nNames = i;				/* # of element names specified */
  nOpts = objc - i;			/* # of options specified */
  options = objv + nNames;		/* Start of options in objv  */
    
  for (i = 0; i < nNames; i++) {
    GetMarkerFromObj(interp, graphPtr, objv[i], &markerPtr);
    if (nOpts == 0) {
      return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
				      markerPtr->classPtr->configSpecs, (char*)markerPtr, 
				      (Tcl_Obj *)NULL, flags);
    } else if (nOpts == 1) {
      return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin,
				      markerPtr->classPtr->configSpecs, (char*)markerPtr, 
				      options[0], flags);
    }
    /* Save the old marker name. */
    oldName = markerPtr->obj.name;
    under = markerPtr->drawUnder;
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, 
				   markerPtr->classPtr->configSpecs, nOpts, options, 
				   (char*)markerPtr, flags) != TCL_OK) {
      return TCL_ERROR;
    }
    if (oldName != markerPtr->obj.name) {
      if (RenameMarker(graphPtr, markerPtr, oldName, markerPtr->obj.name)
	  != TCL_OK) {
	markerPtr->obj.name = oldName;
	return TCL_ERROR;
      }
    }
    if ((*markerPtr->classPtr->configProc) (markerPtr) != TCL_OK) {

      return TCL_ERROR;
    }
    if (markerPtr->drawUnder != under) {
      graphPtr->flags |= CACHE_DIRTY;
    }
  }
  return TCL_OK;
}

static int CreateOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  Tcl_HashEntry *hPtr;
  int isNew;
  ClassId classId;
  int i;
  const char* name;
  char ident[200];
  const char* string;
  char c;

  string = Tcl_GetString(objv[3]);
  c = string[0];
  /* Create the new marker based upon the given type */
  if ((c == 't') && (strcmp(string, "text") == 0)) {
    classId = CID_MARKER_TEXT;
  } else if ((c == 'l') && (strcmp(string, "line") == 0)) {
    classId = CID_MARKER_LINE;
  } else if ((c == 'p') && (strcmp(string, "polygon") == 0)) {
    classId = CID_MARKER_POLYGON;
  } else if ((c == 'i') && (strcmp(string, "image") == 0)) {
    classId = CID_MARKER_IMAGE;
  } else if ((c == 'b') && (strcmp(string, "bitmap") == 0)) {
    classId = CID_MARKER_BITMAP;
  } else if ((c == 'w') && (strcmp(string, "window") == 0)) {
    classId = CID_MARKER_WINDOW;
  } else {
    Tcl_AppendResult(interp, "unknown marker type \"", string,
		     "\": should be \"text\", \"line\", \"polygon\", \"bitmap\", \"image\", or \
\"window\"", (char*)NULL);
    return TCL_ERROR;
  }
  /* Scan for "-name" option. We need it for the component name */
  name = NULL;
  for (i = 4; i < objc; i += 2) {
    int length;

    string = Tcl_GetStringFromObj(objv[i], &length);
    if ((length > 1) && (strncmp(string, "-name", length) == 0)) {
      name = Tcl_GetString(objv[i + 1]);
      break;
    }
  }
  /* If no name was given for the marker, make up one. */
  if (name == NULL) {
    sprintf_s(ident, 200, "marker%d", graphPtr->nextMarkerId++);
    name = ident;
  } else if (name[0] == '-') {
    Tcl_AppendResult(interp, "name of marker \"", name, 
		     "\" can't start with a '-'", (char*)NULL);
    return TCL_ERROR;
  }
  markerPtr = CreateMarker(graphPtr, name, classId);
  if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, name, 
				    markerPtr->obj.className, markerPtr->classPtr->configSpecs, 
				    objc - 4, objv + 4, (char*)markerPtr, 0) != TCL_OK) {
    DestroyMarker(markerPtr);
    return TCL_ERROR;
  }
  if ((*markerPtr->classPtr->configProc) (markerPtr) != TCL_OK) {
    DestroyMarker(markerPtr);
    return TCL_ERROR;
  }
  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.table, name, &isNew);
  if (!isNew) {
    Marker *oldPtr;
    /*
     * Marker by the same name already exists.  Delete the old marker and
     * it's list entry.  But save the hash entry.
     */
    oldPtr = Tcl_GetHashValue(hPtr);
    oldPtr->hashPtr = NULL;
    DestroyMarker(oldPtr);
  }
  Tcl_SetHashValue(hPtr, markerPtr);
  markerPtr->hashPtr = hPtr;
  /* Unlike elements, new markers are drawn on top of old markers. */
  markerPtr->link = Blt_Chain_Prepend(graphPtr->markers.displayList,markerPtr);
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
  return TCL_OK;
}

static int DeleteOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  int i;

  for (i = 3; i < objc; i++) {
    Marker *markerPtr;

    if (GetMarkerFromObj(NULL, graphPtr, objv[i], &markerPtr) == TCL_OK) {
      markerPtr->flags |= DELETE_PENDING;
      Tcl_EventuallyFree(markerPtr, FreeMarker);
    }
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int GetOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  const char* string;

  string = Tcl_GetString(objv[3]);
  if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
    markerPtr = (Marker *)Blt_GetCurrentItem(graphPtr->bindTable);
    if (markerPtr == NULL) {
      return TCL_OK;		/* Report only on markers. */

    }
    if ((markerPtr->obj.classId >= CID_MARKER_BITMAP) &&
	(markerPtr->obj.classId <= CID_MARKER_WINDOW))	    {
      Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		       markerPtr->obj.name, -1);
    }
  }
  return TCL_OK;
}

static int RelinkOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Blt_ChainLink link, place;
  Marker *markerPtr;
  const char* string;

  /* Find the marker to be raised or lowered. */
  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  /* Right now it's assumed that all markers are always in the display
     list. */
  link = markerPtr->link;
  Blt_Chain_UnlinkLink(graphPtr->markers.displayList, markerPtr->link);

  place = NULL;
  if (objc == 5) {
    if (GetMarkerFromObj(interp, graphPtr, objv[4], &markerPtr) != TCL_OK) {
      return TCL_ERROR;
    }
    place = markerPtr->link;
  }

  /* Link the marker at its new position. */
  string = Tcl_GetString(objv[2]);
  if (string[0] == 'l') {
    Blt_Chain_LinkAfter(graphPtr->markers.displayList, link, place);
  } else {
    Blt_Chain_LinkBefore(graphPtr->markers.displayList, link, place);
  }
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }
  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

static int FindOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Blt_ChainLink link;
  Region2d extents;
  const char* string;
  int enclosed;
  int left, right, top, bottom;
  int mode;

#define FIND_ENCLOSED	 (1<<0)
#define FIND_OVERLAPPING (1<<1)
  string = Tcl_GetString(objv[3]);
  if (strcmp(string, "enclosed") == 0) {
    mode = FIND_ENCLOSED;
  } else if (strcmp(string, "overlapping") == 0) {
    mode = FIND_OVERLAPPING;
  } else {
    Tcl_AppendResult(interp, "bad search type \"", string, 
		     ": should be \"enclosed\", or \"overlapping\"", (char*)NULL);
    return TCL_ERROR;
  }

  if ((Tcl_GetIntFromObj(interp, objv[4], &left) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[5], &top) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[6], &right) != TCL_OK) ||
      (Tcl_GetIntFromObj(interp, objv[7], &bottom) != TCL_OK)) {
    return TCL_ERROR;
  }
  if (left < right) {
    extents.left = (double)left;
    extents.right = (double)right;
  } else {
    extents.left = (double)right;
    extents.right = (double)left;
  }
  if (top < bottom) {
    extents.top = (double)top;
    extents.bottom = (double)bottom;
  } else {
    extents.top = (double)bottom;
    extents.bottom = (double)top;
  }
  enclosed = (mode == FIND_ENCLOSED);
  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if ((markerPtr->flags & DELETE_PENDING) || markerPtr->hide) {
      continue;
    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    if ((*markerPtr->classPtr->regionProc)(markerPtr, &extents, enclosed)) {
      Tcl_Obj *objPtr;

      objPtr = Tcl_GetObjResult(interp);
      Tcl_SetStringObj(objPtr, markerPtr->obj.name, -1);
      return TCL_OK;
    }
  }
  Tcl_SetStringObj(Tcl_GetObjResult(interp), "", -1);
  return TCL_OK;
}

static int ExistsOp(Graph* graphPtr, Tcl_Interp* interp, 
		    int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
  return TCL_OK;
}

static int TypeOp(Graph* graphPtr, Tcl_Interp* interp, 
		  int objc, Tcl_Obj* const objv[])
{
  Marker *markerPtr;
  const char* type;

  if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
    return TCL_ERROR;
  }
  switch (markerPtr->obj.classId) {
  case CID_MARKER_BITMAP:	type = "bitmap";	break;
  case CID_MARKER_IMAGE:	type = "image";		break;
  case CID_MARKER_LINE:	type = "line";		break;
  case CID_MARKER_POLYGON:	type = "polygon";	break;
  case CID_MARKER_TEXT:	type = "text";		break;
  case CID_MARKER_WINDOW:	type = "window";	break;
  default:			type = "???";		break;
  }
  Tcl_SetStringObj(Tcl_GetObjResult(interp), type, -1);
  return TCL_OK;
}

static Blt_OpSpec markerOps[] =
  {
    {"bind",      1, BindOp,   3, 6, "marker sequence command",},
    {"cget",      2, CgetOp,   5, 5, "marker option",},
    {"configure", 2, ConfigureOp, 4, 0,"marker ?marker?... ?option value?...",},
    {"create",    2, CreateOp, 4, 0, "type ?option value?...",},
    {"delete",    1, DeleteOp, 3, 0, "?marker?...",},
    {"exists",    1, ExistsOp, 4, 4, "marker",},
    {"find",      1, FindOp,   8, 8, "enclosed|overlapping x1 y1 x2 y2",},
    {"get",       1, GetOp,    4, 4, "name",},
    {"lower",     1, RelinkOp, 4, 5, "marker ?afterMarker?",},
    {"names",     1, NamesOp,  3, 0, "?pattern?...",},
    {"raise",     1, RelinkOp, 4, 5, "marker ?beforeMarker?",},
    {"type",      1, TypeOp,   4, 4, "marker",},
  };
static int nMarkerOps = sizeof(markerOps) / sizeof(Blt_OpSpec);

int Blt_MarkerOp(Graph* graphPtr, Tcl_Interp* interp, 
		 int objc, Tcl_Obj* const objv[])
{
  GraphMarkerProc *proc;
  int result;

  proc = Blt_GetOpFromObj(interp, nMarkerOps, markerOps, BLT_OP_ARG2, 
			  objc, objv,0);
  if (proc == NULL) {
    return TCL_ERROR;
  }
  result = (*proc) (graphPtr, interp, objc, objv);
  return result;
}

void Blt_MarkersToPostScript(Graph* graphPtr, Blt_Ps ps, int under)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if ((markerPtr->classPtr->postscriptProc == NULL) || 
	(markerPtr->nWorldPts == 0)) {
      continue;
    }
    if (markerPtr->drawUnder != under) {
      continue;
    }
    if ((markerPtr->flags & DELETE_PENDING) || markerPtr->hide) {
      continue;
    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    Blt_Ps_VarAppend(ps, "\n% Marker \"", markerPtr->obj.name, 
		     "\" is a ", markerPtr->obj.className, ".\n", (char*)NULL);
    (*markerPtr->classPtr->postscriptProc) (markerPtr, ps);
  }
}

void Blt_DrawMarkers(Graph* graphPtr, Drawable drawable, int under)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_PrevLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);

    if ((markerPtr->nWorldPts == 0) || 
	(markerPtr->drawUnder != under) ||
	(markerPtr->clipped) ||
	(markerPtr->flags & DELETE_PENDING) ||
	(markerPtr->hide)) {
      continue;
    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    (*markerPtr->classPtr->drawProc) (markerPtr, drawable);
  }
}

void
Blt_ConfigureMarkers(Graph* graphPtr)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    (*markerPtr->classPtr->configProc) (markerPtr);
  }
}

void Blt_MapMarkers(Graph* graphPtr)
{
  Blt_ChainLink link;

  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if (markerPtr->nWorldPts == 0) {
      continue;
    }
    if ((markerPtr->flags & DELETE_PENDING) || markerPtr->hide) {
      continue;
    }
    if ((graphPtr->flags & MAP_ALL) || (markerPtr->flags & MAP_ITEM)) {
      (*markerPtr->classPtr->mapProc) (markerPtr);
      markerPtr->flags &= ~MAP_ITEM;
    }
  }
}

void Blt_DestroyMarkers(Graph* graphPtr)
{
  Tcl_HashEntry *hPtr;
  Tcl_HashSearch iter;

  for (hPtr = Tcl_FirstHashEntry(&graphPtr->markers.table, &iter); 
       hPtr != NULL; hPtr = Tcl_NextHashEntry(&iter)) {
    Marker *markerPtr;

    markerPtr = Tcl_GetHashValue(hPtr);
    /*
     * Dereferencing the pointer to the hash table prevents the hash table
     * entry from being automatically deleted.
     */
    markerPtr->hashPtr = NULL;
    DestroyMarker(markerPtr);
  }
  Tcl_DeleteHashTable(&graphPtr->markers.table);
  Tcl_DeleteHashTable(&graphPtr->markers.tagTable);
  Blt_Chain_Destroy(graphPtr->markers.displayList);
}

Marker* Blt_NearestMarker(Graph* graphPtr, int x, int y, int under)
{
  Blt_ChainLink link;
  Point2d point;

  point.x = (double)x;
  point.y = (double)y;
  for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
       link != NULL; link = Blt_Chain_NextLink(link)) {
    Marker *markerPtr;

    markerPtr = Blt_Chain_GetValue(link);
    if ((markerPtr->nWorldPts == 0) ||
	(markerPtr->flags & (DELETE_PENDING|MAP_ITEM)) ||
	(markerPtr->hide)) {
      continue;			/* Don't consider markers that are
				 * pending to be mapped. Even if the
				 * marker has already been mapped, the
				 * coordinates could be invalid now.
				 * Better to pick no marker than the
				 * wrong marker. */

    }
    if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
      continue;
    }
    if ((markerPtr->drawUnder == under) && 
	(markerPtr->state == BLT_STATE_NORMAL)) {
      if ((*markerPtr->classPtr->pointProc) (markerPtr, &point)) {
	return markerPtr;
      }
    }
  }
  return NULL;
}

ClientData Blt_MakeMarkerTag(Graph* graphPtr, const char* tagName)
{
  Tcl_HashEntry *hPtr;
  int isNew;

  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->markers.tagTable, hPtr);
}
