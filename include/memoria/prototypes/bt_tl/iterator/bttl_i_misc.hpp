
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTTL_ITER_MISC_HPP
#define _MEMORIA_PROTOTYPES_BTTL_ITER_MISC_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>



#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::bttl::IteratorMiscName)

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Base::Container::Accumulator                               Accumulator;
    typedef typename Base::Container                                            Container;
    typedef typename Base::Container::Position                                  Position;

    using CtrSizeT 	= typename Container::Types::CtrSizeT;
    using Key		= typename Container::Types::Key;
    using Value		= typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    template <Int Stream>
    using InputTupleAdapter = typename Container::Types::template InputTupleAdapter<Stream>;

    template <Int Stream, typename SubstreamsIdxList, typename... Args>
    using ReadLeafEntryRtnType = typename Container::template ReadLeafStreamEntryRtnType<Stream, SubstreamsIdxList, Args...>;


    template <Int Stream>
    using StreamSizesPath = typename Select<Stream, typename Container::Types::StreamsSizes>::Result;

    static const Int Streams = Container::Types::Streams;

    CtrSizeT toData(CtrSizeT pos)
    {
    	MEMORIA_ASSERT(pos, >=, 0);

    	auto& self = this->self();

    	Int stream = self.stream();

    	if (stream < Streams - 1)
    	{

    	}
    	else {
    		return -1;
    	}
    }

    void toIndex()
    {

    }

