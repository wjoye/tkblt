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

#include <assert.h>

#include "bltInt.h"
#include "bltMath.h"
#include "bltGraph.h"
#include "bltOp.h"
#include "bltGrElem.h"
#include "bltBitmap.h"
#include "bltConfig.h"

typedef int (GraphMarkerProc)(Graph* graphPtr, Tcl_Interp* interp, int objc, 
			      Tcl_Obj* const objv[]);

#define GETBITMAP(b)							\
  (((b)->destBitmap == None) ? (b)->srcBitmap : (b)->destBitmap)

#define MAX_OUTLINE_POINTS	12

#define IMAGE_PHOTO		(1<<7)

/* Map graph coordinates to normalized coordinates [0..1] */
#define NORMALIZE(A,x) 	(((x) - (A)->axisRange.min) * (A)->axisRange.scale)

#define DEF_MARKER_ANCHOR	"center"
#define DEF_MARKER_BACKGROUND	white
#define DEF_MARKER_BITMAP	(char*)NULL
#define DEF_MARKER_CAP_STYLE	"butt"
#define DEF_MARKER_COORDS	(char*)NULL
#define DEF_MARKER_DASHES	(char*)NULL
#define DEF_MARKER_DASH_OFFSET	"0"
#define DEF_MARKER_ELEMENT	(char*)NULL
#define DEF_MARKER_FOREGROUND	black
#define DEF_MARKER_FILL_COLOR	rred
#define DEF_MARKER_FONT		STD_FONT_NORMAL
#define DEF_MARKER_GAP_COLOR	pink
#define DEF_MARKER_HEIGHT	"0"
#define DEF_MARKER_HIDE		"no"
#define DEF_MARKER_JOIN_STYLE	"miter"
#define DEF_MARKER_JUSTIFY	"left"
#define DEF_MARKER_LINE_WIDTH	"1"
#define DEF_MARKER_MAP_X	"x"
#define DEF_MARKER_MAP_Y	"y"
#define DEF_MARKER_NAME		(char*)NULL
#define DEF_MARKER_OUTLINE_COLOR black
#define DEF_MARKER_PAD		"4"
#define DEF_MARKER_ANGLE	"0.0"
#define DEF_MARKER_SCALE	"1.0"
#define DEF_MARKER_STATE	"normal"
#define DEF_MARKER_STIPPLE	(char*)NULL
#define DEF_MARKER_TEXT		(char*)NULL
#define DEF_MARKER_UNDER	"no"
#define DEF_MARKER_WIDTH	"0"
#define DEF_MARKER_WINDOW	(char*)NULL
#define DEF_MARKER_XOR		"no"
#define DEF_MARKER_X_OFFSET	"0"
#define DEF_MARKER_Y_OFFSET	"0"
#define DEF_MARKER_FILTER	"box"

#define DEF_TEXT_TAGS		"Text all"
#define DEF_IMAGE_TAGS		"Image all"
#define DEF_BITMAP_TAGS		"Bitmap all"
#define DEF_WINDOW_TAGS		"Window all"
#define DEF_POLYGON_TAGS	"Polygon all"
#define DEF_LINE_TAGS		"Line all"

static Blt_OptionParseProc ObjToCoordsProc;
static Blt_OptionPrintProc CoordsToObjProc;
static Blt_OptionFreeProc FreeCoordsProc;
static Blt_CustomOption coordsOption =
  {
    ObjToCoordsProc, CoordsToObjProc, FreeCoordsProc, (ClientData)0
  };
static Blt_OptionFreeProc FreeColorPairProc;
static Blt_OptionParseProc ObjToColorPairProc;
static Blt_OptionPrintProc ColorPairToObjProc;
static Blt_CustomOption colorPairOption =
  {
    ObjToColorPairProc, ColorPairToObjProc, FreeColorPairProc, (ClientData)0
  };

extern Blt_CustomOption bltXAxisOption;
extern Blt_CustomOption bltYAxisOption;

typedef Marker *(MarkerCreateProc)(void);
typedef void    (MarkerDrawProc)(Marker *markerPtr, Drawable drawable);
typedef void    (MarkerFreeProc)(Marker *markerPtr);
typedef int     (MarkerConfigProc)(Marker *markerPtr);
typedef void    (MarkerMapProc)(Marker *markerPtr);
typedef void    (MarkerPostscriptProc)(Marker *markerPtr, Blt_Ps ps);
typedef int     (MarkerPointProc)(Marker *markerPtr, Point2d *samplePtr);
typedef int     (MarkerRegionProc)(Marker *markerPtr, Region2d *extsPtr, 
				   int enclosed);

static Tcl_FreeProc FreeMarker;

typedef struct {
  Blt_ConfigSpec *configSpecs;	/* Marker configuration
				 * specifications */
  MarkerConfigProc *configProc;
  MarkerDrawProc *drawProc;
  MarkerFreeProc *freeProc;
  MarkerMapProc *mapProc;
  MarkerPointProc *pointProc;
  MarkerRegionProc *regionProc;
  MarkerPostscriptProc *postscriptProc;

}  MarkerClass;

/*
 *---------------------------------------------------------------------------
 *
 * Marker --
 *
 *	Structure defining the generic marker.  In C++ parlance this would be
 *	the base class from which all markers are derived.
 *
 *	This structure corresponds with the specific types of markers.  Don't
 *	change this structure without changing the individual marker
 *	structures of each type below.
 *
 * -------------------------------------------------------------------------- 
 */
struct _Marker {
  GraphObj obj;			/* Must be first field in marker. */

  MarkerClass *classPtr;

  Tcl_HashEntry *hashPtr;

  Blt_ChainLink link;

  const char* elemName;		/* Element associated with marker. Let's
				 * you link a marker to an element. The
				 * marker is drawn only if the element
				 * is also visible. */
  Axis2d axes;
  Point2d *worldPts;		        /* Coordinate array to position
					 * marker */
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
};
/*
 *---------------------------------------------------------------------------
 *
 * BitmapMarker --
 *
 *---------------------------------------------------------------------------
 */
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
  int nWorldPts;			/* # of points in above array. */

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

  /* Fields specific to bitmap markers. */

  Pixmap srcBitmap;			/* Original bitmap. May be further
					 * scaled or rotated. */
  double reqAngle;			/* Requested rotation of the bitmap */
  float angle;			/* Normalized rotation (0..360
				 * degrees) */
  Tk_Anchor anchor;			/* If only one X-Y coordinate is given,
					 * indicates how to translate the given
					 * marker position.  Otherwise, if there
					 * are two X-Y coordinates, then this
					 * value is ignored. */
  Point2d anchorPt;			/* Translated anchor point. */

  XColor *outlineColor;		/* Foreground color */
  XColor* fillColor;			/* Background color */

  GC gc;				/* Private graphic context */
  GC fillGC;				/* Shared graphic context */
  Pixmap destBitmap;			/* Bitmap to be drawn. */
  int destWidth, destHeight;		/* Dimensions of the final bitmap */

  Point2d outline[MAX_OUTLINE_POINTS];/* Polygon representing the background
				       * of the bitmap. */
  int nOutlinePts;
} BitmapMarker;

