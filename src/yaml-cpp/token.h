
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


#include <memoria/v1/yaml-cpp/mark.h>
#include <iostream>
#include <string>
#include <vector>

namespace memoria {
namespace v1 {
namespace YAML {
const std::string TokenNames[] = {
    "DIRECTIVE", "DOC_START", "DOC_END", "BLOCK_SEQ_START", "BLOCK_MAP_START",
    "BLOCK_SEQ_END", "BLOCK_MAP_END", "BLOCK_ENTRY", "FLOW_SEQ_START",
    "FLOW_MAP_START", "FLOW_SEQ_END", "FLOW_MAP_END", "FLOW_MAP_COMPACT",
    "FLOW_ENTRY", "KEY", "VALUE", "ANCHOR", "ALIAS", "TAG", "SCALAR"};

struct Token {
  // enums
  enum STATUS { VALID, INVALID, UNVERIFIED };
  enum TYPE {
    DIRECTIVE,
    DOC_START,
    DOC_END,
    BLOCK_SEQ_START,
    BLOCK_MAP_START,
    BLOCK_SEQ_END,
    BLOCK_MAP_END,
    BLOCK_ENTRY,
    FLOW_SEQ_START,
    FLOW_MAP_START,
    FLOW_SEQ_END,
    FLOW_MAP_END,
    FLOW_MAP_COMPACT,
    FLOW_ENTRY,
    KEY,
    VALUE,
    ANCHOR,
    ALIAS,
    TAG,
    PLAIN_SCALAR,
    NON_PLAIN_SCALAR
  };

  // data
  Token(TYPE type_, const Mark& mark_)
      : status(VALID), type(type_), mark(mark_), data(0) {}

  friend std::ostream& operator<<(std::ostream& out, const Token& token) {
    out << TokenNames[token.type] << std::string(": ") << token.value;
    for (std::size_t i = 0; i < token.params.size(); i++)
      out << std::string(" ") << token.params[i];
    return out;
  }

  STATUS status;
  TYPE type;
  Mark mark;
  std::string value;
  std::vector<std::string> params;
  int data;
};
}}}

