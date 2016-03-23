
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>

#include <vector>

namespace memoria {
namespace v1 {

class Platform {
public:
    static String getLineSeparator();
    static String getPathSeparator();
    static String getFilePathSeparator();
};

}}