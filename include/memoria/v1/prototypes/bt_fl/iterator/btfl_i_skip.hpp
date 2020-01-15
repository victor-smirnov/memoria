
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

#include <memoria/v1/prototypes/bt/nodes/leaf_node.hpp>
#include <memoria/v1/prototypes/bt/nodes/branch_node.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorSkipName)

    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using Container = typename Base::Container;

    template <typename LeafPath>
    using AccumItemH = typename Container::Types::template AccumItemH<LeafPath>;

    static const int32_t Streams                = Container::Types::Streams;
    static const int32_t DataStreams            = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

    using DataSizesT = typename Container::Types::DataSizesT;

public:
    bool iter_is_begin() const
    {
        auto& self = this->self();
        return self.iter_local_pos() < 0 || self.iter_is_empty();
    }

    bool iter_is_end() const
    {
        auto& self = this->self();

        return self.iter_leaf().node().isSet() ? self.iter_local_pos() >= self.iter_leaf_size(StructureStreamIdx) : true;
    }

    bool is_end() const
    {
        auto& self = this->self();
        return self.iter_leaf().node().isSet() ? self.iter_local_pos() >= self.iter_leaf_size(StructureStreamIdx) : true;
    }

    bool iter_is_end(int32_t idx) const
    {
        auto& self = this->self();

        return self.iter_leaf().node().isSet() ? idx >= self.iter_leaf_size(StructureStreamIdx) : true;
    }

    bool iter_is_content() const
    {
        auto& self = this->self();
        return !(self.iter_is_begin() || self.iter_is_end());
    }

    bool iter_is_content(int32_t idx) const
    {
        auto& self = this->self();

        bool is_set = self.iter_leaf().node().isSet();

        auto iter_leaf_size = self.iter_leaf_size(StructureStreamIdx);

        return is_set && idx >= 0 && idx < iter_leaf_size;
    }

    bool iter_is_not_end() const
    {
        return !self().iter_is_end();
    }

    bool iter_is_empty() const
    {
        auto& self = this->self();
        return self.iter_leaf().node().isEmpty() || self.iter_leaf_size(StructureStreamIdx) == 0;
    }

    bool iter_is_not_empty() const
    {
        return !self().iter_is_empty();
    }

    void iter_dump_keys(std::ostream& out) const
    {
        auto& self = this->self();

        out << "Stream:  " << self.iter_data_stream_s() << std::endl;
        out << "Idx:  " << self.iter_local_pos() << std::endl;
    }





    bool next() {
        return self().iter_btfl_skip_fw(1) > 0;
    }

    bool prev() {
        return self().iter_btfl_skip_bw(1) > 0;
    }


    struct SkipFWFn {
    	template <int32_t StreamIdx, typename IterT>
    	auto process(IterT&& iter, CtrSizeT n)
    	{
    		return iter.template iter_skip_fw<StreamIdx>(n);
    	}
    };


    CtrSizeT iter_btfl_skip_fw(CtrSizeT n)
    {
    	auto& self = this->self();
    	int32_t stream = self.iter_stream();
        return bt::ForEachStream<Streams - 1>::process(stream, SkipFWFn(), self, n);
    }

    struct SkipBWFn {
    	template <int32_t StreamIdx, typename IterT>
    	auto process(IterT&& iter, CtrSizeT n)
    	{
    		return iter.template iter_skip_bw<StreamIdx>(n);
    	}
    };

    CtrSizeT iter_btfl_skip_bw(CtrSizeT n)
    {
    	auto& self = this->self();
    	int32_t stream = self.iter_stream();
        return bt::ForEachStream<Streams - 1>::process(stream, SkipBWFn(), self, n);
    }

    CtrSizeT skip(CtrSizeT n)
    {
        if (n > 0) {
            return iter_skip_fw(n);
        }
        else {
            return iter_skip_bw(n);
        }
    }

    int32_t iter_data_stream() const
    {
        auto& self  = this->self();
        auto s      = self.leaf_structure();

        if (s != nullptr)
        {
        	auto idx    = self.iter_local_pos();

        	if (idx < s->size())
        	{
        		return s->get_symbol(idx);
        	}
        }

        return -1;//throw Exception(MA_SRC, "End Of Data Structure");
    }

    int32_t iter_data_stream_s() const
    {
        auto& self  = this->self();
        auto s      = self.leaf_structure();

        if (s != nullptr)
        {
            auto idx = self.iter_local_pos();

        	if (idx < s->size())
        	{
        		return s->get_symbol(idx);
        	}
        }

        return -1;
    }









    CtrSizeT pos() const
    {
        auto& self = this->self();
        return self.template iter_stream_size_prefix<StructureStreamIdx>() + self.iter_local_pos();
    }

