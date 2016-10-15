
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

#include <iostream>
#include <typeinfo>



namespace memoria {
namespace v1 {




template <typename T>
struct ValueTraits {
    static const Long Size = sizeof(T); //T::SIZE;
    typedef T Type;
    static const bool IsPrimitive = false;
};


#define MEMORIA_VALUE_TRAITS(ValueType, MinValue, MaxValue)                     \
template <>                                                                     \
struct ValueTraits<ValueType> {                                                 \
    static const Long Size              = sizeof(ValueType);                    \
    typedef ValueType                   Type;                                   \
                                                                                \
    static const ValueType Min = MinValue;                                      \
    static const ValueType Max = MaxValue;                                      \
    static const bool IsPrimitive = true;                                       \
};                                                                              \
                                                                                \
template <>                                                                     \
struct ValueTraits<const ValueType> {                                           \
    static const Long Size              = sizeof(ValueType);                    \
    typedef const ValueType                   Type;                             \
                                                                                \
    static const ValueType Min = MinValue;                                      \
    static const ValueType Max = MaxValue;                                      \
    static const bool IsPrimitive = true;                                       \
}


//const Long LongMin = 1l << (sizeof(Long) * 8 - 1);
//const Long LongMax = ~LongMin;

//const ULong ULongMin = 0l;
//const ULong ULongMax = ~ULongMin;

MEMORIA_VALUE_TRAITS(Byte, (Byte)0x80, (Byte)0x7fb);
MEMORIA_VALUE_TRAITS(Short, (Short)0x8000, (Short)0x7fff);
MEMORIA_VALUE_TRAITS(Int,  0x80000000, 0x7fffffff);
//MEMORIA_VALUE_TRAITS(Long, LongMin, LongMax);
MEMORIA_VALUE_TRAITS(BigInt, 0x8000000000000000ll, 0x7fffffffffffffffll);

MEMORIA_VALUE_TRAITS(UByte,  0, 0xFF);
MEMORIA_VALUE_TRAITS(UShort, 0, 0xFFFF);
MEMORIA_VALUE_TRAITS(UInt,   0, 0xFFFFFFFF);
//MEMORIA_VALUE_TRAITS(ULong,  ULongMin, ULongMax);
MEMORIA_VALUE_TRAITS(UBigInt,0ull, 0xFFFFFFFFFFFFFFFFull);

template <>
struct ValueTraits<EmptyValue> {
    static const Long Size = 0;
    typedef EmptyValue Type;
    static const bool IsPrimitive = false;
};


#undef MEMORIA_VALUE_TRAITS

}}
