
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/v1/core/exceptions/memoria.hpp>



#include <string>

namespace memoria {

class BoundsException: public Exception {

public:
    BoundsException(const char* source, StringRef message):
                Exception(source, message) {}

    BoundsException(const char* source, const SBuf& message):
                Exception(source, message) {}
};


}
