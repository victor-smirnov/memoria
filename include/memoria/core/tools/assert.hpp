
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/types.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/string.hpp>
#include <iostream>

namespace memoria {

template <bool Value> class STATIC_ASSERT_FAILURE;
template <> class STATIC_ASSERT_FAILURE <true> {};

}


#define MEMORIA_V1_STATIC_ASSERT(B) \
    enum { MMA_JOIN(MEMORIA_V1_STATIC_ASSERT_, __LINE__) = sizeof(STATIC_ASSERT_FAILURE<(bool)(B)>)}


#ifndef MEMORIA_V1_NO_ASSERTS

#define MEMORIA_V1_ASSERT(Left, Operation, Right)                                                       \
        if (!(Left Operation Right)) {                                                                  \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(format_u8("ASSERT FAILURE: {} {} {} Values: {} {}", \
                    #Left, #Operation, #Right, Left, Right));                                              \
        }


#define MEMORIA_V1_WARNING(Left, Operation, Right)                                                      \
        if ((Left Operation Right)) {                                                                   \
            std::cout<<"WARNING: "                                                                      \
                    <<#Left<<" "<<#Operation<<" "<<#Right<<" Values: "<<Left<<" "<<Right                \
                    <<" at "<<MA_SRC                                                                    \
                    <<std::endl;                                                                        \
        }


#define MEMORIA_V1_ASSERT_TRUE(Arg0)                                                                    \
        if (!(Arg0)) {                                                                                  \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(::memoria::format_u8("ASSERT TRUE FAILURE: {}", #Arg0));        \
        }

#define MEMORIA_V1_ASSERT_FALSE(Arg0)                                                                   \
        if ((Arg0)) {                                                                                   \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(::memoria::format_u8("ASSERT FALSE FAILURE: {}", #Arg0));       \
        }


#define MEMORIA_V1_ASSERT_EXPR(Expr, Msg)                                                               \
        if (!(Expr)) {                                                                                  \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(format_u8("ASSERT FAILURE: {} {}", #Expr, #Msg));   \
        }


#define MEMORIA_V1_ASSERT_NOT_NULL(Operand)                                                             \
        if (!Operand) {                                                                                 \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(::memoria::format_u8("ASSERT FAILURE: {} must not be NULL", #Operand));\
        }

#define MEMORIA_V1_ASSERT_NOT_EMPTY(Operand)                                                            \
        if (Operand.is_null()) {                                                                        \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(format_u8("ASSERT FAILURE: {} must not be 0", #Operand));\
        }


#define MEMORIA_V1_INVALID_STREAM(Idx) \
    MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(format_u8("Invalid Stream: {}", Idx))

#define MEMORIA_V1_ASSERT_ALIGN(MemExpr, Align)                                                         \
        if (::memoria::value_cast<std::ptrdiff_t>(MemExpr) % Align != 0) {                                                \
            MMA_THROW(::memoria::Exception()) << ::memoria::WhatInfo(format_u8("ASSERT FAILURE: \"{}\" is not properly aligned ({})", #MemExpr, Align)); \
        }


#else

#define MEMORIA_V1_ASSERT(Left, Operation, Right)
#define MEMORIA_V1_ASSERT_EXPR(Expr, Msg)
#define MEMORIA_V1_ASSERT_NOT_NULL(Operand)
#define MEMORIA_V1_ASSERT_TRUE(Arg0)
#define MEMORIA_V1_INVALID_STREAM(Idx)
#define MEMORIA_V1_ASSERT_NOT_EMPTY(Operand)
#define MEMORIA_V1_ASSERT_ALIGN(MemExpr, Align)

#endif

