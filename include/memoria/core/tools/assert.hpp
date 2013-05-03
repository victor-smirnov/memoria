
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




#ifndef _MEMORIA_CORE_TOOLS_ASSERT_HPP
#define _MEMORIA_CORE_TOOLS_ASSERT_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/string_buffer.hpp>

#include <iostream>

namespace memoria    {

template <bool Value> class STATIC_ASSERT_FAILURE;
template <> class STATIC_ASSERT_FAILURE <true> {};

#define MEMORIA_STATIC_ASSERT(B) \
    enum { MEMORIA_JOIN(MEMORIA_static_assert_, __LINE__) = sizeof(::memoria::STATIC_ASSERT_FAILURE<(bool)(B)>)}


#ifndef MEMORIA_NO_ASSERTS

#define MEMORIA_ASSERT(Left, Operation, Right)                                                                            \
        if (!(Left Operation Right)) {                                                                                    \
            throw memoria::vapi::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "\
                    <<#Left<<" "<<#Operation<<" "<<#Right<<" Values: "<<Left<<" "<<Right); \
        }


#define MEMORIA_ASSERT_TRUE(Arg0)                                                                       \
        if (!(Arg0)) {                                                                                  \
            throw memoria::vapi::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT TRUE FAILURE: "				\
                    <<#Arg0); 																			\
        }


#define MEMORIA_ASSERT_EXPR(Expr, Msg)                                                              \
        if (!(Expr)) {                                                                              \
            throw memoria::vapi::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "<<#Expr<<" "<<#Msg);   \
        }


#define MEMORIA_ASSERT_NOT_NULL(Operand)                                                                        \
        if (Operand == NULL) {                                                                                  \
            throw memoria::vapi::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "<<#Operand<<" must not be NULL");  \
        }

#define MEMORIA_ASSERT_NOT_EMPTY(Operand)                                                                       \
        if (Operand.isEmpty()) {                                                                                    \
            throw memoria::vapi::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "<<#Operand<<" must not be 0"); \
        }


#define MEMORIA_INVALID_STREAM(Idx) \
	throw memoria::vapi::Exception(MEMORIA_SOURCE, SBuf()<<"Invalid Stream: "<<Idx)

#else

#define MEMORIA_ASSERT(Left, Operation, Right)
#define MEMORIA_ASSERT_EXPR(Expr, Msg)
#define MEMORIA_ASSERT_NOT_NULL(Operand)
#define MEMORIA_ASSERT_TRUE(Arg0)

#endif

}

#endif
