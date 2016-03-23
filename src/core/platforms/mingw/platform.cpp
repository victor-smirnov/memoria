
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/core/tools/platform.hpp>


namespace memoria {

using namespace std;

String Platform::getPathSeparator() {
    return ";";
}

String Platform::getLineSeparator() {
    return "\r\n";
}

String Platform::getFilePathSeparator() {
    return "/";
}

}

