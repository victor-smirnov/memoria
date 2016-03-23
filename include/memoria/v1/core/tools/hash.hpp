
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

#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace v1 {

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





}}
