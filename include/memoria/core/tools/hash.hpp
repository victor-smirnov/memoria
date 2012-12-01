
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_HASH_HPP_
#define MEMORIA_CORE_TOOLS_HASH_HPP_

#include <memoria/core/types/types.hpp>

namespace memoria{

template <typename T, Int n = sizeof(T)> struct PtrToHash;

template <typename T>
struct PtrToHash<T, sizeof(Int)> {
    static Int hash(T value) {
        return T2T<Int>(value);
    }
};

template <typename T>
struct PtrToHash<T, sizeof(BigInt)> {
    static Int hash(T value) {
        BigInt v = T2T<BigInt>(value);
        Int v0 = v;
        Int v1 = v >> (sizeof(Int) * 8);

        return v0^v1;
    }
};





}

#endif
