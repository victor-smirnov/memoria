
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


#include <cstddef>

namespace memoria {
namespace v1 {
namespace YAML {
class StringCharSource {
 public:
  StringCharSource(const char* str, std::size_t size)
      : m_str(str), m_size(size), m_offset(0) {}

  operator bool() const { return m_offset < m_size; }
  char operator[](std::size_t i) const { return m_str[m_offset + i]; }
  bool operator!() const { return !static_cast<bool>(*this); }

  const StringCharSource operator+(int i) const {
    StringCharSource source(*this);
    if (static_cast<int>(source.m_offset) + i >= 0)
      source.m_offset += i;
    else
      source.m_offset = 0;
    return source;
  }

  StringCharSource& operator++() {
    ++m_offset;
    return *this;
  }

  StringCharSource& operator+=(std::size_t offset) {
    m_offset += offset;
    return *this;
  }

 private:
  const char* m_str;
  std::size_t m_size;
  std::size_t m_offset;
};
}}}

