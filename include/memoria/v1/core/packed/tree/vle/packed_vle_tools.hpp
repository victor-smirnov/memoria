
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
    static constexpr Int BitsPerElement = Codec::ElementSize;
    static constexpr Int BlockSize = 128;// bytes

    static constexpr Int BranchingFactor = PackedTreeBranchingFactor;
    static constexpr Int ValuesPerBranch = BlockSize * 8 / BitsPerElement;
};

template <
    typename IndexValueT,
    Int kBlocks,
    template <typename> class CodecT,
    typename ValueT = BigInt,
    Int kBranchingFactor = PkdVLETreeShapeProvider<CodecT<ValueT>>::BranchingFactor,
    Int kValuesPerBranch = PkdVLETreeShapeProvider<CodecT<ValueT>>::ValuesPerBranch
>
struct PkdVLETreeTypes {
    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    template <typename T>
    using Codec = CodecT<T>;

    static constexpr Int Blocks = kBlocks;
    static constexpr Int BranchingFactor = kBranchingFactor;
    static constexpr Int ValuesPerBranch = kValuesPerBranch;
};



}}