
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

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_output.hpp>

#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>



#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorReadName)

    using Container = typename Base::Container;

    using CtrSizeT      = typename Container::Types::CtrSizeT;
    using DataSizesT    = typename Container::Types::DataSizesT;

    static const Int Streams                = Container::Types::Streams;
    static const Int DataStreams            = Container::Types::DataStreams;
    static const Int StructureStreamIdx     = Container::Types::StructureStreamIdx;

    template <typename IOBuffer>
    using ReadWalkerPool = ObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanThroughStrategy>>;

    template <typename IOBuffer>
    using ScanWalkerPool = ObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanRunStrategy>>;


public:

    template <typename IOBuffer>
    CtrSizeT bulkio_read(BufferConsumer<IOBuffer>* consumer, const CtrSizeT& limits = std::numeric_limits<CtrSizeT>::max()) {
        return bulkio_read_<ReadWalkerPool>(consumer, -1, limits);
    }

    template <typename IOBuffer>
    CtrSizeT bulkio_scan(BufferConsumer<IOBuffer>* consumer, Int expected_stream = -1, const CtrSizeT& limits = std::numeric_limits<CtrSizeT>::max()) {
        return bulkio_read_<ScanWalkerPool>(consumer, expected_stream, limits);
    }

    template <typename IOBuffer>
    auto create_read_walker(CtrSizeT limit = std::numeric_limits<CtrSizeT>::max()) {
        return create_walker_<ReadWalkerPool, IOBuffer>(limit);
    }

    template <typename IOBuffer>
    auto create_scan_walker(CtrSizeT limit = std::numeric_limits<CtrSizeT>::max()) {
        return create_walker_<ScanWalkerPool, IOBuffer>(limit);
    }


    template <typename Walker, typename IOBuffer>
    Int bulkio_populate(Walker& walker, IOBuffer* buffer)
    {
        auto& self = this->self();

        auto start_id = self.leaf()->id();

        Int entries = 0;

        bool more_data = false;

        buffer->rewind();

        while (true)
        {
            auto result = walker.populate(*buffer);

            entries += result.entries();

            if (result.ending() == btfl::io::Ending::END_OF_PAGE)
            {
                if (!walker.next_page())
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
                throw Exception(MA_SRC, SBuf() << "Invalid populate IO buffer status: " << (Int) result.ending());
            }
        }

        self.idx()  = walker.idx();
        self.leaf() = walker.leaf();

        if (self.leaf()->id() != start_id)
        {
            self.refresh();
        }

        return more_data ? entries : -entries;
    }

protected:

    template <template <typename> class WalkerPoolT, typename IOBuffer>
    auto create_walker_(CtrSizeT limit)
    {
        auto& self = this->self();

        auto walker = self.ctr().pools().get_instance(PoolT<WalkerPoolT<IOBuffer>>()).get_unique();

        walker->init(self, limit);

        return walker;
    }


    template <template <typename> class WalkerPoolT, typename IOBuffer>
    CtrSizeT bulkio_read_(BufferConsumer<IOBuffer>* consumer, Int expected_stream, const CtrSizeT& limits)
    {
        auto& self = this->self();

        auto start_id = self.leaf()->id();

        auto walker = self.ctr().pools().get_instance(PoolT<WalkerPoolT<IOBuffer>>()).get_unique();

        walker->init(self, expected_stream, limits);

        IOBuffer& buffer = consumer->buffer();
        buffer.rewind();

        Int entries = 0;

        while (true)
        {
            auto result = walker->populate(buffer);

            entries += result.entries();

            if (result.ending() == btfl::io::Ending::END_OF_PAGE)
            {
                if (!walker->next_page())
                {
                    if (entries > 0)
                    {
                        buffer.rewind();
                        consumer->process(buffer, entries);
                        buffer.rewind();
                    }

                    entries = 0;

                    break;
                }
            }
            else if (result.ending() == btfl::io::Ending::END_OF_IOBUFFER)
            {
                if (entries > 0)
                {
                    buffer.rewind();
                    consumer->process(buffer, entries);
                    buffer.rewind();
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
                    buffer.rewind();
                    consumer->process(buffer, entries);
                    buffer.rewind();
                }
                else {
                    // put backward skip code here...
                }

                break;
            }
        }

        self.idx()  = walker->idx();
        self.leaf() = walker->leaf();

        CtrSizeT total = walker->totals();

        walker->clear();

        if (self.leaf()->id() != start_id)
        {
            self.refresh();
        }

        if (self.idx() >= self.leaf_size(StructureStreamIdx))
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

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorReadName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
