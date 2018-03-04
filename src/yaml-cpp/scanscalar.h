
// Copyright (c) 2008-2015 Jesse Beder.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once


#include <string>

#include "regex_yaml.h"
#include "stream.h"


namespace memoria {
namespace v1 {
namespace YAML {
enum CHOMP { STRIP = -1, CLIP, KEEP };
enum ACTION { NONE, BREAK, THROW };
enum FOLD { DONT_FOLD, FOLD_BLOCK, FOLD_FLOW };

struct ScanScalarParams {
  ScanScalarParams()
      : end(nullptr),
        eatEnd(false),
        indent(0),
        detectIndent(false),
        eatLeadingWhitespace(0),
        escape(0),
        fold(DONT_FOLD),
        trimTrailingSpaces(0),
        chomp(CLIP),
        onDocIndicator(NONE),
        onTabInIndentation(NONE),
        leadingSpaces(false) {}

  // input:
  const RegEx* end;   // what condition ends this scalar?
                      // unowned.
  bool eatEnd;        // should we eat that condition when we see it?
  int indent;         // what level of indentation should be eaten and ignored?
  bool detectIndent;  // should we try to autodetect the indent?
  bool eatLeadingWhitespace;  // should we continue eating this delicious
                              // indentation after 'indent' spaces?
  char escape;  // what character do we escape on (i.e., slash or single quote)
                // (0 for none)
  FOLD fold;    // how do we fold line ends?
  bool trimTrailingSpaces;  // do we remove all trailing spaces (at the very
                            // end)
  CHOMP chomp;  // do we strip, clip, or keep trailing newlines (at the very
                // end)
  //   Note: strip means kill all, clip means keep at most one, keep means keep
  // all
  ACTION onDocIndicator;      // what do we do if we see a document indicator?
  ACTION onTabInIndentation;  // what do we do if we see a tab where we should
                              // be seeing indentation spaces

  // output:
  bool leadingSpaces;
};

std::string ScanScalar(Stream& INPUT, ScanScalarParams& info);
}}}