static Blt_ConfigSpec bitmapConfigSpecs[] = {
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
   Tk_Offset(BitmapMarker, anchor), 0},
  {BLT_CONFIG_COLOR, "-background", "background", "Background",
   DEF_MARKER_BACKGROUND, Tk_Offset(BitmapMarker, fillColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-bg", "background", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_BITMAP_TAGS, 
   Tk_Offset(BitmapMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap", DEF_MARKER_BITMAP, 
   Tk_Offset(BitmapMarker, srcBitmap), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
   Tk_Offset(BitmapMarker, worldPts), BLT_CONFIG_NULL_OK, 
   &coordsOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
   Tk_Offset(BitmapMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char*)NULL, (char*)NULL, 0, 0},
  {BLT_CONFIG_SYNONYM, "-fill", "background", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
   DEF_MARKER_FOREGROUND, Tk_Offset(BitmapMarker, outlineColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
   Tk_Offset(BitmapMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
   Tk_Offset(BitmapMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
   Tk_Offset(BitmapMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, DEF_MARKER_NAME, 
   Tk_Offset(BitmapMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-outline", "foreground", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_DOUBLE, "-rotate", "rotate", "Rotate", DEF_MARKER_ANGLE, 
   Tk_Offset(BitmapMarker, reqAngle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_MARKER_STATE, 
   Tk_Offset(BitmapMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
   Tk_Offset(BitmapMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
   Tk_Offset(BitmapMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
   Tk_Offset(BitmapMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc ConfigureBitmapProc;
static MarkerCreateProc CreateBitmapProc;
static MarkerDrawProc DrawBitmapProc;
static MarkerFreeProc FreeBitmapProc;
static MarkerMapProc MapBitmapProc;
static MarkerPointProc PointInBitmapProc;
static MarkerPostscriptProc BitmapToPostscriptProc;
static MarkerRegionProc RegionInBitmapProc;

static MarkerClass bitmapMarkerClass = {
  bitmapConfigSpecs,
  ConfigureBitmapProc,
  DrawBitmapProc,
  FreeBitmapProc,
  MapBitmapProc,
  PointInBitmapProc,
  RegionInBitmapProc,
  BitmapToPostscriptProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * LineMarker --
 *
 *---------------------------------------------------------------------------
 */
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
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_LINE_TAGS, 
   Tk_Offset(LineMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", DEF_MARKER_CAP_STYLE, 
   Tk_Offset(LineMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
   Tk_Offset(LineMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
  {BLT_CONFIG_CUSTOM, "-dashes", "dashes", "Dashes", DEF_MARKER_DASHES, 
   Tk_Offset(LineMarker, dashes), BLT_CONFIG_NULL_OK, &dashesOption},
  {BLT_CONFIG_PIXELS, "-dashoffset", "dashOffset", "DashOffset",
   DEF_MARKER_DASH_OFFSET, Tk_Offset(LineMarker, dashes.offset),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
   Tk_Offset(LineMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-fill", "fill", "Fill", (char*)NULL, 
   Tk_Offset(LineMarker, fillColor), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", DEF_MARKER_JOIN_STYLE, 
   Tk_Offset(LineMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   DEF_MARKER_LINE_WIDTH, Tk_Offset(LineMarker, lineWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
   Tk_Offset(LineMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
   Tk_Offset(LineMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
   Tk_Offset(LineMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, DEF_MARKER_NAME, 
   Tk_Offset(LineMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_COLOR, "-outline", "outline", "Outline",
   DEF_MARKER_OUTLINE_COLOR, Tk_Offset(LineMarker, outlineColor),
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_MARKER_STATE, 
   Tk_Offset(LineMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
   Tk_Offset(LineMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
   Tk_Offset(LineMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", DEF_MARKER_XOR, 
   Tk_Offset(LineMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
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

/*
 *---------------------------------------------------------------------------
 *
 * PolygonMarker --
 *
 *---------------------------------------------------------------------------
 */
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
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_POLYGON_TAGS, 
   Tk_Offset(PolygonMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", DEF_MARKER_CAP_STYLE, 
   Tk_Offset(PolygonMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
   Tk_Offset(PolygonMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
  {BLT_CONFIG_CUSTOM, "-dashes", "dashes", "Dashes", DEF_MARKER_DASHES, 
   Tk_Offset(PolygonMarker, dashes), BLT_CONFIG_NULL_OK, &dashesOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
   Tk_Offset(PolygonMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_MARKER_FILL_COLOR, 
   Tk_Offset(PolygonMarker, fill), BLT_CONFIG_NULL_OK, &colorPairOption},
  {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", DEF_MARKER_JOIN_STYLE, 
   Tk_Offset(PolygonMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-linewidth", "lineWidth", "LineWidth",
   DEF_MARKER_LINE_WIDTH, Tk_Offset(PolygonMarker, lineWidth),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
   Tk_Offset(PolygonMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
   Tk_Offset(PolygonMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
   Tk_Offset(PolygonMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, DEF_MARKER_NAME, 
   Tk_Offset(PolygonMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", 
   DEF_MARKER_OUTLINE_COLOR, Tk_Offset(PolygonMarker, outline),
   BLT_CONFIG_NULL_OK, &colorPairOption},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_MARKER_STATE, 
   Tk_Offset(PolygonMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BITMAP, "-stipple", "stipple", "Stipple", DEF_MARKER_STIPPLE, 
   Tk_Offset(PolygonMarker, stipple), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
   Tk_Offset(PolygonMarker, drawUnder), 
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
   Tk_Offset(PolygonMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", DEF_MARKER_XOR, 
   Tk_Offset(PolygonMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
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


/*
 *---------------------------------------------------------------------------
 *
 * TextMarker --
 *
 *---------------------------------------------------------------------------
 */
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
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
   Tk_Offset(TextMarker, anchor), 0},
  {BLT_CONFIG_COLOR, "-background", "background", "MarkerBackground",
   (char*)NULL, Tk_Offset(TextMarker, fillColor), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-bg", "background", "Background", (char*)NULL, 0, 0},
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_TEXT_TAGS,
   Tk_Offset(TextMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
   Tk_Offset(TextMarker, worldPts), BLT_CONFIG_NULL_OK, 
   &coordsOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element",
   DEF_MARKER_ELEMENT, Tk_Offset(TextMarker, elemName), 
   BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-fg", "foreground", "Foreground", (char*)NULL, 0, 0},
  {BLT_CONFIG_SYNONYM, "-fill", "background", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_FONT, "-font", "font", "Font", 	DEF_MARKER_FONT, 
   Tk_Offset(TextMarker, style.font), 0},
  {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
   DEF_MARKER_FOREGROUND, Tk_Offset(TextMarker, style.color), 0},
  {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
   DEF_MARKER_JUSTIFY, Tk_Offset(TextMarker, style.justify),
   BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
   Tk_Offset(TextMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
   Tk_Offset(TextMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
   Tk_Offset(TextMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, DEF_MARKER_NAME, 
   Tk_Offset(TextMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_SYNONYM, "-outline", "foreground", (char*)NULL, (char*)NULL, 
   0, 0},
  {BLT_CONFIG_PIXELS, "-padx", "padX", "PadX", DEF_MARKER_PAD, 
   Tk_Offset(TextMarker, style.xPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-pady", "padY", "PadY", DEF_MARKER_PAD, 
   Tk_Offset(TextMarker, style.yPad), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_DOUBLE, "-rotate", "rotate", "Rotate", DEF_MARKER_ANGLE, 
   Tk_Offset(TextMarker, style.angle), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_MARKER_STATE, 
   Tk_Offset(TextMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_STRING, "-text", "text", "Text", DEF_MARKER_TEXT, 
   Tk_Offset(TextMarker, string), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
   Tk_Offset(TextMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
   Tk_Offset(TextMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
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

/*
 *---------------------------------------------------------------------------
 *
 * WindowMarker --
 *
 *---------------------------------------------------------------------------
 */
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
  {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
   Tk_Offset(WindowMarker, anchor), 0},
  {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags", DEF_WINDOW_TAGS, 
   Tk_Offset(WindowMarker, obj.tags), BLT_CONFIG_NULL_OK,
   &listOption},
  {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
   Tk_Offset(WindowMarker, worldPts), BLT_CONFIG_NULL_OK, 
   &coordsOption},
  {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
   Tk_Offset(WindowMarker, elemName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_PIXELS, "-height", "height", "Height", DEF_MARKER_HEIGHT, 
   Tk_Offset(WindowMarker, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
   Tk_Offset(WindowMarker, hide), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
   Tk_Offset(WindowMarker, axes.x), 0, &bltXAxisOption},
  {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
   Tk_Offset(WindowMarker, axes.y), 0, &bltYAxisOption},
  {BLT_CONFIG_STRING, "-name", (char*)NULL, (char*)NULL, DEF_MARKER_NAME, 
   Tk_Offset(WindowMarker, obj.name), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_MARKER_STATE, 
   Tk_Offset(WindowMarker, state), BLT_CONFIG_DONT_SET_DEFAULT, &stateOption},
  {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
   Tk_Offset(WindowMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-width", "width", "Width", DEF_MARKER_WIDTH, 
   Tk_Offset(WindowMarker, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_STRING, "-window", "window", "Window", DEF_MARKER_WINDOW, 
   Tk_Offset(WindowMarker, childName), BLT_CONFIG_NULL_OK},
  {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
   Tk_Offset(WindowMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
  {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
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

/*
 *---------------------------------------------------------------------------
 *
 * BoxesDontOverlap --
 *
 *	Tests if the bounding box of a marker overlaps the plotting area in
 *	any way.  If so, the marker will be drawn.  Just do a min/max test on
 *	the extents of both boxes.
 *
 *	Note: It's assumed that the extents of the bounding box lie 
 *	      within the area.  So for a 10x10 rectangle, bottom and
 *	      left would be 9.
 *
 * Results:
 *	Returns 0 is the marker is visible in the plotting area, and 1
 *	otherwise (marker is clipped).
 *
 *---------------------------------------------------------------------------
 */
static int
BoxesDontOverlap(Graph* graphPtr, Region2d *extsPtr)
{
  assert(extsPtr->right >= extsPtr->left);
  assert(extsPtr->bottom >= extsPtr->top);
  assert(graphPtr->right >= graphPtr->left);
  assert(graphPtr->bottom >= graphPtr->top);

  return (((double)graphPtr->right < extsPtr->left) ||
	  ((double)graphPtr->bottom < extsPtr->top) ||
	  (extsPtr->right < (double)graphPtr->left) ||
	  (extsPtr->bottom < (double)graphPtr->top));
}


/*
 *---------------------------------------------------------------------------
 *
 * GetCoordinate --
 *
 * 	Convert the expression string into a floating point value. The * only
 * 	reason we use this routine instead of Blt_ExprDouble is to * handle
 * 	"elastic" bounds.  That is, convert the strings "-Inf", * "Inf" into
 * 	-(DBL_MAX) and DBL_MAX respectively.
 *
 * Results:
 *	The return value is a standard TCL result.  The value of the
 * 	expression is passed back via valuePtr.
 *
 *---------------------------------------------------------------------------
 */
static int
GetCoordinate(
	      Tcl_Interp* interp,			/* Interpreter to return results */
	      Tcl_Obj *objPtr,			/* Numeric expression string to
						 * parse */
	      double *valuePtr)			/* Real-valued result of expression */
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


/*
 *---------------------------------------------------------------------------
 *
 * PrintCoordinate --
 *
 * 	Convert the floating point value into its string representation.  The
 * 	only reason this routine is used in instead of sprintf, is to handle
 * 	the "elastic" bounds.  That is, convert the values DBL_MAX and
 * 	-(DBL_MAX) into "+Inf" and "-Inf" respectively.
 *
 * Results:
 *	The return value is a standard TCL result.  The string of the *
 *	expression is passed back via string.
 *
 * -------------------------------------------------------------------------- */
static Tcl_Obj *
PrintCoordinate(double x)
{
  if (x == DBL_MAX) {
    return Tcl_NewStringObj("+Inf", -1);
  } else if (x == -DBL_MAX) {
    return Tcl_NewStringObj("-Inf", -1);
  } else {
    return Tcl_NewDoubleObj(x);
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseCoordinates --
 *
 *	The TCL coordinate list is converted to their floating point
 *	values. It will then replace the current marker coordinates.
 *
 *	Since different marker types require different number of coordinates
 *	this must be checked here.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side effects:
 *	If the marker coordinates are reset, the graph is eventually redrawn
 *	with at the new marker coordinates.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseCoordinates(
		 Tcl_Interp* interp,
		 Marker *markerPtr,
		 int objc,
		 Tcl_Obj* const objv[])
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

/*ARGSUSED*/
static void
FreeCoordsProc(
	       ClientData clientData,		/* Not used. */
	       Display *display,			/* Not used. */
	       char* widgRec,
	       int offset)
{
  Marker *markerPtr = (Marker *)widgRec;
  Point2d **pointsPtr = (Point2d **)(widgRec + offset);

  if (*pointsPtr != NULL) {
    free(*pointsPtr);
    *pointsPtr = NULL;
  }
  markerPtr->nWorldPts = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToCoordsProc --
 *
 *	Given a TCL list of numeric expression representing the element
 *	values, convert into an array of floating point values. In addition,
 *	the minimum and maximum values are saved.  Since elastic values are
 *	allow (values which translate to the min/max of the graph), we must
 *	try to get the non-elastic minimum and maximum.
 *
 * Results:
 *	The return value is a standard TCL result.  The vector is
 *	passed back via the vecPtr.
 *
 * -------------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
ObjToCoordsProc(
		ClientData clientData,	/* Not used. */
		Tcl_Interp* interp,		/* Interpreter to return results */
		Tk_Window tkwin,		/* Not used. */
		Tcl_Obj *objPtr,		/* TCL list of numeric expressions */
		char* widgRec,		/* Marker record */
		int offset,			/* Not used. */
		int flags)			/* Not used. */
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

/*
 *---------------------------------------------------------------------------
 *
 * CoordsToObjProc --
 *
 *	Convert the vector of floating point values into a TCL list.
 *
 * Results:
 *	The string representation of the vector is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
CoordsToObjProc(
		ClientData clientData,	/* Not used. */
		Tcl_Interp* interp,
		Tk_Window tkwin,		/* Not used. */
		char* widgRec,		/* Marker record */
		int offset,			/* Not used. */
		int flags)			/* Not used. */
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

/*LINTLIBRARY*/
static int
GetColorPair(
	     Tcl_Interp* interp,
	     Tk_Window tkwin,
	     Tcl_Obj *fgObjPtr, Tcl_Obj *bgObjPtr,
	     ColorPair *pairPtr,
	     int allowDefault)
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

void
Blt_FreeColorPair(ColorPair *pairPtr)
{
  if ((pairPtr->bgColor != NULL) && (pairPtr->bgColor != COLOR_DEFAULT)) {
    Tk_FreeColor(pairPtr->bgColor);
  }
  if ((pairPtr->fgColor != NULL) && (pairPtr->fgColor != COLOR_DEFAULT)) {
    Tk_FreeColor(pairPtr->fgColor);
  }
  pairPtr->bgColor = pairPtr->fgColor = NULL;
}

static void
FreeColorPairProc(
		  ClientData clientData,		/* Not used. */
		  Display *display,			/* Not used. */
		  char* widgRec,
		  int offset)
{
  ColorPair *pairPtr = (ColorPair *)(widgRec + offset);

  Blt_FreeColorPair(pairPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColorPairProc --
 *
 *	Convert the color names into pair of XColor pointers.
 *
 * Results:
 *	A standard TCL result.  The color pointer is written into the
 *	widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColorPairProc(
		   ClientData clientData,		/* Not used. */
		   Tcl_Interp* interp,			/* Interpreter to return results */
		   Tk_Window tkwin,			/* Not used. */
		   Tcl_Obj *objPtr,			/* String representing color */
		   char* widgRec,			/* Widget record */
		   int offset,				/* Offset to field in structure */
		   int flags)				/* Not used. */
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

static Tcl_Obj *
ColorPairToObjProc(
		   ClientData clientData,		/* Not used. */
		   Tcl_Interp* interp,
		   Tk_Window tkwin,			/* Not used. */
		   char* widgRec,			/* Element information record */
		   int offset,				/* Offset to field in structure */
		   int flags)				/* Not used. */
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

static INLINE int
IsElementHidden(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * HMap --
 *
 *	Maps the given graph coordinate value to its axis, returning a window
 *	position.  This is a slight variation on the normal Blt_HMap routine.
 *	It treats -Inf as the minimum axis value and Inf as the maximum.
 *
 * Results:
 *	Returns a floating point number representing the window coordinate
 *	position on the given axis.
 *
 * -------------------------------------------------------------------------- 
 */
static double
HMap(Axis *axisPtr, double x)
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

/*
 *---------------------------------------------------------------------------
 *
 * VMap --
 *
 *	Map the given graph coordinate value to its axis, returning a window
 *	position.  This is a slight variation on the normal Blt_VMap routine.
 *	It treats -Inf as the minimum axis value and Inf as the maximum.
 *
 * Results:
 *	Returns a double precision number representing the window coordinate
 *	position on the given axis.
 *
 *---------------------------------------------------------------------------
 */
static double
VMap(Axis *axisPtr, double y)
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

/*
 *---------------------------------------------------------------------------
 *
 * MapPoint --
 *
 *	Maps the given graph x,y coordinate values to a window position.
 *
 * Results:
 *	Returns a XPoint structure containing the window coordinates of the
 *	given graph x,y coordinate.
 *
 *---------------------------------------------------------------------------
 */
static Point2d
MapPoint(
	 Point2d *pointPtr,		/* Graph X-Y coordinate. */
	 Axis2d *axesPtr)		/* Specifies which axes to use */
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
  return result;			/* Result is screen coordinate. */
}

static Marker *
CreateMarker(
	     Graph* graphPtr,
	     const char* name,
	     ClassId classId)
{    
  Marker *markerPtr;

  /* Create the new marker based upon the given type */
  switch (classId) {
  case CID_MARKER_BITMAP:
    markerPtr = CreateBitmapProc(); /* bitmap */
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


static void
DestroyMarker(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureBitmapProc --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a bitmap
 *	marker.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as bitmap pixmap, colors, rotation,
 *	etc. get set for markerPtr; old resources get freed, if there were
 *	any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ConfigureBitmapProc(Marker *markerPtr)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  GC newGC;
  XGCValues gcValues;
  unsigned long gcMask;

  if (bmPtr->srcBitmap == None) {
    return TCL_OK;
  }
  bmPtr->angle = fmod(bmPtr->reqAngle, 360.0);
  if (bmPtr->angle < 0.0) {
    bmPtr->angle += 360.0;
  }
  gcMask = 0;

  if (bmPtr->outlineColor != NULL) {
    gcMask |= GCForeground;
    gcValues.foreground = bmPtr->outlineColor->pixel;
  }

  if (bmPtr->fillColor != NULL) {
    /* Opaque bitmap: both foreground and background (fill) colors
     * are used. */
    gcValues.background = bmPtr->fillColor->pixel;
    gcMask |= GCBackground;
  } else {
    /* Transparent bitmap: set the clip mask to the current bitmap. */
    gcValues.clip_mask = bmPtr->srcBitmap;
    gcMask |= GCClipMask;
  }

  /* 
   * This is technically a shared GC, but we're going to set/change the clip
   * origin anyways before we draw the bitmap.  This relies on the fact that
   * no other client will be allocated this GC with the GCClipMask set to
   * this particular bitmap.
   */
  newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
  if (bmPtr->gc != NULL) {
    Tk_FreeGC(graphPtr->display, bmPtr->gc);
  }
  bmPtr->gc = newGC;

  /* Create the background GC containing the fill color. */

  if (bmPtr->fillColor != NULL) {
    gcValues.foreground = bmPtr->fillColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (bmPtr->fillGC != NULL) {
      Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
    }
    bmPtr->fillGC = newGC;
  }

  markerPtr->flags |= MAP_ITEM;
  if (markerPtr->drawUnder) {
    graphPtr->flags |= CACHE_DIRTY;
  }

  Blt_EventuallyRedrawGraph(graphPtr);
  return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MapBitmapProc --
 *
 * 	This procedure gets called each time the layout of the graph changes.
 * 	The x, y window coordinates of the bitmap marker are saved in the
 * 	marker structure.
 *
 *	Additionly, if no background color was specified, the
 *	GCTileStipXOrigin and GCTileStipYOrigin attributes are set in the
 *	private GC.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Window coordinates are saved and if no background color was set, the
 *	GC stipple origins are changed to calculated window coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
MapBitmapProc(Marker *markerPtr)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  Region2d extents;
  Graph* graphPtr = markerPtr->obj.graphPtr;
  Point2d anchorPt;
  Point2d corner1, corner2;
  int destWidth, destHeight;
  int srcWidth, srcHeight;
  int i;

  if (bmPtr->srcBitmap == None) {
    return;
  }
  if (bmPtr->destBitmap != None) {
    Tk_FreePixmap(graphPtr->display, bmPtr->destBitmap);
    bmPtr->destBitmap = None;
  }
  /* 
   * Collect the coordinates.  The number of coordinates will determine the
   * calculations to be made.
   * 
   *	   x1 y1	A single pair of X-Y coordinates.  They represent
   *			the anchor position of the bitmap.  
   *
   *	x1 y1 x2 y2	Two pairs of X-Y coordinates.  They represent
   *			two opposite corners of a bounding rectangle. The
   *			bitmap is possibly rotated and scaled to fit into
   *			this box.
   *
   */   
  Tk_SizeOfBitmap(graphPtr->display, bmPtr->srcBitmap, &srcWidth, 
		  &srcHeight);
  corner1 = MapPoint(markerPtr->worldPts, &markerPtr->axes);
  if (markerPtr->nWorldPts > 1) {
    double hold;

    corner2 = MapPoint(markerPtr->worldPts + 1, &markerPtr->axes);
    /* Flip the corners if necessary */
    if (corner1.x > corner2.x) {
      hold = corner1.x, corner1.x = corner2.x, corner2.x = hold;
    }
    if (corner1.y > corner2.y) {
      hold = corner1.y, corner1.y = corner2.y, corner2.y = hold;
    }
  } else {
    corner2.x = corner1.x + srcWidth - 1;
    corner2.y = corner1.y + srcHeight - 1;
  }
  destWidth = (int)(corner2.x - corner1.x) + 1;
  destHeight = (int)(corner2.y - corner1.y) + 1;

  if (markerPtr->nWorldPts == 1) {
    anchorPt = Blt_AnchorPoint(corner1.x, corner1.y, (double)destWidth, 
			       (double)destHeight, bmPtr->anchor);
  } else {
    anchorPt = corner1;
  }
  anchorPt.x += markerPtr->xOffset;
  anchorPt.y += markerPtr->yOffset;

  /* Check if the bitmap sits at least partially in the plot area. */
  extents.left   = anchorPt.x;
  extents.top    = anchorPt.y;
  extents.right  = anchorPt.x + destWidth - 1;
  extents.bottom = anchorPt.y + destHeight - 1;
  markerPtr->clipped = BoxesDontOverlap(graphPtr, &extents);
  if (markerPtr->clipped) {
    return;				/* Bitmap is offscreen. Don't generate
					 * rotated or scaled bitmaps. */
  }

  /*  
   * Scale the bitmap if necessary. It's a little tricky because we only
   * want to scale what's visible on the screen, not the entire bitmap.
   */
  if ((bmPtr->angle != 0.0f) || (destWidth != srcWidth) || 
      (destHeight != srcHeight)) {
    int regionX, regionY, regionWidth, regionHeight; 
    double left, right, top, bottom;

    /* Ignore parts of the bitmap outside of the plot area. */
    left   = MAX(graphPtr->left, extents.left);
    right  = MIN(graphPtr->right, extents.right);
    top    = MAX(graphPtr->top, extents.top);
    bottom = MIN(graphPtr->bottom, extents.bottom);

    /* Determine the portion of the scaled bitmap to display. */
    regionX = regionY = 0;
    if (graphPtr->left > extents.left) {
      regionX = (int)(graphPtr->left - extents.left);
    }
    if (graphPtr->top > extents.top) {
      regionY = (int)(graphPtr->top - extents.top);
    }	    
    regionWidth = (int)(right - left) + 1;
    regionHeight = (int)(bottom - top) + 1;
	
    anchorPt.x = left;
    anchorPt.y = top;
    bmPtr->destBitmap = Blt_ScaleRotateBitmapArea(graphPtr->tkwin, 
						  bmPtr->srcBitmap, srcWidth, srcHeight, regionX, regionY, 
						  regionWidth, regionHeight, destWidth, destHeight, bmPtr->angle);
    bmPtr->destWidth = regionWidth;
    bmPtr->destHeight = regionHeight;
  } else {
    bmPtr->destWidth = srcWidth;
    bmPtr->destHeight = srcHeight;
    bmPtr->destBitmap = None;
  }
  bmPtr->anchorPt = anchorPt;
  {
    double xScale, yScale;
    double tx, ty;
    double rotWidth, rotHeight;
    Point2d polygon[5];
    int n;

    /* 
     * Compute a polygon to represent the background area of the bitmap.
     * This is needed for backgrounds of arbitrarily rotated bitmaps.  We
     * also use it to print a background in PostScript.
     */
    Blt_GetBoundingBox(srcWidth, srcHeight, bmPtr->angle, &rotWidth, 
		       &rotHeight, polygon);
    xScale = (double)destWidth / rotWidth;
    yScale = (double)destHeight / rotHeight;
	
    /* 
     * Adjust each point of the polygon. Both scale it to the new size and
     * translate it to the actual screen position of the bitmap.
     */
    tx = extents.left + destWidth * 0.5;
    ty = extents.top + destHeight * 0.5;
    for (i = 0; i < 4; i++) {
      polygon[i].x = (polygon[i].x * xScale) + tx;
      polygon[i].y = (polygon[i].y * yScale) + ty;
    }
    Blt_GraphExtents(graphPtr, &extents);
    n = Blt_PolyRectClip(&extents, polygon, 4, bmPtr->outline); 
    assert(n <= MAX_OUTLINE_POINTS);
    if (n < 3) { 
      memcpy(&bmPtr->outline, polygon, sizeof(Point2d) * 4);
      bmPtr->nOutlinePts = 4;
    } else {
      bmPtr->nOutlinePts = n;
    }
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * PointInBitmapProc --
 *
 *	Indicates if the given point is over the bitmap marker.  The area of
 *	the bitmap is the rectangle.
 *
 * Results:
 *	Returns 1 is the point is over the bitmap marker, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
static int
PointInBitmapProc(Marker *markerPtr, Point2d *samplePtr)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;

  if (bmPtr->srcBitmap == None) {
    return 0;
  }
  if (bmPtr->angle != 0.0f) {
    Point2d points[MAX_OUTLINE_POINTS];
    int i;

    /*  
     * Generate the bounding polygon (isolateral) for the bitmap and see
     * if the point is inside of it.
     */
    for (i = 0; i < bmPtr->nOutlinePts; i++) {
      points[i].x = bmPtr->outline[i].x + bmPtr->anchorPt.x;
      points[i].y = bmPtr->outline[i].y + bmPtr->anchorPt.y;
    }
    return Blt_PointInPolygon(samplePtr, points, bmPtr->nOutlinePts);
  }
  return ((samplePtr->x >= bmPtr->anchorPt.x) && 
	  (samplePtr->x < (bmPtr->anchorPt.x + bmPtr->destWidth)) &&
	  (samplePtr->y >= bmPtr->anchorPt.y) && 
	  (samplePtr->y < (bmPtr->anchorPt.y + bmPtr->destHeight)));
}


/*
 *---------------------------------------------------------------------------
 *
 * RegionInBitmapProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RegionInBitmapProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;

  if (markerPtr->nWorldPts < 1) {
    return FALSE;
  }
  if (bmPtr->angle != 0.0f) {
    Point2d points[MAX_OUTLINE_POINTS];
    int i;
	
    /*  
     * Generate the bounding polygon (isolateral) for the bitmap and see
     * if the point is inside of it.
     */
    for (i = 0; i < bmPtr->nOutlinePts; i++) {
      points[i].x = bmPtr->outline[i].x + bmPtr->anchorPt.x;
      points[i].y = bmPtr->outline[i].y + bmPtr->anchorPt.y;
    }
    return Blt_RegionInPolygon(extsPtr, points, bmPtr->nOutlinePts, 
			       enclosed);
  }
  if (enclosed) {
    return ((bmPtr->anchorPt.x >= extsPtr->left) &&
	    (bmPtr->anchorPt.y >= extsPtr->top) && 
	    ((bmPtr->anchorPt.x + bmPtr->destWidth) <= extsPtr->right) &&
	    ((bmPtr->anchorPt.y + bmPtr->destHeight) <= extsPtr->bottom));
  }
  return !((bmPtr->anchorPt.x >= extsPtr->right) ||
	   (bmPtr->anchorPt.y >= extsPtr->bottom) ||
	   ((bmPtr->anchorPt.x + bmPtr->destWidth) <= extsPtr->left) ||
	   ((bmPtr->anchorPt.y + bmPtr->destHeight) <= extsPtr->top));
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawBitmapProc --
 *
 *	Draws the bitmap marker that have a transparent of filled background.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	GC stipple origins are changed to current window coordinates.
 *	Commands are output to X to draw the marker in its current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawBitmapProc(Marker *markerPtr, Drawable drawable)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  double rangle;
  Pixmap bitmap;

  bitmap = GETBITMAP(bmPtr);
  if ((bitmap == None) || (bmPtr->destWidth < 1) || (bmPtr->destHeight < 1)) {
    return;
  }
  rangle = fmod(bmPtr->angle, 90.0);
  if ((bmPtr->fillColor == NULL) || (rangle != 0.0)) {

    /* 
     * If the bitmap is rotated and a filled background is required, then
     * a filled polygon is drawn before the bitmap.
     */
    if (bmPtr->fillColor != NULL) {
      int i;
      XPoint polygon[MAX_OUTLINE_POINTS];

      for (i = 0; i < bmPtr->nOutlinePts; i++) {
	polygon[i].x = (short int)bmPtr->outline[i].x;
	polygon[i].y = (short int)bmPtr->outline[i].y;
      }
      XFillPolygon(graphPtr->display, drawable, bmPtr->fillGC,
		   polygon, bmPtr->nOutlinePts, Convex, CoordModeOrigin);
    }
    XSetClipMask(graphPtr->display, bmPtr->gc, bitmap);
    XSetClipOrigin(graphPtr->display, bmPtr->gc, (int)bmPtr->anchorPt.x, 
		   (int)bmPtr->anchorPt.y);
  } else {
    XSetClipMask(graphPtr->display, bmPtr->gc, None);
    XSetClipOrigin(graphPtr->display, bmPtr->gc, 0, 0);
  }
  XCopyPlane(graphPtr->display, bitmap, drawable, bmPtr->gc, 0, 0,
	     bmPtr->destWidth, bmPtr->destHeight, (int)bmPtr->anchorPt.x, 
	     (int)bmPtr->anchorPt.y, 1);
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapToPostscriptProc --
 *
 *	Generates PostScript to print a bitmap marker.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
BitmapToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  Graph* graphPtr = markerPtr->obj.graphPtr;
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  Pixmap bitmap;

  bitmap = GETBITMAP(bmPtr);
  if ((bitmap == None) || (bmPtr->destWidth < 1) || (bmPtr->destHeight < 1)) {
    return;				/* No bitmap to display. */
  }
  if (bmPtr->fillColor != NULL) {
    Blt_Ps_XSetBackground(ps, bmPtr->fillColor);
    Blt_Ps_XFillPolygon(ps, bmPtr->outline, 4);
  }
  Blt_Ps_XSetForeground(ps, bmPtr->outlineColor);

  Blt_Ps_Format(ps,
		"  gsave\n    %g %g translate\n    %d %d scale\n", 
		bmPtr->anchorPt.x, bmPtr->anchorPt.y + bmPtr->destHeight, 
		bmPtr->destWidth, -bmPtr->destHeight);
  Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {",
		bmPtr->destWidth, bmPtr->destHeight, bmPtr->destWidth, 
		-bmPtr->destHeight, bmPtr->destHeight);
  Blt_Ps_XSetBitmapData(ps, graphPtr->display, bitmap,
			bmPtr->destWidth, bmPtr->destHeight);
  Blt_Ps_VarAppend(ps, 
		   "    } imagemask\n",
		   "grestore\n", (char*)NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeBitmapProc --
 *
 *	Releases the memory and attributes of the bitmap marker.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Bitmap attributes (GCs, colors, bitmap, etc) get destroyed.  Memory is
 *	released, X resources are freed, and the graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeBitmapProc(Marker *markerPtr)
{
  BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  if (bmPtr->gc != NULL) {
    Tk_FreeGC(graphPtr->display, bmPtr->gc);
  }
  if (bmPtr->fillGC != NULL) {
    Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
  }
  if (bmPtr->destBitmap != None) {
    Tk_FreePixmap(graphPtr->display, bmPtr->destBitmap);
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateBitmapProc --
 *
 *	Allocate memory and initialize methods for the new bitmap marker.
 *
 * Results:
 *	The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *	Memory is allocated for the bitmap marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
CreateBitmapProc(void)
{
  BitmapMarker *bmPtr;

  bmPtr = calloc(1, sizeof(BitmapMarker));
  bmPtr->classPtr = &bitmapMarkerClass;
  return (Marker *)bmPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureTextProc --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a text marker.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for markerPtr; old resources get freed, if there were any.  The
 *	marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureTextProc(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * MapTextProc --
 *
 *	Calculate the layout position for a text marker.  Positional information
 *	is saved in the marker.  If the text is rotated, a bitmap containing the
 *	text is created.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If no background color has been specified, the GC stipple origins are
 *	changed to current window coordinates. For both rotated and
 *	non-rotated text, if any old bitmap is leftover, it is freed.
 *
 *---------------------------------------------------------------------------
 */
static void
MapTextProc(Marker *markerPtr)
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
  anchorPt = MapPoint(markerPtr->worldPts, &markerPtr->axes);
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
  markerPtr->clipped = BoxesDontOverlap(graphPtr, &extents);
  tmPtr->anchorPt = anchorPt;

}

static int
PointInTextProc(Marker *markerPtr, Point2d *samplePtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * RegionInTextProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RegionInTextProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
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

/*
 *---------------------------------------------------------------------------
 *
 * DrawTextProc --
 *
 *	Draws the text marker on the graph.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to draw the marker in its current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTextProc(Marker *markerPtr, Drawable drawable) 
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

/*
 *---------------------------------------------------------------------------
 *
 * TextToPostscriptProc --
 *
 *	Outputs PostScript commands to draw a text marker at a given x,y
 *	coordinate, rotation, anchor, and font.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	PostScript font and color settings are changed.
 *
 *---------------------------------------------------------------------------
 */
static void
TextToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
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

/*
 *---------------------------------------------------------------------------
 *
 * FreeTextProc --
 *
 *	Destroys the structure containing the attributes of the text marker.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Text attributes (GCs, colors, stipple, font, etc) get destroyed.
 *	Memory is released, X resources are freed, and the graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeTextProc(Marker *markerPtr)
{
  TextMarker *tmPtr = (TextMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;

  Blt_Ts_FreeStyle(graphPtr->display, &tmPtr->style);
}

/*
 *---------------------------------------------------------------------------

 * CreateTextProc --
 *
 *	Allocate memory and initialize methods for the new text marker.
 *
 * Results:
 *	The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *	Memory is allocated for the text marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
CreateTextProc(void)
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureWindowProc --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a window
 *	marker.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as window pathname, placement,
 *	etc. get set for markerPtr; old resources get freed, if there were
 *	any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureWindowProc(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * MapWindowProc --
 *
 *	Calculate the layout position for a window marker.  Positional
 *	information is saved in the marker.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapWindowProc(Marker *markerPtr)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;
  Graph* graphPtr = markerPtr->obj.graphPtr;
  Point2d anchorPt;
  Region2d extents;
  int width, height;

  if (wmPtr->child == (Tk_Window)NULL) {
    return;
  }
  anchorPt = MapPoint(markerPtr->worldPts, &markerPtr->axes);

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
  markerPtr->clipped = BoxesDontOverlap(graphPtr, &extents);
}

/*
 *---------------------------------------------------------------------------
 *
 * PointInWindowProc --
 *
 *---------------------------------------------------------------------------
 */
static int
PointInWindowProc(Marker *markerPtr, Point2d *samplePtr)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  return ((samplePtr->x >= wmPtr->anchorPt.x) && 
	  (samplePtr->x < (wmPtr->anchorPt.x + wmPtr->width)) &&
	  (samplePtr->y >= wmPtr->anchorPt.y) && 
	  (samplePtr->y < (wmPtr->anchorPt.y + wmPtr->height)));
}

/*
 *---------------------------------------------------------------------------
 *
 * RegionInWindowProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RegionInWindowProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
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

/*
 *---------------------------------------------------------------------------
 *
 * DrawWindowProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
DrawWindowProc(Marker *markerPtr, Drawable drawable)
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

/*
 *---------------------------------------------------------------------------
 *
 * WindowToPostscriptProc --
 *
 *---------------------------------------------------------------------------
 */
static void
WindowToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  if (wmPtr->child == NULL) {
    return;
  }
  if (Tk_IsMapped(wmPtr->child)) {
    //	Blt_Ps_XDrawWindow(ps, wmPtr->child, wmPtr->anchorPt.x, wmPtr->anchorPt.y);
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeWindowProc --
 *
 *	Destroys the structure containing the attributes of the window *
 *	marker.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Window is destroyed and removed from the screen.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeWindowProc(Marker *markerPtr)
{
  WindowMarker *wmPtr = (WindowMarker *)markerPtr;

  if (wmPtr->child != NULL) {
    Tk_DeleteEventHandler(wmPtr->child, StructureNotifyMask,
			  ChildEventProc, wmPtr);
    Tk_ManageGeometry(wmPtr->child, (Tk_GeomMgr *) 0, (ClientData)0);
    Tk_DestroyWindow(wmPtr->child);
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateWindowProc --
 *
 *	Allocate memory and initialize methods for the new window marker.
 *
 * Results:
 *	The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *	Memory is allocated for the window marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
CreateWindowProc(void)
{
  WindowMarker *wmPtr;

  wmPtr = calloc(1, sizeof(WindowMarker));
  wmPtr->classPtr = &windowMarkerClass;
  return (Marker *)wmPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildEventProc --
 *
 *	This procedure is invoked whenever StructureNotify events occur for a
 *	window that's managed as part of a graph window marker. This
 *	procedure's only purpose is to clean up when windows are deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is disassociated from the window item when it is
 *	deleted.
 *
 *---------------------------------------------------------------------------
 */
static void
ChildEventProc(ClientData clientData, XEvent *eventPtr)
{
  WindowMarker *wmPtr = clientData;

  if (eventPtr->type == DestroyNotify) {
    wmPtr->child = NULL;
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildGeometryProc --
 *
 *	This procedure is invoked whenever a window that's associated with a
 *	window item changes its requested dimensions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size and location on the window of the window may change,
 *	depending on the options specified for the window item.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
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

/*
 *---------------------------------------------------------------------------
 *
 * ChildCustodyProc --
 *
 *	This procedure is invoked when an embedded window has been stolen by
 *	another geometry manager.  The information and memory associated with
 *	the widget is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the graph to be redrawn without the embedded widget at
 *	the next idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ChildCustodyProc(ClientData clientData, Tk_Window tkwin)
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

/*
 *---------------------------------------------------------------------------
 *
 * MapLineProc --
 *
 *	Calculate the layout position for a line marker.  Positional
 *	information is saved in the marker.  The line positions are stored in
 *	an array of points (malloc'ed).
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapLineProc(Marker *markerPtr)
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
  p = MapPoint(srcPtr, &markerPtr->axes);
  p.x += markerPtr->xOffset;
  p.y += markerPtr->yOffset;

  segPtr = segments;
  for (srcPtr++, pend = markerPtr->worldPts + markerPtr->nWorldPts; 
       srcPtr < pend; srcPtr++) {
    Point2d next;

    next = MapPoint(srcPtr, &markerPtr->axes);
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
			     (double)markerPtr->obj.graphPtr->halo);
}

/*
 *---------------------------------------------------------------------------
 *
 * RegionInLineProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RegionInLineProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  if (markerPtr->nWorldPts < 2) {
    return FALSE;
  }
  if (enclosed) {
    Point2d *pp, *pend;

    for (pp = markerPtr->worldPts, pend = pp + markerPtr->nWorldPts; 
	 pp < pend; pp++) {
      Point2d p;

      p = MapPoint(pp, &markerPtr->axes);
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

      p = MapPoint(pp, &markerPtr->axes);
      q = MapPoint(pp + 1, &markerPtr->axes);
      if (Blt_LineRectClip(extsPtr, &p, &q)) {
	count++;
      }
    }
    return (count > 0);		/* At least 1 segment passes through
				 * region. */
  }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawLineProc --
 *
 *---------------------------------------------------------------------------
 */
static void
DrawLineProc(Marker *markerPtr, Drawable drawable)
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureLineProc --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a line marker.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as line width, colors, dashes,
 *	etc. get set for markerPtr; old resources get freed, if there were
 *	any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureLineProc(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * LineToPostscriptProc --
 *
 *	Prints postscript commands to display the connect line.  Dashed lines
 *	need to be handled specially, especially if a background color is
 *	designated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	PostScript output commands are saved in the interpreter
 *	(infoPtr->interp) result field.
 *
 *---------------------------------------------------------------------------
 */
static void
LineToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
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

/*
 *---------------------------------------------------------------------------
 *
 * FreeLineProc --
 *
 *	Destroys the structure and attributes of a line marker.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Line attributes (GCs, colors, stipple, etc) get released.  Memory is
 *	deallocated, X resources are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeLineProc(Marker *markerPtr)
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


/*
 *---------------------------------------------------------------------------
 *
 * CreateLineProc --
 *
 *	Allocate memory and initialize methods for a new line marker.
 *
 * Results:
 *	The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *	Memory is allocated for the line marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
CreateLineProc(void)
{
  LineMarker *lmPtr;

  lmPtr = calloc(1, sizeof(LineMarker));
  lmPtr->classPtr = &lineMarkerClass;
  lmPtr->xor = FALSE;
  lmPtr->capStyle = CapButt;
  lmPtr->joinStyle = JoinMiter;
  return (Marker *)lmPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * MapPolygonProc --
 *
 *	Calculate the layout position for a polygon marker.  Positional
 *	information is saved in the polygon in an array of points (malloc'ed).
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapPolygonProc(Marker *markerPtr)
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
      *dp = MapPoint(sp, &markerPtr->axes);
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

static int
PointInPolygonProc(Marker *markerPtr, Point2d *samplePtr)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

  if ((markerPtr->nWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
    return Blt_PointInPolygon(samplePtr, pmPtr->screenPts, 
			      markerPtr->nWorldPts + 1);
  }
  return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * RegionInPolygonProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RegionInPolygonProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
  PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    
  if ((markerPtr->nWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
    return Blt_RegionInPolygon(extsPtr, pmPtr->screenPts, 
			       markerPtr->nWorldPts, enclosed);
  }
  return FALSE;
}

static void
DrawPolygonProc(Marker *markerPtr, Drawable drawable)
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


static void
PolygonToPostscriptProc(Marker *markerPtr, Blt_Ps ps)
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigurePolygonProc --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) a polygon
 *	marker.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side effects:
 *	Configuration information, such as polygon color, dashes, fillstyle,
 *	etc. get set for markerPtr; old resources get freed, if there were
 *	any.  The marker is eventually redisplayed.
 *
 * -------------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
ConfigurePolygonProc(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * FreePolygonProc --
 *
 *	Release memory and resources allocated for the polygon element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the polygon element is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
FreePolygonProc(Marker *markerPtr)
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

/*
 *---------------------------------------------------------------------------
 *
 * CreatePolygonProc --
 *
 *	Allocate memory and initialize methods for the new polygon marker.
 *
 * Results:
 *	The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *	Memory is allocated for the polygon marker structure.
 *
 * -------------------------------------------------------------------------- 
 */
static Marker *
CreatePolygonProc(void)
{
  PolygonMarker *pmPtr;

  pmPtr = calloc(1, sizeof(PolygonMarker));
  pmPtr->classPtr = &polygonMarkerClass;
  pmPtr->capStyle = CapButt;
  pmPtr->joinStyle = JoinMiter;
  return (Marker *)pmPtr;
}

static int
GetMarkerFromObj(Tcl_Interp* interp, Graph* graphPtr, Tcl_Obj *objPtr,
		 Marker **markerPtrPtr)
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


static int
RenameMarker(Graph* graphPtr, Marker *markerPtr, const char* oldName, 
	     const char* newName)
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

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns a list of marker identifiers in interp->result;
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	.g element bind elemName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	This procedure creates and initializes a new marker.
 *
 * Results:
 *	The return value is a pointer to a structure describing the new
 *	element.  If an error occurred, then the return value is NULL and an
 *	error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated, etc.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Deletes the marker given by markerId.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new display list.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 * 	Find the legend entry from the given argument.  The argument can be
 * 	either a screen position "@x,y" or the name of an element.
 *
 *	I don't know how useful it is to test with the name of an element.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * RelinkOp --
 *
 *	Reorders the marker (given by the first name) before/after the another
 *	marker (given by the second name) in the marker display list.  If no
 *	second name is given, the marker is placed at the beginning/end of the
 *	list.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new display list.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RelinkOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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


/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *	Returns if marker by a given ID currently exists.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FindOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *	Returns if marker by a given ID currently exists.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
{
  Tcl_HashEntry *hPtr;

  hPtr = Tcl_FindHashEntry(&graphPtr->markers.table, Tcl_GetString(objv[3]));
  Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
  return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *	Returns a symbolic name for the type of the marker whose ID is given.
 *
 * Results:
 *	A standard TCL result. interp->result will contain the symbolic type
 *	of the marker.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(Graph* graphPtr, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
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

/* Public routines */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MarkerOp --
 *
 *	This procedure is invoked to process the TCL command that corresponds
 *	to a widget managed by this module.  See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

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

/*ARGSUSED*/
int
Blt_MarkerOp(Graph* graphPtr, Tcl_Interp* interp, int objc, 
	     Tcl_Obj* const objv[])
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

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MarkersToPostScript --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MarkersToPostScript(Graph* graphPtr, Blt_Ps ps, int under)
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

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawMarkers --
 *
 *	Calls the individual drawing routines (based on marker type) for each
 *	marker in the display list.
 *
 *	A marker will not be drawn if
 *
 *	1) An element linked to the marker (indicated by elemName) is
 *	   currently hidden.
 *
 *	2) No coordinates have been specified for the marker.
 *
 *	3) The marker is requesting to be drawn at a different level
 *	   (above/below the elements) from the current mode.
 *
 *	4) The marker is configured as hidden (-hide option).
 *
 *	5) The marker isn't visible in the current viewport (i.e. clipped).
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Markers are drawn into the drawable (pixmap) which will eventually
 *	be displayed in the graph window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawMarkers(Graph* graphPtr, Drawable drawable, int under)
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

void
Blt_MapMarkers(Graph* graphPtr)
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

void
Blt_DestroyMarkers(Graph* graphPtr)
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

Marker *
Blt_NearestMarker(
		  Graph* graphPtr,
		  int x, int y,			/* Screen coordinates */
		  int under)
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

ClientData
Blt_MakeMarkerTag(Graph* graphPtr, const char* tagName)
{
  Tcl_HashEntry *hPtr;
  int isNew;

  assert(tagName != NULL);
  hPtr = Tcl_CreateHashEntry(&graphPtr->markers.tagTable, tagName, &isNew);
  return Tcl_GetHashKey(&graphPtr->markers.tagTable, hPtr);
}


