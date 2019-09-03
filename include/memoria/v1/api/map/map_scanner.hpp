
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/common/iobuffer_adatpters.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

namespace memoria {
namespace v1 {


namespace _ {

template <typename KeyT, typename ValueT, bool FixedSizeKey, bool FixedSizeValue>
struct MapKeysValues;

template <typename KeyT, typename ValueT>
struct MapKeysValues<KeyT, ValueT, false, false>
{
    ArenaBuffer<KeyT> keys_;
    ArenaBuffer<ValueT> values_;

    Span<const KeyT> keys() const {return keys_.span();}
    Span<const ValueT> values() const {return values_.span();}

    void prepare(size_t size) {
        keys_.clear();
        values_.clear();
        keys_.ensure(size);
        values_.ensure(size);
    }
};


template <typename KeyT, typename ValueT>
struct MapKeysValues<KeyT, ValueT, true, false>
{
    Span<const KeyT> keys_;
    ArenaBuffer<ValueT> values_;

    const Span<const KeyT>& keys() const {return keys_;}
    Span<const ValueT> values() const {return values_.span();}

    void prepare(size_t size) {
        values_.clear();
        values_.ensure(size);
    }
};


template <typename KeyT, typename ValueT>
struct MapKeysValues<KeyT, ValueT, false, true>
{
    ArenaBuffer<KeyT> keys_;
    Span<const ValueT> values_;

    Span<const KeyT> keys() const {return keys_.span();}
    const Span<const ValueT>& values() const {return values_;}

    void prepare(size_t size) {
        keys_.clear();
        keys_.ensure(size);
    }
};

template <typename KeyT, typename ValueT>
struct MapKeysValues<KeyT, ValueT, true, true>
{
    Span<const KeyT> keys_;
    Span<const ValueT> values_;

    const Span<const KeyT>& keys() const {return keys_;}
    const Span<const ValueT>& values() const {return values_;}

    void prepare(size_t size) {}
};



}


template <typename Types, typename Profile>
class MapScanner {
public:
    using KeyView   = typename DataTypeTraits<typename Types::Key>::ViewType;
    using ValueView = typename DataTypeTraits<typename Types::Value>::ViewType;

private:

    static constexpr bool DirectKeys   = DataTypeTraits<typename Types::Key>::isFixedSize;
    static constexpr bool DirectValues = DataTypeTraits<typename Types::Value>::isFixedSize;

    using IOVSchema = Linearize<typename Types::IOVSchema>;

    bool finished_{false};

    _::MapKeysValues<KeyView, ValueView, DirectKeys, DirectValues> entries_;

    CtrSharedPtr<BTSSIterator<Profile>> btss_iterator_;

public:
    MapScanner(CtrSharedPtr<BTSSIterator<Profile>> iterator):
        btss_iterator_(iterator)
    {
        populate();
    }

    Span<const KeyView>   keys() const {return entries_.keys();}
    Span<const ValueView> values() const {return entries_.values();}

    bool is_end() const {
        return finished_ || btss_iterator_->is_end();
    }

    bool next_leaf()
    {
        bool available = btss_iterator_->next_leaf();
        if (available) {
            populate();
        }
        else {
            finished_ = true;
        }

        return available;
    }

private:
    void populate()
    {
        const io::IOVector& iovector = btss_iterator_->iovector_view();

        int32_t size = iovector.symbol_sequence().size();
        if (size > 0)
        {
            entries_.prepare(size);

            int32_t start = btss_iterator_->iovector_pos();

            IOSubstreamAdapter<Select<0, IOVSchema>>::read_to(
                iovector.substream(0),
                0, // Column
                start,
                size - start,
                entries_.keys_
            );

            IOSubstreamAdapter<Select<1, IOVSchema>>::read_to(
                iovector.substream(1),
                0, // Column
                start,
                size - start,
                entries_.values_
            );
        }
    }
};


}}
