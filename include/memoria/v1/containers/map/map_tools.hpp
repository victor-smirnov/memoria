
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input.hpp>

namespace memoria       {
namespace map           {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, Int Selector> struct MapKeyStructTF;

template <typename KeyType>
struct MapKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct MapKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};



template <typename ValueType, Int Selector> struct MapValueStructTF;

template <typename ValueType>
struct MapValueStructTF<ValueType, 1>: HasType<PkdFSQArrayT<ValueType>> {};

template <typename ValueType>
struct MapValueStructTF<ValueType, 0>: HasType<PkdVDArrayT<ValueType>> {};




template <typename T> struct MapBranchStructTF;

template <typename KeyType>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, Int Indexes>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    //FIXME: Extend KeyType to contain enough space to represent practically large sums
    //Should be done systematically on the level of BT

    using Type = IfThenElse <
            HasFieldFactory<KeyType>::Value,
            PkdFQTreeT<KeyType, Indexes>,
            PkdVQTreeT<KeyType, Indexes>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, Int Indexes>
struct MapBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    using Type = IfThenElse<
            HasFieldFactory<KeyType>::Value,
            PkdFMTreeT<KeyType, Indexes>,
            PkdVBMTreeT<KeyType>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};





template <typename Key, typename Value, typename CtrSizeT = BigInt>
class KeyValueEntry {
    const Key& key_;
    const Value& value_;

public:
    KeyValueEntry(const Key& key, const Value& value): key_(key), value_(value) {}

    auto get(StreamTag<0>, StreamTag<0>, Int) const {
        return 0;
    }

    auto get(StreamTag<0>, StreamTag<1>, Int) const {
        return key_;
    }

    auto get(StreamTag<0>, StreamTag<2>, Int) const {
        return value_;
    }
};

template <typename T, Int SubstreamIdx = 0>
class ValueBuffer {
    const T& value_;
public:
    ValueBuffer(const T& value): value_(value) {}

    auto get(StreamTag<0>, StreamTag<SubstreamIdx>, Int) const {
        return value_;
    }
};




template <typename CtrT, typename InputIterator, Int EntryBufferSize = 1000>
class MapEntryIteratorInputProvider: public memoria::btss::AbstractIteratorBTSSInputProvider<
    CtrT,
    MapEntryIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
    InputIterator
>
{
    using Base = memoria::btss::AbstractIteratorBTSSInputProvider<
            CtrT,
            MapEntryIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
            InputIterator
    >;

public:

    using typename Base::CtrSizeT;

public:
    MapEntryIteratorInputProvider(CtrT& ctr, const InputIterator& start, const InputIterator& end, Int capacity = 10000):
        Base(ctr, start, end, capacity)
    {}

    auto buffer(StreamTag<0>, StreamTag<0>, Int idx, Int block) {
        return CtrSizeT();
    }

    const auto& buffer(StreamTag<0>, StreamTag<1>, Int idx, Int block) {
        return std::get<0>(Base::input_value_buffer_[idx]);
    }

    const auto& buffer(StreamTag<0>, StreamTag<2>, Int idx, Int block) {
        return std::get<1>(Base::input_value_buffer_[idx]);
    }
};



}
}
