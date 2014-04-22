/*
 * Smithsonian Astrophysical Observatory, Cambridge, MA, USA
 * This code has been modified under the terms listed below and is made
 * available under the same terms.
 */

/*
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __BltGrHairs_h__
#define __BltGrHairs_h__

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include <tk.h>

#include "bltGrMisc.h"

class Graph;

typedef struct {
  XColor* colorPtr;
  Blt_Dashes dashes;
  int hide;
  int lineWidth;
  int x;
  int y;
} CrosshairsOptions;

class Crosshairs {
 protected:
  Tk_OptionTable optionTable_;
  void* ops_;

  GC gc_;

 public:
  Graph* graphPtr_;
  int visible_;
  XSegment segArr_[2];

 public:
  Crosshairs(Graph*);
  virtual ~Crosshairs();

  void configure();
  void on();
  void off();
  void enable();
  void disable();

  Tk_OptionTable optionTable() {return optionTable_;}
  void* ops() {return ops_;}
};

#endif
