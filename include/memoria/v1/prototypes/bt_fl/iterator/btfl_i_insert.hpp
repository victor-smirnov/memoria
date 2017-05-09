
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

protected:

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
        self.ctr().template insert_stream_entry<StructureStreamIdx>(self, StructureStreamIdx, self.idx(), InsertSymbolFn<StructureStreamIdx>(Stream));

        int32_t key_idx = self.data_stream_idx(Stream);
        self.ctr().template insert_stream_entry<Stream>(self, Stream, key_idx, std::forward<EntryFn>(entry));
    }




    SplitResult split(int32_t stream, int32_t target_idx)
    {
        auto& self  = this->self();
        auto& leaf  = self.leaf();
        
        int32_t structure_size = self.structure_size();

        if (structure_size > 1)
        {
            int32_t split_idx = structure_size / 2;

            auto half_ranks = self.leafrank(split_idx);
            auto right      = self.ctr().split_leaf_p(leaf, half_ranks);

            auto& idx = self.idx();

            if (idx > split_idx)
            {
                leaf = right;
                idx -= split_idx;

                self.refresh();
                
                return SplitResult(SplitStatus::RIGHT, idx);
            }
            else {
                return SplitResult(SplitStatus::LEFT, idx);
            }

//             if (target_idx > half_ranks[stream - 1])
//             {
//                 leaf = right;
//                 target_idx -= half_ranks[stream - 1];
// 
//                 return SplitResult(SplitStatus::RIGHT, target_idx);
//             }
//             else {
//                 return SplitResult(SplitStatus::LEFT, target_idx);
//             }
        }
        else {
            //auto ranks = self.leaf_sizes();

            self.ctr().split_leaf_p(leaf, self.leaf_sizes());
            
            return SplitResult(SplitStatus::LEFT, self.idx());

//             if (target_idx > ranks[stream])
//             {
//                 target_idx -= ranks[stream];
// 
//                 return SplitResult(SplitStatus::RIGHT, target_idx);
//             }
//             else {
//                 return SplitResult(SplitStatus::LEFT, target_idx);
//             }
        }
    }





//    template <int32_t StreamIdx, typename EntryBuffer>
//    SplitStatus _insert(const EntryBuffer& data)
//    {
//        auto& self  = this->self();
//        auto stream = self.stream();
//
//        MEMORIA_V1_ASSERT(StreamIdx, ==, stream);
//
//        auto main_split_status = self.ctr().template insert_stream_entry<StreamIdx>(self, data);
//
//        auto tmp = self;
//        tmp.toIndex();
//
//        if (tmp.has_same_leaf(self))
//        {
//            auto status = tmp.add_substream_size(tmp.stream(), tmp.idx(), 1);
//
//            if (status == SplitStatus::NONE)
//            {
//                return main_split_status;
//            }
//            else
//            {
//                auto pos = self.pos();
//
//                self = tmp;
//                self.toData(pos);
//
//                return SplitStatus::UNKNOWN;
//            }
//        }
//        else {
//            tmp.add_substream_size(tmp.stream(), tmp.idx(), 1);
//        }
//
//        return main_split_status;
//    }





MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorInsertName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
