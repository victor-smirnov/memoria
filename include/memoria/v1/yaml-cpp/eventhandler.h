
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

#include <memoria/v1/yaml-cpp/anchor.h>
#include <memoria/v1/yaml-cpp/emitterstyle.h>

namespace memoria {
namespace v1 {
namespace YAML {
struct Mark;

class EventHandler {
 public:
  virtual ~EventHandler() {}

  virtual void OnDocumentStart(const Mark& mark) = 0;
  virtual void OnDocumentEnd() = 0;

  virtual void OnNull(const Mark& mark, anchor_t anchor) = 0;
  virtual void OnAlias(const Mark& mark, anchor_t anchor) = 0;
  virtual void OnScalar(const Mark& mark, const std::string& tag,
                        anchor_t anchor, const std::string& value) = 0;

  virtual void OnSequenceStart(const Mark& mark, const std::string& tag,
                               anchor_t anchor, EmitterStyle::value style) = 0;
  virtual void OnSequenceEnd() = 0;

  virtual void OnMapStart(const Mark& mark, const std::string& tag,
                          anchor_t anchor, EmitterStyle::value style) = 0;
  virtual void OnMapEnd() = 0;
};
}}}

