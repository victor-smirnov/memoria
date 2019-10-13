
// Copyright 2019 Victor Smirnov
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
#include <memoria/v1/core/types/list/append.hpp>

#include <type_traits>

namespace memoria {
namespace v1 {
namespace pdtbuf_ {

template <typename T, typename PkdStruct, psize_t Block>
class PDTDimension;

template <typename List, typename PkdStruct, psize_t StartIdx>
struct DimensionsListBuilder;

template <typename T, typename PkdStruct, typename... Types, psize_t Idx>
struct DimensionsListBuilder<TL<T, Types...>, PkdStruct, Idx> {
    using Dimension = PDTDimension<T, PkdStruct, Idx>;

    using Type = MergeLists<
        Dimension,
        DimensionsListBuilder<TL<Types...>, PkdStruct, Dimension::Width + Idx>
    >;
};

template <typename PkdStruct, psize_t Idx>
struct DimensionsListBuilder<TL<>, PkdStruct, Idx> {
    using Type = MergeLists<>;
};


template <typename PkdStructSO>
class PkdBufViewAccessor {
    PkdStructSO buf_;
public:
    using ViewType = typename PkdStructSO::ViewType;

    PkdBufViewAccessor(PkdStructSO buf):
        buf_(buf)
    {}

    ViewType get(psize_t idx) const {
        return buf_.access(idx);
    }

    bool operator==(const PkdBufViewAccessor& other) const {
        return buf_.data() == other.buf_.data();
    }
};

}}}
