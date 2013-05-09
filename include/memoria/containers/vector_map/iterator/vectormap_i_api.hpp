
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTORMAP_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector_map/vectormap_names.hpp>
#include <memoria/containers/vector_map/vectormap_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {




MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::vmap::ItrApiName)

	typedef Ctr<typename Types::CtrTypes>                      	Container;


	typedef typename Base::Allocator                                            Allocator;
	typedef typename Base::NodeBase                                             NodeBase;
	typedef typename Base::NodeBaseG                                            NodeBaseG;
	typedef typename Base::TreePath                                             TreePath;

	typedef typename Container::Value                                     		Value;
	typedef typename Container::Key                                       		Key;
	typedef typename Container::Element                                   		Element;
	typedef typename Container::Accumulator                               		Accumulator;

	typedef typename Container::DataSource                                		DataSource;
	typedef typename Container::DataTarget                                		DataTarget;
	typedef typename Container::LeafDispatcher                                	LeafDispatcher;
	typedef typename Container::Position										Position;

	typedef std::pair<BigInt, BigInt> 											BlobDescriptorEntry; // ID, Size

	bool operator++()
	{
		return self().nextEntry();
	}

	bool operator++(Int)
	{
		return self().nextEntry();
	}

	bool operator--() {
		return self().prevEntry();
	}

	bool operator--(Int) {
		return self().prevEntry();
	}

	bool nextEntry()
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			Int size = self.leafSize(0);

			if (self.key_idx() < size - 1)
			{
				self.key_idx()++;

				auto entry = self.entry();

				self.cache().add(entry.first, entry.second);

				return true;
			}
			else if (self.nextLeaf())
			{
				auto entry = self.entry();
				self.cache().add(entry.first, entry.second);

				return true;
			}
			else {
				self.key_idx() = size;
				self.cache().add(0, 0);
			}

			return false;
		}
		else {
			self.findEntry();
			return self.nextEntry();
		}
	}

	bool prevEntry()
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			if (self.key_idx() > 0)
			{
				self.key_idx()--;

				auto entry = self.entry();

				self.cache().sub(entry.first, entry.second);

				return true;
			}
			else if (self.prevLeaf())
			{
				auto entry = self.entry();
				self.cache().sub(entry.first, entry.second);

				return true;
			}
			else {
				self.key_idx() = -1;
				self.cache().sub(0, 0);

				return false;
			}
		}
		else {
			self.findEntry();
			return self.prevEntry();
		}
	}

	bool isEof() const
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			return self.key_idx() >= self.leafSize(0);
		}
		else {
			if (self.key_idx() < self.leafSize(1))
			{
				return self.pos() >= self.cache().size();
			}
			else {
				return true;
			}
		}
	}
//
//	bool isBof() const {
//		return self().key_idx() < 0;
//	}

	BigInt id() const
	{
		return self().cache().id();
	}

	struct GlobalPosFn {
		BigInt prefix_ = 0;

		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeLeafNode, NodeTypes, root, leaf>* node, Int idx) {}

		template <typename NodeTypes, bool root, bool leaf>
		void treeNode(const TreeNode<TreeMapNode, NodeTypes, root, leaf>* node, Int idx)
		{
			node->sum(0, 0, 0, idx, prefix_);
		}
	};

	BigInt global_pos() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 1);

		GlobalPosFn fn;

		self.model().walkUp(self.path(), self.key_idx(), fn);

		return fn.prefix_ + self.key_idx();
	}

	BigInt pos() const
	{
		auto& self = this->self();
		return self.global_pos() - self.cache().blob_base();
	}

	BigInt blob_size() const {
		return self().cache().size();
	}

	bool isBlobEof() const
	{
		auto& self = this->self();

		BigInt size = self.blob_size();
		BigInt pos  = self.pos();

		return pos < size;
	}

	bool isBlobBof() const
	{
		auto& self = this->self();

		BigInt global_pos  	= self.global_pos();
		BigInt blob_base	= self.cache().blob_base();

		return global_pos < blob_base;
	}

	struct EntryFn {
		typedef BlobDescriptorEntry ReturnType;
		typedef BlobDescriptorEntry ResultType;

		template <typename Node>
		ReturnType treeNode(const Node* node, Int idx)
		{
			return node->template processStreamRtn<0>(*this, idx);
		}

		template <Int StreamIdx, typename StreamType>
		ResultType stream(StreamType* obj, Int idx)
		{
			BlobDescriptorEntry entry;

			entry.first  = obj->value(0, idx);
			entry.second = obj->value(1, idx);

			return entry;
		}
	};

	BlobDescriptorEntry entry() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 0);

		return LeafDispatcher::dispatchConstRtn(self.leaf().node(), EntryFn(), self.key_idx());
	}

	struct LocalDataOffsetFn {
		typedef Int ReturnType;
		typedef Int ResultType;


		template <typename Node>
		ReturnType treeNode(const Node* node, Int block, Int idx)
		{
			return node->template processStreamRtn<0>(*this, block, idx);
		}

		template <Int StreamIdx, typename StreamType>
		ResultType stream(const StreamType* obj, Int block, Int idx)
		{
			return obj->sum(block, idx);
		}
	};

	Int data_offset() const
	{
		auto& self = this->self();
		return LeafDispatcher::dispatchConstRtn(self.leaf().node(), LocalDataOffsetFn(), 0, self.key_idx());
	}



	BigInt findData(BigInt offset = 0)
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			Int data_offset = self.data_offset();
			self.stream() = 1;

			return self.skipFw(data_offset + offset) - data_offset;
		}
		else
		{
			findEntry();
			return findData(offset);
		}
	}

	struct FindEntryFn {
		typedef Int ReturnType;
		typedef Int ResultType;

		template <typename Node>
		ReturnType treeNode(const Node* node, Int offset)
		{
			return node->template processStreamRtn<0>(*this, offset);
		}

		template <Int StreamIdx, typename StreamType>
		ResultType stream(const StreamType* obj, Int offset)
		{
			return obj->findLTForward(1, offset).idx();
		}
	};

	void findEntry()
	{
		auto& self = this->self();

		if (self.stream() == 1)
		{
			BigInt offset = self.pos();
			self.skipBw(offset);

			Int local_offset = self.key_idx();

			Int entry_idx = LeafDispatcher::dispatchConstRtn(self.leaf().node(), FindEntryFn(), local_offset);

			self.key_idx() = entry_idx;

			self.stream() = 0;
		}
	}

	BigInt skipFw(BigInt amount)
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			return self.findData(amount);
		}
		else {
			return self.template _findFw<vmap::SkipForwardWalker>(0, amount);
		}
	}

	BigInt skipBw(BigInt amount)
	{
		auto& self = this->self();
		return self.template _findBw<vmap::SkipBackwardWalker>(0, amount);
	}


    BigInt read(DataTarget& tgt)
    {
    	auto& self = this->self();

    	vmap::VectorMapTarget target(&tgt);

    	Position pos;

    	pos[1] = self.key_idx();

    	return self.model().readStreams(self, pos, target)[1];
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::vmap::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS

#endif
