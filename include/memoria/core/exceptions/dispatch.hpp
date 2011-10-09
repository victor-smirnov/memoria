
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_EXCEPTIONS_DISPATCH_HPP
#define	_MEMORIA_VAPI_EXCEPTIONS_DISPATCH_HPP

#include <memoria/core/exceptions/memoria.hpp>

namespace memoria    {
namespace vapi       {

using namespace std;

class MEMORIA_API DispatchException: public MemoriaException {
    Int code1_;
    Int code2_;
public:

    DispatchException(const string &source, const string &message, Int code1 = 0, Int code2 = 0):
                MemoriaException(source, message), code1_(code1), code2_(code2) {}

    DispatchException(const string &source, Int code1, Int code2 = -1):
                MemoriaException(source, "Invalid dispatch"), code1_(code1), code2_(code2) {}

    Int code1() const {
        return code1_;
    }

    Int code2() const {
        return code2_;
    }
};


}
}

#endif
