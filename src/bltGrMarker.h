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

#ifndef __bltgrmarker_h__
#define __bltgrmarker_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

extern "C" {
#include "bltGraph.h"
};

namespace Blt {

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
  protected:
    ClassId classId_;
    const char *name_;
    const char *className_;
    Tk_OptionTable optionTable_;
    void* ops_;
    Graph* graphPtr_;
    Tcl_HashEntry* hashPtr_;
    int clipped_;

  public:
    Blt_ChainLink link;
    unsigned int flags;		

  private:
    double HMap(Axis*, double);
    double VMap(Axis*, double);

  protected:
    Point2d mapPoint(Point2d*, Axis2d*);
    int boxesDontOverlap(Graph*, Region2d*);

  public:
    Marker(Graph*, const char*, Tcl_HashEntry*);
    virtual ~Marker();

    virtual int configure() =0;
    virtual void draw(Drawable) =0;
    virtual void map() =0;
    virtual int pointIn(Point2d*) =0;
    virtual int regionIn(Region2d*, int) =0;
    virtual void postscript(Blt_Ps) =0;

    ClassId classId() {return classId_;}
    const char* name() {return name_;}
    const char* className() {return className_;}
    int clipped() {return clipped_;}
    Tk_OptionTable optionTable() {return optionTable_;}
    void* ops() {return ops_;}
  };

};

#endif
