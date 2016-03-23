
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>
#include <iostream>

namespace memoria {
namespace v1 {

template <bool Value> class STATIC_ASSERT_FAILURE;
template <> class STATIC_ASSERT_FAILURE <true> {};

}}


#define MEMORIA_STATIC_ASSERT(B) \
    enum { MEMORIA_JOIN(MEMORIA_static_assert_, __LINE__) = sizeof(v1::STATIC_ASSERT_FAILURE<(bool)(B)>)}


#ifndef MEMORIA_NO_ASSERTS

#define MEMORIA_ASSERT(Left, Operation, Right)                                                          \
        if (!(Left Operation Right)) {                                                                  \
            throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "                 \
                    <<#Left<<" "<<#Operation<<" "<<#Right<<" Values: "<<Left<<" "<<Right);              \
        }


#define MEMORIA_WARNING(Left, Operation, Right)                                                         \
        if ((Left Operation Right)) {                                                                   \
            std::cout<<"WARNING: "                                                                      \
                    <<#Left<<" "<<#Operation<<" "<<#Right<<" Values: "<<Left<<" "<<Right                \
                    <<" at "<<MA_SRC                                                                    \
                    <<std::endl;                                                                        \
        }


#define MEMORIA_ASSERT_TRUE(Arg0)                                                                       \
        if (!(Arg0)) {                                                                                  \
            throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT TRUE FAILURE: "            \
                    <<#Arg0);                                                                           \
        }

#define MEMORIA_ASSERT_FALSE(Arg0)                                                                      \
        if ((Arg0)) {                                                                                   \
            throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FALSE FAILURE: "           \
                    <<#Arg0);                                                                           \
        }


#define MEMORIA_ASSERT_EXPR(Expr, Msg)                                                              \
        if (!(Expr)) {                                                                              \
            throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "<<#Expr<<" "<<#Msg);   \
        }


#define MEMORIA_ASSERT_NOT_NULL(Operand)                                                                        \
        if (Operand == NULL) {                                                                                  \
            throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "<<#Operand<<" must not be NULL");  \
        }

#define MEMORIA_ASSERT_NOT_EMPTY(Operand)                                                                       \
        if (Operand.is_null()) {                                                                                    \
            throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"ASSERT FAILURE: "<<#Operand<<" must not be 0"); \
        }


#define MEMORIA_INVALID_STREAM(Idx) \
    throw v1::Exception(MEMORIA_SOURCE, SBuf()<<"Invalid Stream: "<<Idx)

#define MEMORIA_ASSERT_ALIGN(MemExpr, Align)                                                    \
        if (T2T<std::ptrdiff_t>(MemExpr) % Align != 0) {                                        \
            throw v1::Exception(MEMORIA_SOURCE,                                          \
                SBuf()<<"ASSERT FAILURE: \""<<#MemExpr<<"\" is not properly aligned ("<<Align<<")");    \
        }


#else

#define MEMORIA_ASSERT(Left, Operation, Right)
#define MEMORIA_ASSERT_EXPR(Expr, Msg)
#define MEMORIA_ASSERT_NOT_NULL(Operand)
#define MEMORIA_ASSERT_TRUE(Arg0)
#define MEMORIA_INVALID_STREAM(Idx)
#define MEMORIA_ASSERT_NOT_EMPTY(Operand)
#define MEMORIA_ASSERT_ALIGN(MemExpr, Align)

#endif

