
// Copyright 2016 Victor Smirnov
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
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include "../btfl_test_names.hpp"

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl_test::IterApiName)


    using typename Base::Container;


    using CtrSizeT  = typename Container::Types::CtrSizeT;
    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;


    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    static const Int Streams          = Container::Types::Streams;
    static const Int DataStreams      = Container::Types::DataStreams;

    template <Int Level>
    using BTFLSampleData = typename Container::Types::template IOData<Level>;

    template <typename BTFLDataT>
    using BTFLDataIOBufferProducerPool = ObjectPool<
            btfl::BTFLDataIOBufferProducer<
                    BTFLDataT,
										DataStreams,
                    DataStreams - btfl::BTFLDataStreamsCounter<BTFLDataT>::Value
            >
        >;

    template <typename BTFLDataT>
    using BTFLDataReaderPool = ObjectPool<
            btfl::BTFLDataReader<
                            BTFLDataT,
                            DataStreams,
                            DataStreams - btfl::BTFLDataStreamsCounter<BTFLDataT>::Value
            >
        >;

public:

    template <typename IOBuffer>
    auto bulkio_insert(BufferProducer<IOBuffer>& provider, const Int initial_capacity = 20000)
    {
        auto& self = this->self();

        return self.ctr().bulkio_insert(self, provider, initial_capacity);
    }

    template <typename BTFLDataT>
    auto insert_iodata(const BTFLDataT& data)
    {
        auto& self = this->self();

        auto iodata_producer = self.ctr().pools().get_instance(PoolT<BTFLDataIOBufferProducerPool<BTFLDataT>>()).get_unique(65536);

        iodata_producer->init(data);

        return self.bulkio_insert(*iodata_producer.get());
    }

    template <typename BTFLDataT, typename InputIter>
    auto insert_iodata(const InputIter& start, const InputIter& end)
    {
        auto& self = this->self();

        auto iodata_producer = self.ctr().pools().get_instance(PoolT<BTFLDataIOBufferProducerPool<BTFLDataT>>()).get_unique(65536);

        iodata_producer->init(start, end);

        return self.bulkio_insert(*iodata_producer.get());
    }

    template <Int Level>
    BTFLSampleData<Level> readEntries(CtrSizeT number = std::numeric_limits<CtrSizeT>::max())
    {
        auto& self = this->self();

        Int stream = self.data_stream_s();

        if (stream == Level || stream == -1)
        {
            using BTFLDataT = BTFLSampleData<Level>;

            BTFLDataT data;

            auto reader = self.ctr().pools().get_instance(PoolT<BTFLDataReaderPool<BTFLDataT>>()).get_unique();

            reader->init(data);

            self.bulkio_scan_ge(reader.get(), -1, number);

            return data;
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
        }
    }


    template <Int Level>
    BTFLSampleData<Level> readData(CtrSizeT length)
    {
        auto& self = this->self();

        Int stream = self.data_stream_s();

        if (stream == Level || stream == -1)
        {
            using BTFLDataT = BTFLSampleData<Level>;

            BTFLDataT data;

            auto reader = self.ctr().pools().get_instance(PoolT<BTFLDataReaderPool<BTFLDataT>>()).get_unique(65536);

            reader->init(data);

            self.bulkio_read(reader.get(), length);

            return data;
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
        }
    }


    template <Int Stream>
    auto key() const
    {
        auto& self = this->self();

        Int stream = self.data_stream();

        MEMORIA_V1_ASSERT(stream, ==, Stream);

        Int key_idx = self.data_stream_idx(stream);

        return std::get<0>(self.template read_leaf_entry<Stream, IntList<1>>(key_idx, 0));
    }

    CtrSizeT countChildren() const
    {
        auto& self = this->self();
        Int stream = self.data_stream();

        if (stream < DataStreams - 1)
        {
            auto ii = self.clone();
            ii->selectGEFw(1, stream);

            auto r0 = self.rank(stream + 1);
            auto r1 = ii->rank(stream + 1);

            return r1 - r0;
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
        }
    }

    void toChild(CtrSizeT n)
    {
    	auto& self = this->self();
    	Int stream = self.data_stream();

    	if (stream < DataStreams - 1)
    	{
    		self.selectFw(1, stream + 1);
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
    	}
    }

    void toParent(Int level)
    {
    	auto& self = this->self();
    	self.selectBw(1, level);
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl_test::IterApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}}