//    CtrSizeT stream_pos() const
//    {
//
//        return fn.pos_ + self.iter_local_pos();
//    }


private:

    template <int32_t Stream>
    struct SizePrefix {
        CtrSizeT pos_ = 0;

        template <typename NodeTypes>
        void treeNode(const bt::BranchNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
        {
            using BranchSizePath = IntList<Stream>;

            auto sizes_substream = node->template substream<BranchSizePath>();

            pos_ += sizes_substream->sum(0, end);
        }

        template <typename NodeTypes>
        void treeNode(const bt::LeafNode<NodeTypes>* node, WalkCmd cmd, int32_t start, int32_t end)
        {
        }
    };

public:
    template <int32_t Stream>
    auto iter_stream_size_prefix() const
    {
        auto& self = this->self();
        SizePrefix<Stream> fn;

        self.ctr().ctr_walk_tree_up(self.iter_leaf(), self.iter_local_pos(), fn);

        return fn.pos_;
    }

public:

    int iter_structure_stream_idx(int stream, int data_idx)
    {
        auto& self = this->self();
        auto s = self.leaf_structure();

        if (s != nullptr)
        {
            auto result = s->selectFW(data_idx + 1, stream);

            if (result.is_found())
            {
                return result.iter_local_pos();
            }
            else {
                return self.iter_leaf_size(StructureStreamIdx);
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Structure stream is empty");
        }
    }

    void iter_to_structure_stream()
    {
    	auto& self = this->self();

    	if (self.iter_stream() != StructureStreamIdx)
    	{
    		int32_t stream = self.iter_stream();

    		int32_t data_idx = self.iter_local_pos();

    		auto s = self.leaf_structure();

    		if (s != nullptr)
    		{
                //auto result = s->selectFW(stream, data_idx);
                auto result = s->selectFW(data_idx + 1, stream);

    			self.iter_stream() = StructureStreamIdx;

    			if (result.is_found())
    			{
                    self.iter_local_pos() = result.local_pos();
    			}
    			else {
    				self.iter_local_pos() = self.iter_leaf_size(StructureStreamIdx);
    			}
    		}
    		else {
                MMA1_THROW(Exception()) << WhatCInfo("Structure stream is empty");
    		}
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: {}", self.iter_stream()));
    	}
    }

    void iter_to_data_stream(int32_t stream)
    {
    	auto& self = this->self();

    	if (self.iter_stream() == StructureStreamIdx)
    	{
            int32_t data_idx = self.data_stream_idx(stream);
            self.iter_local_pos() = data_idx;
            self.iter_stream() 	  = stream;
    	}
    	else {
            MMA1_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: {}", self.iter_stream()));
    	}
    }

    int32_t data_stream_idx(int32_t stream) const
    {
        auto& self = this->self();
        return self.data_stream_idx(stream, self.iter_local_pos());
    }

    int32_t data_stream_idx(int32_t stream, int32_t structure_idx) const
    {
        auto& self = this->self();

        auto s = self.leaf_structure();
        if (s != nullptr) {
        	return s->rank(structure_idx, stream);
        }
        else {
        	return 0;
        }
    }

    CtrSizesT iter_leafrank() const {
        return self().iter_leafrank(self().iter_local_pos());
    }


    CtrSizesT iter_leafrank(int32_t structure_idx) const
    {
        auto& self = this->self();

        auto leaf_structure = self.leaf_structure();

        if (leaf_structure != nullptr) {

        	CtrSizesT ranks;

        	for (int32_t c = 0; c < DataStreams; c++)
        	{
        		ranks[c] = leaf_structure->rank(structure_idx, c);
        	}

        	ranks[StructureStreamIdx] = structure_idx;

        	return ranks;
        }
        else {
        	CtrSizesT ranks;
        	ranks[StructureStreamIdx] = structure_idx;
        	return ranks;
        }
    }

    CtrSizeT iter_leafrank(int32_t stream, int32_t structure_idx) const
    {
        auto& self = this->self();

        auto leaf_structure = self.leaf_structure();

        if (leaf_structure != nullptr)
        {
            return leaf_structure->rank(structure_idx, stream);
        }
        else {
            return 0;
        }
    }


    int32_t iter_structure_size() const
    {
        return self().iter_leaf_size(StructureStreamIdx);
    }


    const auto* leaf_structure() const
    {
        auto& self = this->self();
        return self.ctr().template ctr_get_packed_struct<IntList<StructureStreamIdx, 1>>(self.iter_leaf());
    }

    int32_t symbol_idx(int32_t stream, int32_t position) const
    {
        auto& self = this->self();

        auto s = self.leaf_structure();

        if (s != nullptr)
        {
            return self.leaf_structure()->selectFW(position + 1, stream).local_pos();
        }
        else {
        	return 0;
        }
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorSkipName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
