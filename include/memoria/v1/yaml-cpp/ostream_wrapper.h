
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
class YAML_CPP_API ostream_wrapper {
 public:
  ostream_wrapper();
  explicit ostream_wrapper(std::ostream& stream);
  ~ostream_wrapper();

  void write(const std::string& str);
  void write(const char* str, std::size_t size);

  void set_comment() { m_comment = true; }

  const char* str() const {
    if (m_pStream) {
      return 0;
    } else {
      m_buffer[m_pos] = '\0';
      return &m_buffer[0];
    }
  }

  std::size_t row() const { return m_row; }
  std::size_t col() const { return m_col; }
  std::size_t pos() const { return m_pos; }
  bool comment() const { return m_comment; }

 private:
  void update_pos(char ch);

 private:
  mutable std::vector<char> m_buffer;
  std::ostream* const m_pStream;

  std::size_t m_pos;
  std::size_t m_row, m_col;
  bool m_comment;
};

template <std::size_t N>
inline ostream_wrapper& operator<<(ostream_wrapper& stream,
                                   const char(&str)[N]) {
  stream.write(str, N - 1);
  return stream;
}

inline ostream_wrapper& operator<<(ostream_wrapper& stream,
                                   const std::string& str) {
  stream.write(str);
  return stream;
}

inline ostream_wrapper& operator<<(ostream_wrapper& stream, char ch) {
  stream.write(&ch, 1);
  return stream;
}
}}}