//    void insertKey(Key key)
//    {
//    	auto& self = this->self();
//
//    	if (self.stream() == 0)
//    	{
//    		auto delta = key - self.keyPrefix();
//
//    		MEMORIA_ASSERT(delta, >, 0);
//
//    		self.ctr().template insertStreamEntry<0>(
//    				self,
//    				InputTupleAdapter<0>::convert(core::make_sv<Key>(delta, 0))
//    		);
//
//    		if (!self.isEnd())
//    		{
//    			auto k = self.rawKey();
//
//    			MEMORIA_ASSERT_TRUE(k - delta >= 0);
//
//    			self.ctr().template updateStreamEntry<0, IntList<0>>(self, std::make_tuple(std::make_pair(0, k - delta)));
//    		}
//
//    		self.template _skipBw<0>(1);
//    	}
//    	else {
//    		self.toIndex();
//    		self.insertKey(key);
//    	}
//    }
//
//    CtrSizeT addSize(SizeT delta)
//    {
//    	auto& self = this->self();
//
//    	if (self.stream() == 0)
//    	{
//    		auto current_size = self.cache().data_size();
//    		self.ctr().template updateStreamEntry<0, IntList<0>>(self, std::make_tuple(std::make_pair(1, current_size + delta)));
//
//    		return current_size + delta;
//    	}
//    	else {
//    		auto tmp = self;
//    		tmp.toIndex();
//
//    		auto new_size = tmp.addSize(delta);
//    		self.cache().data_size() = new_size;
//
//    		return new_size;
//    	}
//    }
//
//    void split(Int pos)
//    {
//    	auto& self = this->self();
//
//    	Int data_offset = self.leaf_data_offset();
//
//    	auto local_pos = self.toLocalPos(pos - data_offset);
//
//    	local_pos[0] ++;
//    	local_pos[1] += data_offset;
//
//    	self.ctr().splitLeafP(self.leaf(), local_pos);
//    }
//
//    void split()
//    {
//    	auto& self = this->self();
//
//    	auto sizes = self.ctr().getLeafStreamSizes(self.leaf());
//    	int total_size = sizes.sum();
//
//    	self.split(total_size  / 2);
//    }
//
//
//    void toIndex()
//    {
//    	auto& self = this->self();
//    	self.nextKey(0);
//    }
//
//
//    CtrSizeT nextKey(CtrSizeT size)
//	{
//    	auto& self = this->self();
//
//    	MEMORIA_ASSERT(size, >=, 0);
//
//    	if (self.stream() == 0)
//    	{
//    		return self.template _skipFw<0>(size);
//    	}
//    	else {
//    		Int idx 		= self.idx();
//    		Int data_offset = self.leaf_data_offset();
//
//    		self.stream() 	= 0;
//    		self.idx() 		= 0;
//
//    		if (idx >= data_offset)
//    		{
//    			Int local_data_pos = data_offset - idx;
//    			self.template _findFwGT<IntList<0, 0>>(1, local_data_pos);
//
//    			if (size > 0)
//    			{
//    				return self.template _skipFw<0>(size);
//    			}
//    			else {
//    				return 0;
//    			}
//    		}
//    		else {
//    			if (size > 0)
//    			{
//    				return self.template _skipFw<0>(size - 1);
//    			}
//    			else {
//    				return self.template _skipBw<0>(0);
//    			}
//    		}
//    	}
//    }
//
//
//    CtrSizeT operator+=(CtrSizeT size)
//    {
//    	auto& self = this->self();
//
//    	if (size >= 0)
//    	{
//    		if (self.stream() == 0)
//    		{
//    			return self.template _skipFw<0>(size);
//    		}
//    		else {
//    			return self.template _skipFw<1>(size);
//    		}
//    	}
//    	else {
//    		return self-=(-size);
//    	}
//    }
//
//    CtrSizeT operator-=(CtrSizeT size)
//    {
//    	auto& self = this->self();
//
//    	if (size >= 0)
//    	{
//    		if (self.stream() == 0)
//    		{
//    			return self.template _skipBw<0>(size);
//    		}
//    		else {
//    			return self.template _skipBw<1>(size);
//    		}
//    	}
//    	else {
//    		return self+=(-size);
//    	}
//    }
//
//    CtrSizeT seek(CtrSizeT pos)
//    {
//    	MEMORIA_ASSERT(pos, >=, 0);
//
//    	auto& self = this->self();
//
//    	self.isEnd();
//
//    	if (self.stream() == 0)
//    	{
//    		MEMORIA_ASSERT_TRUE(self.size() >= 0);
//
//    		auto node_data_offset 	= self.leaf_data_offset();
//
//    		self.stream() 	= 1;
//    		self.idx() 		= 0;
//
//    		return self.template _skip<1>(node_data_offset + pos);
//    	}
//    	else {
//    		auto data_size = self.size();
//    		auto local_pos = self.local_pos();
//
//    		auto target = pos < data_size ? pos : data_size;
//
//    		auto delta = pos - local_pos;
//
//    		self.template _skip<1>(delta);
//
//    		return target;
//    	}
//    }
//
    CtrSizeT leaf_data_offset() const
    {
    	auto& self = this->self();

    	auto data_prefix_by_idx = self.data_size_by_idx_prefix();
    	auto data_prefix 		= self.data_size_prefix();
//    	auto keys_size_prefix 	= self.keys_size_prefix();

    	auto node_data_offset 	= data_prefix_by_idx - data_prefix;// - keys_size_prefix;

    	return node_data_offset;
    }
//
//    CtrSizeT data_size_by_idx_prefix() const
//    {
//    	auto& cache = self().cache();
//    	return bt::Path<0, 0>::get(cache.prefixes())[0];
//    }
//
    CtrSizeT size(Int stream) const {
    	return self().cache().data_size(stream);
    }

//    CtrSizeT keys_size_prefix(Int stream) const {
//    	return self().cache().size_prefix(stream);
//    }
//
//    CtrSizeT data_size_prefix() const {
//    	return self().cache().size_prefix()[1];
//    }

    CtrSizeT pos() const
    {
    	auto& self = this->self();
    	return self.cache().data_pos(self.stream());
    }
