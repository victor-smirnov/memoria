
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/core/tools/file.hpp>

namespace memoria {

File::File(StringRef path): path_(NormalizePath(path)) {}
File::File(const File& file): path_(file.GetPath()) {}

File::~File() throw() {}

}
