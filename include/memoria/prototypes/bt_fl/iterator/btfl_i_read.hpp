
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

#include <memoria/core/types.hpp>

#include <memoria/core/tools/object_pool.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/common/ctr_api_btfl.hpp>

#include <iostream>

namespace memoria {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorReadName)

    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::Container;

    using DataSizesT    = typename Container::Types::DataSizesT;

    static const int32_t Streams                = Container::Types::Streams;
    static const int32_t DataStreams            = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

#ifdef MMA_USE_IOBUFFER
    template <typename IOBuffer>
    using ReadWalkerPool = HeavyObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanThroughStrategy>>;

    template <typename IOBuffer>
    using ScanWalkerPool = HeavyObjectPool<btfl::io::BTFLWalker<MyType, IOBuffer, btfl::io::ScanRunGTStrategy>>;

    template <typename IOBuffer>
    using ScanRunWalkerPool = HeavyObjectPool<btfl::io::BTFLScanRunWalker<MyType, IOBuffer>>;
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
            int32_t size  = self.iter_structure_size();

            int32_t leaf_remainder = size - start;
            CtrSizesT to_copy;

            if (processed + leaf_remainder <= length)
            {
                to_copy = self.iter_leafrank(leaf_remainder);
            }
            else {
                to_copy = self.iter_leafrank(length - processed);
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
        return Base::iter_refresh();
    }

    void refresh_prefixes()
    {
        return Base::iter_refresh();
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorReadName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
