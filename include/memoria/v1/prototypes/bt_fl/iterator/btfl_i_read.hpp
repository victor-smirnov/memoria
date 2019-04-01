
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/core/tools/object_pool.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_output.hpp>

#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/api/common/ctr_api_btfl.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorReadName)

    using Container = typename Base::Container;

    using CtrSizeT      = typename Container::Types::CtrSizeT;
    using DataSizesT    = typename Container::Types::DataSizesT;

    static const int32_t Streams                = Container::Types::Streams;
    static const int32_t DataStreams            = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

    template <typename IOBuffer>
    using ReadWalkerPool = ObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanThroughStrategy>>;

    template <typename IOBuffer>
    using ScanWalkerPool = ObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanRunGTStrategy>>;

    template <typename IOBuffer>
    using ScanRunWalkerPool = ObjectPool<btfl::io::BTFLScanRunWalker<MyType, IOBuffer>>;


public:

    template <typename IOBuffer>
    CtrSizeT bulkio_read(bt::BufferConsumer<IOBuffer>* consumer, const CtrSizeT& limits = std::numeric_limits<CtrSizeT>::max()) {
        return bulkio_read_<ReadWalkerPool>(consumer, -1, limits);
    }

    template <typename IOBuffer>
    CtrSizeT bulkio_scan_ge(bt::BufferConsumer<IOBuffer>* consumer, int32_t expected_stream = -1, const CtrSizeT& limits = std::numeric_limits<CtrSizeT>::max()) {
        return bulkio_read_<ScanWalkerPool>(consumer, expected_stream, limits);
    }

    template <typename IOBuffer>
    CtrSizeT bulkio_scan_run(bt::BufferConsumer<IOBuffer>* consumer, int32_t expected_stream = -1, const CtrSizeT& limits = std::numeric_limits<CtrSizeT>::max()) {
    	return bulkio_read_<ScanRunWalkerPool>(consumer, expected_stream, limits);
    }

    template <typename IOBuffer>
    auto create_read_walker(int32_t expected_stream, CtrSizeT limit = std::numeric_limits<CtrSizeT>::max()) {
        return create_walker_<ReadWalkerPool, IOBuffer>(expected_stream, limit);
    }

    template <typename IOBuffer>
    auto create_scan_ge_walker(int32_t expected_stream, CtrSizeT limit = std::numeric_limits<CtrSizeT>::max()) {
        return create_walker_<ScanWalkerPool, IOBuffer>(expected_stream, limit);
    }

    template <typename IOBuffer>
    auto create_scan_run_walker(int32_t expected_stream, CtrSizeT limit = std::numeric_limits<CtrSizeT>::max()) {
        return create_walker_<ScanRunWalkerPool, IOBuffer>(expected_stream, limit);
    }

    template <typename IOBuffer>
    auto create_scan_run_walker_handler(int32_t expected_stream, CtrSizeT limit = std::numeric_limits<CtrSizeT>::max()) {
        return create_walker_handler<ScanRunWalkerPool, IOBuffer>(expected_stream, limit);
    }


    template <typename Walker, typename IOBuffer>
    int32_t bulkio_populate(Walker& walker, IOBuffer* buffer)
    {
        auto& self = this->self();

        auto start_id = self.leaf()->id();

        int32_t entries = 0;

        bool more_data = false;

        buffer->rewind();

        while (true)
        {
            auto result = walker.populate(*buffer);

            entries += result.entries();

            if (result.ending() == btfl::io::Ending::END_OF_PAGE)
            {
                if (!walker.next_block())
                {
                    more_data = false;
                    break;
                }
            }
            else if (result.ending() == btfl::io::Ending::END_OF_IOBUFFER)
            {
                more_data = true;
                break;
            }
            else if (result.ending() == btfl::io::Ending::LIMIT_REACHED)
            {
                more_data = false;
                break;
            }
            else
            {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid populate IO buffer status: {}", (int32_t) result.ending()));
            }
        }

        self.local_pos()  = walker.local_pos();
        self.leaf().assign(walker.leaf());

        if (self.leaf()->id() != start_id)
        {
            self.refresh();
        }

        return more_data ? entries : -entries;
    }

public:

    template <template <typename> class WalkerPoolT, typename IOBuffer>
    auto create_walker_(int expected_stream, CtrSizeT limit)
    {
        auto& self = this->self();

        auto walker = self.ctr().pools().get_instance(PoolT<WalkerPoolT<IOBuffer>>()).get_unique();

        walker->init(self, expected_stream, limit);

        return walker;
    }

    template <template <typename> class WalkerPoolT, typename IOBuffer>
    auto create_walker_handler(int expected_stream, CtrSizeT limit)
    {
        auto& self = this->self();
        return make_btfl_populate_walker_handler<IOBuffer>(
            self,
            self.template create_walker_<WalkerPoolT, IOBuffer>(expected_stream, limit)
        );
    }


    template <template <typename> class WalkerPoolT, typename IOBuffer>
    CtrSizeT bulkio_read_(bt::BufferConsumer<IOBuffer>* consumer, int32_t expected_stream, const CtrSizeT& limits)
    {
        auto& self = this->self();

        auto start_id = self.leaf()->id();

        auto walker   = self.ctr().pools().get_instance(PoolT<WalkerPoolT<IOBuffer>>()).get_unique();
        auto iobuffer = self.ctr().pools().get_instance(PoolT<ObjectPool<IOBuffer>>()).get_unique(65536);

        walker->init(self, expected_stream, limits);

        IOBuffer& buffer = *iobuffer.get();
        buffer.rewind();

        int32_t entries = 0;

        while (true)
        {
            auto result = walker->populate(buffer);

            entries += result.entries();

            if (result.ending() == btfl::io::Ending::END_OF_PAGE)
            {
                if (!walker->next_block())
                {
                    if (entries > 0)
                    {
                        buffer.flip();
                        consumer->process(buffer, entries);
                        buffer.moveRemainingToStart();
                    }

                    entries = 0;

                    break;
                }
            }
            else if (result.ending() == btfl::io::Ending::END_OF_IOBUFFER)
            {
                if (entries > 0)
                {
                    buffer.flip();
                    consumer->process(buffer, entries);
                    buffer.moveRemainingToStart();
                    entries = 0;
                }
                else {
                    // put backward skip code here...
                }
            }
            else // LIMIT_REACHED
            {
                if (entries > 0)
                {
                    buffer.flip();
                    consumer->process(buffer, entries);
                    buffer.moveRemainingToStart();
                }
                else {
                    // put backward skip code here...
                }

                break;
            }
        }

        self.local_pos()  = walker->local_pos();
        self.leaf().assign(walker->leaf());

        CtrSizeT total = walker->totals();

        walker->clear();

        if (self.leaf()->id() != start_id)
        {
            self.refresh();
        }

        if (self.local_pos() >= self.leaf_size(StructureStreamIdx))
        {
            self.skipFw(0);
        }

        return total;
    }




    void refresh()
    {
        Base::refresh();
    }

    void refresh_prefixes()
    {
        Base::refresh();
    }


    void checkPrefix() {
        auto tmp = self();

        tmp.refresh();

        MEMORIA_V1_ASSERT(self().cache(), ==, tmp.cache());
    }

//    void prepare() {
//        Base::prepare();
//
//        auto& self = this->self();
//        auto& cache = self.cache();
//    }


    void init()
    {
        Base::init();
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorReadName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
