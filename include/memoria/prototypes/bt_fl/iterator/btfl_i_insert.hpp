
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

#include <memoria/core/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorInsertName)


    using Container = typename Base::Container;


    static const int32_t Streams            = Container::Types::Streams;
    static const int32_t DataStreams        = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx = Container::Types::StructureStreamIdx;


    using CtrSizeT  = typename Container::Types::CtrSizeT;

    auto insert_iovector(io::IOVectorProducer& provider, int64_t start, int64_t length)
    {
        auto& self = this->self();
        return self.ctr().ctr_insert_iovector(self, provider, start, length);
    }

    auto insert_iovector(io::IOVector& iovector, int64_t start, int64_t length)
    {
        auto& self = this->self();
        return self.ctr().ctr_insert_iovector(self, iovector, start, length);
    }



public:

    template <int32_t Stream>
    struct InsertSymbolFn {

        CtrSizeT one_;
        const int32_t symbol_;

        InsertSymbolFn(int32_t symbol): one_(1), symbol_(symbol) {}

        const auto& get(const bt::StreamTag<Stream>& , const bt::StreamTag<0>&, int32_t block) const
        {
            return one_;
        }

        const auto& get(const bt::StreamTag<Stream>& , const bt::StreamTag<1>&, int32_t block) const
        {
            return symbol_;
        }
    };


    template <int32_t Stream, typename EntryFn>
    VoidResult iter_insert_entry(EntryFn&& entry) noexcept
    {
        auto& self = this->self();

        std::function<OpStatus (int, int)> insert_fn = [&](int structure_idx, int stream_idx) -> OpStatus {
            auto status1 = self.ctr().template ctr_try_insert_stream_entry_no_mgr<StructureStreamIdx>(self.iter_leaf(), structure_idx, InsertSymbolFn<StructureStreamIdx>(Stream));
            if (isOk(status1))
            {
                return self.ctr().template ctr_try_insert_stream_entry_no_mgr<Stream>(self.iter_leaf(), stream_idx, std::forward<EntryFn>(entry));
            }

            return OpStatus::FAIL;
        };

        int structure_idx = self.iter_local_pos();

        int32_t stream_idx = self.data_stream_idx(Stream, structure_idx);
        return self.ctr().template ctr_insert_stream_entry0<Stream>(self, structure_idx, stream_idx, insert_fn);
    }


    template <int32_t Stream, typename SubstreamsList, typename EntryFn>
    VoidResult iter_update_entry(EntryFn&& entry) noexcept
    {
        auto& self = this->self();
        int32_t key_idx = self.data_stream_idx(Stream);
        self.ctr().template ctr_update_stream_entry<Stream, SubstreamsList>(self, Stream, key_idx, std::forward<EntryFn>(entry));
    }



    Result<SplitResult> split(int32_t stream, int32_t target_stream_idx) noexcept
    {
        using ResultT = Result<SplitResult>;

        auto& self  = this->self();
        auto leaf   = self.iter_leaf();
        
        int32_t structure_size = self.iter_structure_size();

        if (structure_size > 1)
        {
            int32_t structure_split_idx = structure_size / 2;

            auto half_ranks = self.iter_leafrank(structure_split_idx);
            auto right      = self.ctr().ctr_split_leaf(leaf, half_ranks);

            auto& structure_idx = self.iter_local_pos();

            if (structure_idx > structure_split_idx)
            {
                leaf.assign(right);
                structure_idx -= structure_split_idx;

                self.iter_refresh();
                
                return ResultT::of(SplitStatus::RIGHT, target_stream_idx - half_ranks[stream]);
            }
            else {
                return ResultT::of(SplitStatus::LEFT, target_stream_idx);
            }
        }
        else {
            auto iter_leaf_sizes = self.iter_leaf_sizes();

            self.ctr().ctr_split_leaf(leaf, iter_leaf_sizes);
            
            return ResultT::of(SplitStatus::LEFT, iter_leaf_sizes[stream]);
        }
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorInsertName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
