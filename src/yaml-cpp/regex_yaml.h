
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
#include <vector>

#include <memoria/v1/yaml-cpp/dll.h>

namespace memoria {
namespace v1 {
namespace YAML {
class Stream;

enum REGEX_OP {
  REGEX_EMPTY,
  REGEX_MATCH,
  REGEX_RANGE,
  REGEX_OR,
  REGEX_AND,
  REGEX_NOT,
  REGEX_SEQ
};

// simplified regular expressions
// . Only straightforward matches (no repeated characters)
// . Only matches from start of string
class YAML_CPP_API RegEx {
 public:
  RegEx();
  RegEx(char ch);
  RegEx(char a, char z);
  RegEx(const std::string& str, REGEX_OP op = REGEX_SEQ);
  ~RegEx() {}

  friend YAML_CPP_API RegEx operator!(const RegEx& ex);
  friend YAML_CPP_API RegEx operator||(const RegEx& ex1, const RegEx& ex2);
  friend YAML_CPP_API RegEx operator&&(const RegEx& ex1, const RegEx& ex2);
  friend YAML_CPP_API RegEx operator+(const RegEx& ex1, const RegEx& ex2);

  bool Matches(char ch) const;
  bool Matches(const std::string& str) const;
  bool Matches(const Stream& in) const;
  template <typename Source>
  bool Matches(const Source& source) const;

  int Match(const std::string& str) const;
  int Match(const Stream& in) const;
  template <typename Source>
  int Match(const Source& source) const;

 private:
  RegEx(REGEX_OP op);

  template <typename Source>
  bool IsValidSource(const Source& source) const;
  template <typename Source>
  int MatchUnchecked(const Source& source) const;

  template <typename Source>
  int MatchOpEmpty(const Source& source) const;
  template <typename Source>
  int MatchOpMatch(const Source& source) const;
  template <typename Source>
  int MatchOpRange(const Source& source) const;
  template <typename Source>
  int MatchOpOr(const Source& source) const;
  template <typename Source>
  int MatchOpAnd(const Source& source) const;
  template <typename Source>
  int MatchOpNot(const Source& source) const;
  template <typename Source>
  int MatchOpSeq(const Source& source) const;

 private:
  REGEX_OP m_op;
  char m_a, m_z;
  std::vector<RegEx> m_params;
};
}}}

#include "regeximpl.h"


