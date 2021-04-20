
// Copyright 2017 Victor Smirnov
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

#include <memoria/api/common/ctr_api_btfl.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memory>
#include <tuple>

namespace memoria {

struct EdgeMap {};

struct EdgeMapFindResult {
    UAcc128T prefix;
    int64_t pos;
    int64_t size;

    bool is_found() const {return pos >= 0 && pos < size;}
};

template <typename Profile>
class CtrApi<EdgeMap, Profile>: public CtrApiBTFLBase<EdgeMap, Profile> {
public:
    using Key = UUID;
    using Value = UAcc128T;

    using DataValue = std::tuple<Key, Value>;
private:
    using MapT = InvertedIndex;
    
    using Base = CtrApiBTFLBase<EdgeMap, Profile>;
    using typename Base::StoreT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    using typename Base::CtrID;
    using typename Base::CtrSizeT;
    
    static constexpr int32_t DataStreams = 2;
    using CtrSizesT = ProfileCtrSizesT<Profile, DataStreams + 1>;
    
    using EdgeMapValueIterator = typename Iterator::EdgeMapValueIterator;

    MMA_DECLARE_CTRAPI_BASIC_METHODS()
    
    Iterator find(const Key& key);
    Iterator find_or_create(const Key& key);
    
    bool contains(const Key& key);
    bool contains(const Key& key, const Value& value);

    bool upsert(const Key& key, const Value& value);

    bool remove(const Key& key);
    bool remove(const Key& key, const Value& value);

    EdgeMapValueIterator iterate(const Key& key);

    Iterator begin() {return Base::ctr_seq_begin();}
    Iterator end() {return Base::ctr_seq_begin();}
    Iterator seek(int64_t pos);

    int64_t size(const Key& key);
    int64_t size() const;

#ifdef MMA_USE_IOBUFFER
    Iterator assign(const Key& key, bt::BufferProducer<CtrIOBuffer>& values_producer);

    template <typename InputIterator, typename EndIterator>
    Iterator assign(const Key& key, const InputIterator& start, const EndIterator& end)
    {
        InputIteratorProvider<Value, InputIterator, EndIterator, CtrIOBuffer> provider(start, end);        
        return assign(key, provider);
    }

    CtrSizeT read(const Key& key, bt::BufferConsumer<CtrIOBuffer>& values_consumer, CtrSizeT start = 0, CtrSizeT length = std::numeric_limits<CtrSizeT>::max());
#endif

    class EdgeMapKeyIterator {
        Iterator iterator_;
        CtrSizeT cnt_{};
        CtrSizeT size_{};
    public:
        EdgeMapKeyIterator(Iterator iterator, CtrSizeT size):
            iterator_(iterator), size_(size)
        {}

        EdgeMapKeyIterator():iterator_(nullptr) {}

        bool has_keys() const
        {
            return cnt_ < size_;
        }

        void next_key()
        {
            if (cnt_ < size_)
            {                
                cnt_++;
                iterator_.next_key();
            }
            else {
                MMA_THROW(RuntimeException()) << WhatCInfo("No such element in iterator");
            }
        }

        CtrSizeT cnt() const {return cnt_;}
        CtrSizeT size() const {return size_;}

        Key key() {
            return iterator_.key();
        }

        EdgeMapValueIterator values();

        Iterator& iterator() {return iterator_;}
    };

    EdgeMapKeyIterator keys() {
        return EdgeMapKeyIterator{this->begin(), this->size()};
    }
};


template <typename Profile>
class IterApi<EdgeMap, Profile>: public IterApiBTFLBase<EdgeMap, Profile> {
    
    using Base = IterApiBTFLBase<EdgeMap, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    using Key = UUID;
    using Value = UAcc128T;

    using typename Base::CtrSizeT;
    
    static constexpr int32_t DataStreams = CtrApi<EdgeMap, Profile>::DataStreams;
    using CtrSizesT = typename CtrApi<EdgeMap, Profile>::CtrSizesT;
    
    using DataValue = std::tuple<Key, Value>;
    
    MMA_DECLARE_ITERAPI_BASIC_METHODS()
    
    Key key() const;
    Value value() const;
    
    bool is_end();
    bool next_key();
    void to_prev_key();

    int64_t iter_skip_fw(int64_t offset) const;
    
    int64_t count_values() const;
    int64_t run_pos() const;
    void insert_key(const Key& key);
    void insert_value(const Value& value);

    int64_t remove(int64_t length = 1);
    bool is_found(const Key& key);

    bool to_values();

    EdgeMapFindResult find_value(const Value& value);

    class EdgeMapValueIterator
    {
        UAcc128T acc_{};
        UAcc128T value_{};

        IterApi iterator_;
        CtrSizeT cnt_{};
        CtrSizeT size_{};
    public:
        EdgeMapValueIterator(const IterApi& iterator, CtrSizeT size):
            iterator_{iterator},
            size_{size}
        {}

        EdgeMapValueIterator(): iterator_{nullptr}
        {}

        bool has_values() const {
            return cnt_ < size_;
        }

        void next_value()
        {
            if (cnt_ < size_)
            {
                if (iterator_.iter_skip_fw(1) < 1)
                {
                    MMA_THROW(RuntimeException()) << WhatCInfo("Can't move forward to expected value element");
                }

                acc_ += value_;
                cnt_++;
            }
            else {
                MMA_THROW(RuntimeException()) << WhatCInfo("No suche element in iterator");
            }
        }

        CtrSizeT size() const {return size_;}
        CtrSizeT pos() const {return cnt_;}

        UAcc128T value()
        {
            value_ = iterator_.value();
            return acc_ + value_;
        }

        auto& iterator() {return iterator_;}
    };
};

template <>
struct TypeHash<EdgeMap>: UInt64Value <
    HashHelper<4000>
> {};

template <>
struct DataTypeTraits<EdgeMap> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    static constexpr bool HasTypeConstructors = false;

    static void create_signature(SBuf& buf, const EdgeMap& obj) {
        buf << "EdgeMap";
    }

    static void create_signature(SBuf& buf) {
        buf << "EdgeMap";
    }
};

    
}