//
//
//
//    CtrSizeT local_pos() const
//    {
//    	auto& self = this->self();
//    	MEMORIA_ASSERT_TRUE(self.stream() == 1);
//
//    	return self.idx() - self.data_size_prefix();
//    }
//
//
//
//
//    struct ToLocalFn {
//    	template <typename PackedStruct, typename T>
//    	Position stream(const PackedStruct* obj, Int block, Int start, T pos)
//    	{
//    		auto size = obj->size();
//
//    		Int c;
//    		for (c = 0; c < size; c++)
//    		{
//    			T val = obj->value(block, c);
//
//    			if (pos < val + 1) {
//    				break;
//    			}
//    			else {
//    				pos -= val + 1;
//    			}
//    		}
//
//    		return Position({c, pos});
//    	}
//    };
//
//
//    Position toLocalPos(Int idx) const
//    {
//    	auto& self = this->self();
//    	return std::get<0>(self.ctr().template _applySubstreamsFn<0, IntList<0>>(self.leaf(), ToLocalFn(), 1, 0, idx));
//    }
//
//    Position toLocalPos() const
//    {
//    	auto& self = this->self();
//
//    	auto idx = self.idx();
//
//    	if (self.stream() == 0)
//    	{
//    		auto sum = std::get<0>(self.ctr().template _sum<0, IntList<0>>(self.leaf(), 1, 0, idx));
//
//    		auto data_offset = self.leaf_data_offset();
//
//    		return Position({idx, data_offset + sum});
//    	}
//    	else {
//    		auto data_offset = self.leaf_data_offset();
//
//    		auto key_pos = std::get<0>(self.ctr().template _find<0, IntList<0>>(self.leaf(), SearchType::GT, 1, 0, idx - data_offset));
//
//    		return Position({key_pos, idx});
//    	}
//    }
//
//
//    Key key() const
//    {
//    	return self().rawKey() + self().keyPrefix();
//    }
//
//    auto rawKey() const -> typename std::tuple_element<0, ReadLeafEntryRtnType<0, IntList<0>, Int, Int>>::type
//    {
//    	auto& self = this->self();
//
//    	MEMORIA_ASSERT(self.stream(), ==, 0);
//
//    	return std::get<0>(self.ctr().template _readLeafStreamEntry<0, IntList<0>>(self.leaf(), self.idx(), 0));
//    }
//
//

    template <Int Stream>
    struct IdxDataSizeFn {
    	template <typename NTypes, typename... Args>
    	auto treeNode(const LeafNode<NTypes>* node, Args&&... args)
    	{
    		using Path = StreamSizesPath<Stream>;

			using StreamPath = bttl::BTTLSizePath<Path>;
			const Int index  = bttl::BTTLSizePathBlockIdx<Path>::Value;

    		return node->template substream<StreamPath>()->value(index, std::forward<Args>(args)...);
    	}
    };


    template <Int Stream>
    auto idx_data_size(Int idx) const
    {
    	auto& self = this->self();
    	return LeafDispatcher::dispatch(self.leaf(), IdxDataSizeFn<Stream>(), idx);
    }

    CtrSizeT pfx_size(Int stream) const {
    	return 0;
    }

    CtrSizeT pfx_count(Int stream) const {
    	return 0;
    }



