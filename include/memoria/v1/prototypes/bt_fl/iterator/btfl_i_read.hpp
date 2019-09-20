
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
    using CtrSizesT     = typename Container::Types::CtrSizesT;

    static const int32_t Streams                = Container::Types::Streams;
    static const int32_t DataStreams            = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;
#ifdef MMA1_USE_IOBUFFER
    template <typename IOBuffer>
    using ReadWalkerPool = ObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanThroughStrategy>>;

    template <typename IOBuffer>
    using ScanWalkerPool = ObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanRunGTStrategy>>;

    template <typename IOBuffer>
    using ScanRunWalkerPool = ObjectPool<btfl::io::BTFLScanRunWalker<MyType, IOBuffer>>;
#endif

public:


    auto read_to(io::IOVectorConsumer& consumer, int64_t length)
    {
        auto& self = this->self();

        std::unique_ptr<io::IOVector> iov = Types::LeafNode::create_iovector();

        auto processed = self.populate(*iov.get(), length);

        consumer.consume(*iov.get());

        return processed;
    }

    auto populate(io::IOVector& io_vector, int64_t length)
    {
        auto& self = this->self();

        auto& view = self.iovector_view();

        int64_t processed{};

        while (processed < length)
        {
            int32_t start = self.iter_local_pos();
            int32_t size  = self.structure_size();

            int32_t leaf_remainder = size - start;
            CtrSizesT to_copy;

            if (processed + leaf_remainder <= length)
            {
                to_copy = self.leafrank(leaf_remainder);
            }
            else {
                to_copy = self.leafrank(length - processed);
            }

            auto to_copy_symbols = to_copy.sum();

            view.symbol_sequence().copy_to(
                io_vector.symbol_sequence(),
                start,
                to_copy_symbols
            );

            for (int32_t stream = 0, ss_start = 0; stream < view.streams(); stream++)
            {
                int32_t ss_end = ss_start + view.stream_size(stream);

                for (int32_t ss = ss_start; ss < ss_end; ss++)
                {
                    view.substream(ss).copy_to(
                                io_vector.substream(ss), start, to_copy[stream]
                    );
                }

                ss_start = ss_end;
            }

            processed += to_copy_symbols;

            if (!self.iter_next_leaf()) {
                break;
            }
        }

        io_vector.reindex();

        return processed;
    }




    void iter_refresh()
    {
        Base::iter_refresh();
    }

    void refresh_prefixes()
    {
        Base::iter_refresh();
    }


    void iter_check_prefix() {
        auto tmp = self();

        tmp.iter_refresh();

        MEMORIA_V1_ASSERT(self().iter_cache(), ==, tmp.iter_cache());
    }

//    void prepare() {
//        Base::prepare();
//
//        auto& self = this->self();
//        auto& iter_cache = self.iter_cache();
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
