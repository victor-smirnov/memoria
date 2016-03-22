
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>

#include <tuple>
#include <vector>

namespace memoria {
namespace mmap    {

using bt::IdxSearchType;
using bt::StreamTag;

template <typename KeyType, Int Selector = HasFieldFactory<KeyType>::Value> struct MMapKeyStructTF;

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 1>: HasType<PkdFMTreeT<KeyType>> {};

template <typename KeyType>
struct MMapKeyStructTF<KeyType, 0>: HasType<PkdVBMTreeT<KeyType>> {};



template <typename ValueType, Int Selector = HasFieldFactory<ValueType>::Value> struct MMapValueStructTF;

template <typename ValueType>
struct MMapValueStructTF<ValueType, 1>: HasType<PkdFSQArrayT<ValueType>> {};

template <typename ValueType>
struct MMapValueStructTF<ValueType, 0>: HasType<PkdVDArrayT<ValueType>> {};


template <typename K, typename V>
using MapData = std::vector<std::pair<K, std::vector<V>>>;

template <typename Ctr, typename Key = typename Ctr::Types::Key, typename Value = typename Ctr::Types::Value, typename Data = std::vector<std::pair<Key, std::vector<Value>>>>
class MMapAdaptor;

template <typename Ctr, typename Key, typename Value>
class MMapAdaptor<Ctr, Key, Value, std::vector<std::pair<Key, std::vector<Value>>>>:
    public bttl::SizedFlatTreeStreamingAdapterBase<
        Ctr,
        MMapAdaptor<Ctr, Key, Value, std::vector<std::pair<Key, std::vector<Value>>>>
    >
{
    using Base = bttl::SizedFlatTreeStreamingAdapterBase<
        Ctr,
        MMapAdaptor<Ctr, Key, Value, std::vector<std::pair<Key, std::vector<Value>>>>
    >;

    using Data = std::vector<std::pair<Key, std::vector<Value>>>;

    const Data& data_;

    using CtrSizesT = core::StaticVector<BigInt, 2>;

    const Value* values_;

public:
    MMapAdaptor(const Data& data): Base(), data_(data), values_() {
        this->init();
    }

    auto prepare(StreamTag<0>) {
        return data_.size();
    }

    auto prepare(StreamTag<1>, const CtrSizesT& path)
    {
        values_ = data_[path[0]].second.data();
        return data_[path[0]].second.size();
    }

    auto buffer(StreamTag<0>, StreamTag<0>, const CtrSizesT& pos, Int block) {
        return 1;
    }

    auto buffer(StreamTag<0>, StreamTag<1>, const CtrSizesT& pos, Int block) {
        return data_[pos[0]].first;
    }

    auto buffer(StreamTag<1>, StreamTag<0>, BigInt pos, Int block) {
        return 1;
    }

    auto buffer(StreamTag<1>, StreamTag<1>, BigInt pos, Int block) {
        return values_[pos];
    }
};







template <typename Ctr, typename Value = typename Ctr::Types::Value, typename Data = std::vector<Value>>
class MMapValueAdaptor;

template <typename Ctr, typename Value>
class MMapValueAdaptor<Ctr, Value, std::vector<Value>>:
    public bttl::SizedFlatTreeStreamingAdapterBase<
        Ctr,
        MMapValueAdaptor<Ctr, Value, std::vector<Value>>
    >
{
    using Base = bttl::SizedFlatTreeStreamingAdapterBase<
        Ctr,
        MMapValueAdaptor<Ctr, Value, std::vector<Value>>
    >;

    using Data      = std::vector<Value>;

    using CtrSizeT  = typename Ctr::Types::CtrSizeT;
    using CtrSizesT = typename Ctr::Types::CtrSizesT;

    using Key       = typename Ctr::Types::Key;


    const Data& data_;
    const Value* values_;

public:
    MMapValueAdaptor(const Data& data): Base(1), data_(data), values_() {
        this->init();
    }

    size_t prepare(StreamTag<0>) {
        return 0;
    }

    size_t prepare(StreamTag<1>, const CtrSizesT& path)
    {
        values_ = data_.data();
        return data_.size();
    }

    auto buffer(StreamTag<0>, StreamTag<0>, const CtrSizesT& pos, Int block) {
        return CtrSizeT();
    }

    auto buffer(StreamTag<0>, StreamTag<1>, const CtrSizesT& pos, Int block) {
        return CtrSizeT();
    }

    auto buffer(StreamTag<1>, StreamTag<0>, BigInt pos, Int block) {
        return CtrSizeT();
    }

    auto buffer(StreamTag<1>, StreamTag<1>, BigInt pos, Int block) {
        return values_[pos];
    }
};



template <typename Ctr, typename Key = typename Ctr::Types::Key, typename Data = std::vector<Key>>
class MMapKeyAdaptor;

template <typename Ctr, typename Key>
class MMapKeyAdaptor<Ctr, Key, std::vector<Key>>:
    public bttl::SizedFlatTreeStreamingAdapterBase<
        Ctr,
        MMapKeyAdaptor<Ctr, Key, std::vector<Key>>
    >
{
    using Base = bttl::SizedFlatTreeStreamingAdapterBase<
        Ctr,
        MMapKeyAdaptor<Ctr, Key, std::vector<Key>>
    >;

    using Data = std::vector<Key>;

    const Data& data_;

    using CtrSizeT = typename Ctr::Types::CtrSizeT;
    using CtrSizesT = typename Ctr::Types::CtrSizesT;

    using Value = typename Ctr::Types::Value;

    const Key* keys_;

public:
    MMapKeyAdaptor(const Data& data): Base(), data_(data), keys_(data.data()) {
        this->init();
    }

    size_t prepare(StreamTag<0>) {
        return data_.size();
    }

    size_t prepare(StreamTag<1>, const CtrSizesT& path)
    {
        return 0;
    }

    auto buffer(StreamTag<0>, StreamTag<0>, const CtrSizesT& pos, Int block) {
        return CtrSizeT();
    }

    auto buffer(StreamTag<0>, StreamTag<1>, const CtrSizesT& pos, Int block) {
        return keys_[pos[0]];
    }

    auto buffer(StreamTag<1>, StreamTag<0>, BigInt pos, Int block) {
        return CtrSizeT();
    }

    auto buffer(StreamTag<1>, StreamTag<1>, BigInt pos, Int block) {
        return Value();
    }
};





}
}
