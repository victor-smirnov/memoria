
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorInsertName)


    using Container = typename Base::Container;


    static const int32_t Streams            = Container::Types::Streams;
    static const int32_t DataStreams        = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx = Container::Types::StructureStreamIdx;


    using CtrSizeT  = typename Container::Types::CtrSizeT;

public:
    template <typename IOBuffer>
    auto bulkio_insert(BufferProducer<IOBuffer>& provider, const int32_t initial_capacity = 20000)
    {
        auto& self = this->self();
        return self.ctr().bulkio_insert(self, provider, initial_capacity);
    }

public:

    template <int32_t Stream>
    struct InsertSymbolFn {

        CtrSizeT one_;
        const int32_t symbol_;

        InsertSymbolFn(int32_t symbol): one_(1), symbol_(symbol) {}

        const auto& get(const StreamTag<Stream>& , const StreamTag<0>&, int32_t block) const
        {
            return one_;
        }

        const auto& get(const StreamTag<Stream>& , const StreamTag<1>&, int32_t block) const
        {
            return symbol_;
        }
    };


    template <int32_t Stream, typename EntryFn>
    void insert_entry(EntryFn&& entry)
    {
        auto& self = this->self();

        std::function<OpStatus(int, int)> insert_fn = [&](int structure_idx, int stream_idx) -> OpStatus {
            auto status1 = self.ctr().template try_insert_stream_entry_no_mgr<StructureStreamIdx>(self.leaf(), structure_idx, InsertSymbolFn<StructureStreamIdx>(Stream));
            if (isOk(status1))
            {
                return self.ctr().template try_insert_stream_entry_no_mgr<Stream>(self.leaf(), stream_idx, std::forward<EntryFn>(entry));
            }

            return OpStatus::FAIL;
        };

        int structure_idx = self.idx();

        int32_t stream_idx = self.data_stream_idx(Stream, structure_idx);
        self.ctr().template insert_stream_entry0<Stream>(self, structure_idx, stream_idx, insert_fn);
    }


    template <int32_t Stream, typename SubstreamsList, typename EntryFn>
    void update_entry(EntryFn&& entry)
    {
        auto& self = this->self();
        int32_t key_idx = self.data_stream_idx(Stream);
        self.ctr().template update_stream_entry<Stream, SubstreamsList>(self, Stream, key_idx, std::forward<EntryFn>(entry));
    }



    SplitResult split(int32_t stream, int32_t target_stream_idx)
    {
        auto& self  = this->self();
        auto& leaf  = self.leaf();
        
        int32_t structure_size = self.structure_size();

        if (structure_size > 1)
        {
            int32_t structure_split_idx = structure_size / 2;

            auto half_ranks = self.leafrank(structure_split_idx);
            auto right      = self.ctr().split_leaf_p(leaf, half_ranks);

            auto& structure_idx = self.idx();

            if (structure_idx > structure_split_idx)
            {
                leaf = right;
                structure_idx -= structure_split_idx;

                self.refresh();
                
                return SplitResult(SplitStatus::RIGHT, target_stream_idx - half_ranks[stream]);
            }
            else {
                return SplitResult(SplitStatus::LEFT, target_stream_idx);
            }
        }
        else {
            auto leaf_sizes = self.leaf_sizes();

            self.ctr().split_leaf_p(leaf, leaf_sizes);
            
            return SplitResult(SplitStatus::LEFT, leaf_sizes[stream]);
        }
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorInsertName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
