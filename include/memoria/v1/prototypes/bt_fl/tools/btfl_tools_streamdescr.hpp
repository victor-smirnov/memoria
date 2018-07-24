
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

#include <memoria/v1/core/types/list/append.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/tools/i7_codec.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <memoria/v1/core/container/container.hpp>

namespace memoria {
namespace v1 {
namespace btfl {

namespace {
    template <typename StreamDescriptorsList> struct GetLeafList;

    template <
        typename... Tail,
        typename LeafType,
        typename IndexRangeList,
        template <typename> class BranchStructTF
    >
    struct GetLeafList<TL<bt::StreamTF<LeafType, BranchStructTF, IndexRangeList>, Tail...>> {
        using Type = MergeLists<
                TL<LeafType>,
                typename GetLeafList<TL<Tail...>>::Type
        >;
    };

    template <>
    struct GetLeafList<TL<>> {
        using Type = TL<>;
    };




    template <typename StreamDescriptorsList, typename CtrSizeT = int64_t>
    struct InferSizeStruct {
        using LeafStreamsStructList = typename GetLeafList<StreamDescriptorsList>::Type;
        static const PackedSizeType LeafSizeType = PackedListStructSizeType<Linearize<LeafStreamsStructList>>::Value;

        using Type = IfThenElse<
                LeafSizeType == PackedSizeType::FIXED,
                PkdFQTreeT<CtrSizeT, 1>,
                PkdVQTreeT<CtrSizeT, 1>
        >;
    };


    template <typename List, typename SizeStruct> struct AppendSizeStruct;

    template <typename SizeStruct>
    struct AppendSizeStruct<TypeList<>, SizeStruct> {
        using Type = TL<TL<SizeStruct>>;
    };

    template <typename... List, typename SizeStruct>
    struct AppendSizeStruct<TypeList<List...>, SizeStruct> {
        using Type = TL<TL<List..., SizeStruct>>;
    };



    template <typename List, typename SizeIndexes> struct AppendSizeIndexes;

    template <typename SizeIndexes>
    struct AppendSizeIndexes<TypeList<>, SizeIndexes> {
        using Type = TL<TL<SizeIndexes>>;
    };

    template <typename... List, typename SizeIndexes>
    struct AppendSizeIndexes<TypeList<List...>, SizeIndexes> {
        using Type = TL<TL<List..., SizeIndexes>>;
    };
}




template <
    typename StreamDescriptorsList,
    typename SizeStruct     = typename InferSizeStruct<StreamDescriptorsList>::Type,
    typename SizeIndexes    = TL<bt::SumRange<0, 1>>
> class BTTLAugmentStreamDescriptors;

template <
    typename... Tail,
    typename LeafType,
    typename IndexRangeList,
    template <typename> class BranchStructTF,
    typename SizeStruct,
    typename SizeIndexes
>
class BTTLAugmentStreamDescriptors<TL<bt::StreamTF<LeafType, BranchStructTF, IndexRangeList>, Tail...>, SizeStruct, SizeIndexes> {
    using NewLeafType       = typename AppendSizeStruct<LeafType, SizeStruct>::Type;
    using NewIndexRangeList = typename AppendSizeIndexes<IndexRangeList, SizeIndexes>::Type;
public:
    using Type = MergeLists<
        bt::StreamTF<NewLeafType, BranchStructTF, NewIndexRangeList>,
        typename BTTLAugmentStreamDescriptors<TL<Tail...>, SizeStruct, SizeIndexes>::Type
    >;
};



template <
    typename LeafType,
    typename IndexRangeList,
    template <typename> class BranchStructTF,
    typename SizeStruct,
    typename SizeIndexes
>
class BTTLAugmentStreamDescriptors<TL<bt::StreamTF<LeafType, BranchStructTF, IndexRangeList>>, SizeStruct, SizeIndexes> {
    // Last stream don't need sizes augmentation
public:
    using Type = bt::StreamTF<LeafType, BranchStructTF, IndexRangeList>;
};


}
}}
