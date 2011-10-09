
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_VAPI_EXCEPTIONS_BOUNDS_HPP
#define	_MEMORIA_VAPI_EXCEPTIONS_BOUNDS_HPP

#include <memoria/core/exceptions/memoria.hpp>
#include <string>

namespace memoria    {
namespace vapi       {

using namespace std;

class MEMORIA_API BoundsException: public MemoriaException {
    BigInt index_;
    BigInt index1_;
    BigInt index2_;

public:
    BoundsException(const string &source, const string &message, BigInt index, BigInt index1, BigInt index2):
                MemoriaException(source, message), index_(index), index1_(index1), index2_(index2) {}

    BoundsException(const string &source, BigInt index, BigInt index1, BigInt index2):
                MemoriaException(source, "Index is out of bounds"), index_(index), index1_(index1), index2_(index2) {}

    BigInt index() const {
        return index_;
    }

    BigInt index1() const {
        return index1_;
    }

    BigInt index2() const {
        return index2_;
    }
};


}
}
#endif
