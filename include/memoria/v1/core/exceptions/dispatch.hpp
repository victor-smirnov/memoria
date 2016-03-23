
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/exceptions/memoria.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>

namespace memoria    {

using namespace std;

class MEMORIA_API DispatchException: public Exception {
    Int code1_;
    Int code2_;
public:

    DispatchException(const char* source, const string &message, Int code1 = 0, Int code2 = 0):
                Exception(source, message), code1_(code1), code2_(code2) {}

    DispatchException(const char* source, const SBuf &message, Int code1 = 0, Int code2 = 0):
                    Exception(source, message.str()), code1_(code1), code2_(code2) {}

    DispatchException(const char* source, Int code1, Int code2 = -1):
                Exception(source, "Invalid dispatch"), code1_(code1), code2_(code2) {}

    Int code1() const {
        return code1_;
    }

    Int code2() const {
        return code2_;
    }
};


}
