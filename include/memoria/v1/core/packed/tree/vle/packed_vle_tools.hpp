
// Copyright 2015 Victor Smirnov
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

template <typename Codec>
struct PkdVLETreeShapeProvider {
    static constexpr int32_t BitsPerElement = Codec::ElementSize;
    static constexpr int32_t BlockSize = 128;// bytes

    static constexpr int32_t BranchingFactor = PackedTreeBranchingFactor;
    static constexpr int32_t ValuesPerBranch = BlockSize * 8 / BitsPerElement;
};

template <
    typename IndexValueT,
    int32_t kBlocks,
    template <typename> class CodecT,
    typename ValueT = int64_t,
    int32_t kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
    int32_t kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
struct PkdVLETreeTypes {
    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    template <typename T>
    using Codec = CodecT<T>;

    static constexpr int32_t Blocks = kBlocks;
    static constexpr int32_t BranchingFactor = kBranchingFactor;
    static constexpr int32_t ValuesPerBranch = kValuesPerBranch;
};



}}