//
//
//    auto idx_raw_key(Int idx) const -> typename std::tuple_element<0, ReadLeafEntryRtnType<0, IntList<0>, Int, Int>>::type
//    {
//    	auto& self = this->self();
//    	return std::get<0>(self.ctr().template _readLeafStreamEntry<0, IntList<0>>(self.leaf(), idx, 0));
//    }
//
//
//
//    Key keyPrefix() const
//    {
//    	auto& self = this->self();
//    	auto& cache = self.cache();
//
//    	return bt::Path<0, 0>::get(cache.prefixes())[0] +
//    		   bt::Path<0, 0>::get(cache.leaf_prefixes())[0];
//    }
//
//    void refreshCache()
//    {
//    	auto& self = this->self();
//    	self.template _refreshCache<0>();
//    }
//
//
//    template <typename Walker>
//    void finish_walking(Int idx, Walker& w, WalkCmd cmd) {
//    	Base::finish_walking(idx, w, cmd);
//    }
//

    void update_leaf_ranks()
    {
    	auto& self = this->self();
    	auto& prefixes = self.cache().ranks();
    	self.compute_leaf_prefixes(prefixes);
    }

    void update_leaf_ranks(WalkCmd cmd)
    {
    	if (cmd == WalkCmd::LAST_LEAF){
    		update_leaf_ranks();
    	}
    }

    template <typename WWTypes>
    void finish_walking(Int idx, const FindForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = FindForwardWalker<WWTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	auto start 	 = walker.branch_size_prefix_backup()[stream] + walker.idx_backup();
    	auto current = walker.branch_size_prefix()[stream] + idx;

    	pos[stream] += current - start;

    	if (stream < Streams - 1)
    	{
    		pos[stream + 1] = 0;

    		if (self.isContent(idx))
    		{
    			size[stream + 1] = self.template idx_data_size<Stream>(idx);
    		}
    		else {
    			size[stream + 1] = -1;
    		}
    	}

    	update_leaf_ranks(cmd);
    }

    template <typename WWTypes>
    void finish_walking(Int idx, const FindBackwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = FindBackwardWalker<WWTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	auto start 	 = walker.branch_size_prefix_backup()[stream] + walker.idx_backup();
    	auto current = walker.branch_size_prefix()[stream] + idx;

    	pos[stream] -= start - current;

    	if (stream < Streams - 1)
    	{
    		pos[stream + 1] = 0;

    		if (self.isContent(idx))
    		{
    			size[stream + 1] = self.template idx_data_size<Stream>(idx);
    		}
    		else {
    			size[stream + 1] = -1;
    		}
    	}

    	update_leaf_ranks(cmd);
    }


    template <typename WWTypes>
    void finish_walking(Int idx, const FindGEForwardWalker<WWTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = FindGEForwardWalker<WWTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	auto start 	 = walker.branch_size_prefix_backup()[stream] + walker.idx_backup();
    	auto current = walker.branch_size_prefix()[stream] + idx;

    	pos[stream] += current - start;

    	if (stream < Streams - 1)
    	{
    		pos[stream + 1] = 0;

    		if (self.isContent(idx))
    		{
    			size[stream + 1] = self.template idx_data_size<Stream>(idx);
    		}
    		else {
    			size[stream + 1] = -1;
    		}
    	}

    	update_leaf_ranks(cmd);
    }


    template <typename WTypes>
    void finish_walking(Int idx, const SkipForwardWalker<WTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = SkipForwardWalker<WTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	pos[stream] += walker.sum();

    	if (stream < Streams - 1)
    	{
    		if (self.isContent(idx))
    		{
    			size[stream + 1] = self.template idx_data_size<Stream>(idx);
    		}
    		else {
    			size[stream + 1] = -1;
    		}
    	}

    	update_leaf_ranks(cmd);
    }

    template <typename WTypes>
    void finish_walking(Int idx, const SkipBackwardWalker<WTypes>& walker, WalkCmd cmd)
    {
    	constexpr Int Stream = SkipBackwardWalker<WTypes>::Stream;

    	auto& self = this->self();
    	auto& cache = self.cache();

    	auto& pos  = cache.data_pos();
    	auto& size = cache.data_size();

    	auto stream = self.stream();

    	pos[stream] -= walker.sum();

    	if (stream < Streams - 1)
    	{
    		if (self.isContent(idx))
    		{
    			size[stream + 1] = self.template idx_data_size<Stream>(idx);
    		}
    		else {
    			size[stream + 1] = -1;
    		}
    	}

    	update_leaf_ranks(cmd);
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::bttl::IteratorMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS


#endif

