
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

#include <memoria/v1/core/types.hpp>

namespace memoria {
namespace v1 {

template <typename T, int32_t n = sizeof(T)> struct PtrToHash;

template <typename T>
struct PtrToHash<T, sizeof(int32_t)> {
    static int32_t hash(T value) {
        return T2T<int32_t>(value);
    }
};

template <typename T>
struct PtrToHash<T, sizeof(int64_t)> {
    static int32_t hash(T value) {
        int64_t v = T2T<int64_t>(value);
        int32_t v0 = v;
        int32_t v1 = v >> (sizeof(int32_t) * 8);

        return v0^v1;
    }
};





}}
