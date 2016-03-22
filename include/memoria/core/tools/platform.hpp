
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/strings/string.hpp>

#include <vector>

namespace memoria{

class MEMORIA_API Platform {
public:
    static String getLineSeparator();
    static String getPathSeparator();
    static String getFilePathSeparator();
};

}
