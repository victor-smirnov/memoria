
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_EXCEPTIONS_CAST_HPP
#define	_MEMORIA_VAPI_EXCEPTIONS_CAST_HPP

#include <memoria/core/exceptions/memoria.hpp>
#include <memoria/core/exceptions/npe.hpp>

#include <string>



namespace memoria    {
namespace vapi       {

using namespace std;

/*class MEMORIA_API TypeCastException: public MemoriaException {
    const Type* src_type_;
    const Type* tgt_type_;

public:
    TypeCastException(const string &source, const string &message, const Type* src_type, const Type* tgt_type):
                MemoriaException(source, message), src_type_(src_type), tgt_type_(tgt_type) {}

    TypeCastException(const string &source, const Type* src_type, const Type* tgt_type):
                MemoriaException(source, "Invalid type cast"), src_type_(src_type), tgt_type_(tgt_type) {}

    const Type* src_type() const {
        return src_type_;
    }

    const Type* tgt_type() const {
        return tgt_type_;
    }
};

template <typename T>
T* TypedCast(Typed *typed, bool check_null = true) {
    if (check_null && typed == NULL) {
        throw NullPointerException(MEMORIA_SOURCE, "Argument must not be null");
    }

    if (typed != NULL) {
        if (!typed->IsInstance(T::MyType())) {
            throw TypeCastException(MEMORIA_SOURCE, "Can't cast "+typed->type()->name()+" to "+T::MyType()->name(), typed->type(), T::MyType());
        }
        else {
            return static_cast<T*>(typed);
        }
    }
    else {
        return NULL;
    }
}
*/


}
}



#endif
