
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

#include "emitterstate.h"
#include <memoria/v1/yaml-cpp/emittermanip.h>
#include <memoria/v1/yaml-cpp/ostream_wrapper.h>

namespace memoria {
namespace v1 {
namespace YAML {
class ostream_wrapper;
}  // namespace YAML

namespace YAML {
class Binary;

struct StringFormat {
  enum value { Plain, SingleQuoted, DoubleQuoted, Literal };
};

namespace Utils {
StringFormat::value ComputeStringFormat(const std::string& str,
                                        EMITTER_MANIP strFormat,
                                        FlowType::value flowType,
                                        bool escapeNonAscii);

bool WriteSingleQuotedString(ostream_wrapper& out, const std::string& str);
bool WriteDoubleQuotedString(ostream_wrapper& out, const std::string& str,
                             bool escapeNonAscii);
bool WriteLiteralString(ostream_wrapper& out, const std::string& str,
                        std::size_t indent);
bool WriteChar(ostream_wrapper& out, char ch);
bool WriteComment(ostream_wrapper& out, const std::string& str,
                  std::size_t postCommentIndent);
bool WriteAlias(ostream_wrapper& out, const std::string& str);
bool WriteAnchor(ostream_wrapper& out, const std::string& str);
bool WriteTag(ostream_wrapper& out, const std::string& str, bool verbatim);
bool WriteTagWithPrefix(ostream_wrapper& out, const std::string& prefix,
                        const std::string& tag);
bool WriteBinary(ostream_wrapper& out, const Binary& binary);
}
}}}


