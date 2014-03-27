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

#ifndef _BLT_GR_MARKER_H
#define _BLT_GR_MARKER_H

#define MAX_OUTLINE_POINTS	12

namespace Blt {
  class Marker;

typedef Marker* (MarkerCreateProc)(Graph*);
typedef void (MarkerDrawProc)(Marker* markerPtr, Drawable drawable);
typedef void (MarkerFreeProc)(Marker* markerPtr);
typedef int (MarkerConfigProc)(Marker* markerPtr);
typedef void (MarkerMapProc)(Marker* markerPtr);
typedef void (MarkerPostscriptProc)(Marker* markerPtr, Blt_Ps ps);
typedef int (MarkerPointProc)(Marker* markerPtr, Point2d *samplePtr);
typedef int (MarkerRegionProc)(Marker* markerPtr, Region2d *extsPtr, int enclosed);

typedef struct {
  Tk_OptionSpec *optionSpecs;
  MarkerConfigProc *configProc;
  MarkerDrawProc *drawProc;
  MarkerFreeProc *freeProc;
  MarkerMapProc *mapProc;
  MarkerPointProc *pointProc;
  MarkerRegionProc *regionProc;
  MarkerPostscriptProc *postscriptProc;
}  MarkerClass;

typedef struct {
  Point2d* points;
  int num;
} Coords;

typedef struct {
  const char** tags;
  Coords* worldPts;
  const char* elemName;
  Axis2d axes;
  int hide;
  int state;
  int drawUnder;
  int xOffset;
  int yOffset;
} MarkerOptions;

 class Marker {
 public:
   GraphObj obj;
   MarkerClass *classPtr;
   Tk_OptionTable optionTable;
   Tcl_HashEntry* hashPtr;
   Blt_ChainLink link;
   int clipped;
   unsigned int flags;		

   void* ops;

 public:
   Marker(Graph*, const char*);
   virtual ~Marker();
 };

};

Point2d Blt_MapPoint(Point2d *pointPtr, Axis2d *axesPtr);
void Blt_FreeMarker(char*);
int Blt_BoxesDontOverlap(Graph* graphPtr, Region2d *extsPtr);

extern Tk_ObjCustomOption coordsObjOption;
extern Tk_ObjCustomOption capStyleObjOption;
extern Tk_ObjCustomOption joinStyleObjOption;
extern Tk_ObjCustomOption xAxisObjOption;
extern Tk_ObjCustomOption yAxisObjOption;

#endif
