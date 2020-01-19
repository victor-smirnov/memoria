//  filesystem/string_file.hpp  --------------------------------------------------------//

//  Copyright Beman Dawes 2015

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

#ifndef MEMORIA_BOOST_FILESYSTEM_STRING_FILE_HPP
#define MEMORIA_BOOST_FILESYSTEM_STRING_FILE_HPP

#include <string>
#include <memoria/filesystem/fstream.hpp>
#include <memoria/filesystem/operations.hpp>

namespace memoria {
namespace filesystem
{
inline
void save_string_file(const path& p, const std::string& str)
{
  filesystem::ofstream file;
  file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  file.open(p, std::ios_base::binary);
  file.write(str.c_str(), str.size());
}

inline
void load_string_file(const path& p, std::string& str)
{
  filesystem::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(p, std::ios_base::binary);
  std::size_t sz = static_cast<std::size_t>(filesystem::file_size(p));
  str.resize(sz, '\0');
  file.read(&str[0], sz);
}
}  // namespace filesystem
}  // namespace memoria

#endif  // include guard
