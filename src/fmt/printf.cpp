/*
 Formatting library for C++

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include <memoria/v1/fmt/format.hpp>
#include <memoria/v1/fmt/printf.hpp>

namespace memoria {
namespace v1 {
namespace fmt {

template <typename Char>
void printf(BasicWriter<Char> &w, BasicCStringRef<Char> format, ArgList args);

int fprintf(std::FILE *f, CStringRef format, ArgList args) {
  MemoryWriter w;
  printf(w, format, args);
  std::size_t size = w.size();
  return std::fwrite(w.data(), 1, size, f) < size ? -1 : static_cast<int>(size);
}



template void PrintfFormatter<char>::format(CStringRef format);
template void PrintfFormatter<wchar_t>::format(WCStringRef format);


}  // namespace fmt
}}