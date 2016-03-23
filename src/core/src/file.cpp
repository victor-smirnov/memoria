
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/v1/core/tools/file.hpp>

namespace memoria {
namespace v1 {

File::File(StringRef path): path_(normalizePath(path)) {}
File::File(const File& file): path_(file.getPath()) {}

File::~File() throw() {}

}}