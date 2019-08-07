
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>



#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {

template <
        typename T,
        bool FixedSize = DataTypeTraits<typename T::DataType>::isFixedSize
>
struct MapSubstreamAdapter;

template <typename DataType, template <typename CxxType> class ValueCodec>
struct MapSubstreamAdapter<ICtrApiSubstream<DataType, io::ColumnWise, ValueCodec>, false>
{
    using ValueView = typename DataTypeTraits<DataType>::ViewType;
    using Codec = ValueCodec<ValueView>;

    using SubstreamT = typename io::IOSubstreamInterfaceTF<
        ValueView,
        true, // ColumnWise
        false // FixedSize
    >::Type;

    template <typename Buffer>
    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int size, Buffer& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        io::ConstVLenArrayColumnMetadata descr = subs.select_and_describe(column, start);

        if (start + size <= descr.size)
        {
            Codec codec;
            size_t pos = 0;
            for (int32_t c = 0; c < size; c++)
            {
                ValueView vv;
                size_t len = codec.decode(descr.data_buffer, vv, pos);

                buffer.emplace_back(std::move(vv));

                pos += len;
            }
        }
        else {
            MMA1_THROW(RuntimeException())
                    << fmt::format_ex(
                           u"Substream range check: start = {}, size = {}, size = {}",
                           start, size, descr.size
                       );
        }
    }
};


template <typename DataType, template <typename CxxType> class ValueCodec>
struct MapSubstreamAdapter<ICtrApiSubstream<DataType, io::RowWise, ValueCodec>, false>
{
    using ValueView = typename DataTypeTraits<DataType>::ViewType;
    using Codec = ValueCodec<ValueView>;

    using SubstreamT = typename io::IOSubstreamInterfaceTF<
        ValueView,
        false, // RowWise
        false // FixedSize
    >::Type;

    template <typename Buffer>
    static void read_to(const io::IOSubstream& substream, int32_t column, int32_t start, int size, Buffer& buffer)
    {
        const auto& subs = io::substream_cast<SubstreamT>(substream);
        if (subs.columns() == 1)
        {
            if (start + size <= subs.size())
            {
                auto data_buffer = subs.select(start);

                Codec codec;
                size_t pos = 0;
                for (int32_t c = 0; c < size; c++)
                {
                    ValueView vv;
                    size_t len = codec.decode(data_buffer, vv, pos);

                    buffer.emplace_back(std::move(vv));

                    pos += len;
                }
            }
            else {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(
                               u"Substream range check: start = {}, size = {}, size = {}",
                               start, size, subs.size()
                           );
            }
        }
        else {
            MMA1_THROW(RuntimeException())
                << WhatCInfo("Multicolumn RowWise substreams are not yet supported");
        }
    }
};


template <
    typename Types,
    typename Profile,
    bool DirectKeys   = DataTypeTraits<typename Types::Key>::isFixedSize,
    bool DirectValues = DataTypeTraits<typename Types::Value>::isFixedSize
>
class MapScanner;

template <typename Types, typename Profile>
class MapScanner<Types, Profile, false, false> {
public:
    using KeyView   = typename DataTypeTraits<typename Types::Key>::ViewType;
    using ValueView = typename DataTypeTraits<typename Types::Value>::ViewType;

private:

    using IOVSchema = Linearize<typename Types::IOVSchema>;

    bool finished_{false};

    io::DefaultIOBuffer<KeyView> keys_;
    io::DefaultIOBuffer<ValueView> values_;

    CtrSharedPtr<BTSSIterator<Profile>> btss_iterator_;


public:
    MapScanner(CtrSharedPtr<BTSSIterator<Profile>> iterator):
        btss_iterator_(iterator)
    {
        populate();
    }

    Span<const KeyView>   keys() const {return keys_.span();}
    Span<const ValueView> values() const {return values_.span();}

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
        keys_.clear();
        values_.clear();

        const io::IOVector& iovector = btss_iterator_->iovector_view();

        int32_t size = iovector.symbol_sequence().size();
        if (size > 0)
        {
            keys_.ensure(size);
            values_.ensure(size);

            int32_t start = btss_iterator_->iovector_pos();

            MapSubstreamAdapter<Select<0, IOVSchema>>::read_to(
                iovector.substream(0),
                0, // Column
                start,
                size - start,
                keys_
            );

            MapSubstreamAdapter<Select<1, IOVSchema>>::read_to(
                iovector.substream(0),
                0, // Column
                start,
                size - start,
                values_
            );
        }
    }
};




template <typename Types, typename Profile>
class MapScanner<Types, Profile, true, true> {
public:
    using CxxKey   = typename Types::CxxKey;
    using CxxValue = typename Types::CxxValue;

private:
    Span<const CxxKey> keys_;
    Span<const CxxValue> values_;

    CtrSharedPtr<BTSSIterator<Profile>> btss_iterator_;

public:

    const Span<const CxxKey>&   keys() const {return keys_;}
    const Span<const CxxValue>& values() const {return values_;}

    bool is_end() const {
        return btss_iterator_->is_end();
    }

    bool next_leaf() {
        return btss_iterator_->next_leaf();
    }
};

}}
