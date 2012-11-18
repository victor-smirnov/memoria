
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_VAPI_EXCEPTIONS_BOUNDS_HPP
#define _MEMORIA_VAPI_EXCEPTIONS_BOUNDS_HPP

#include <memoria/core/exceptions/memoria.hpp>
#include <string>

namespace memoria    {
namespace vapi       {

using namespace std;

class MEMORIA_API BoundsException: public Exception {

public:
    BoundsException(const char* source, StringRef message):
                Exception(source, message) {}

    BoundsException(const char* source, const SBuf& message):
                Exception(source, message) {}


};


}
}
#endif
