
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_VECTORMAP_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector_map/vmap_names.hpp>
#include <memoria/containers/vector_map/vmap_tools.hpp>
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
			Int size = self.leaf_size(0);

			if (self.idx() < size - 1)
			{
				self.idx()++;

				auto entry = self.entry();

				self.cache().add(entry.first, entry.second, self.idx());
				self.cache().addToGlobalPos(entry.second);

				return true;
			}
			else if (self.nextLeaf())
			{
				auto entry = self.entry();
				self.cache().add(entry.first, entry.second, 0);
				self.cache().addToGlobalPos(entry.second);
				self.cache().setEntries(self.leaf_size(0));

				return true;
			}
			else {
				size = self.leaf_size(0);
				self.idx() = size;
				self.cache().add(0, 0, size);
				self.cache().setEntries(size);
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
			if (self.idx() > 0)
			{
				self.idx()--;

				auto entry = self.entry();

				self.cache().sub(entry.first, entry.second, self.idx());
				self.cache().addToGlobalPos(-entry.second);

				return true;
			}
			else
			{
				if (self.prevLeaf())
				{
					Int entries = self.leafSize(self.stream());

					self.idx() = entries - 1;

					MEMORIA_ASSERT_TRUE(self.idx() >= 0);

					auto entry = self.entry();
					auto& cache= self.cache();

					cache.sub(entry.first, entry.second, self.idx());
					cache.addToGlobalPos(-entry.second);
					cache.setEntries(entries);

					return true;
				}
				else {
					self.idx() = -1;
					self.cache().sub(0, 0, -1);

					return false;
				}
			}
		}
		else {
			self.findEntry();
			return self.prevEntry();
		}
	}


	bool isEnd() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 0);

		return self.idx() >= self.leafSize(0);
	}

	bool isBegin() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 0);

		return self.idx() < 0 || self.leafSize(0) == 0;
	}

	bool isEof() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 1);

		BigInt pos = self.pos();
		BigInt size = self.cache().size();

		return size == 0 || pos >= size;
	}

	bool isBof() const
	{
		auto& self = this->self();
		MEMORIA_ASSERT_TRUE(self.stream() == 1);

		BigInt pos = self.pos();

		return self.blob_size() == 0 || pos < 0;
	}


	BigInt id() const
	{
		return self().cache().id();
	}

	struct PrefixFn {
		BigInt prefix_ = 0;

		Int stream_;
		Int block_;

		PrefixFn(Int stream, Int block): stream_(stream), block_(block) {}

		template <typename NodeTypes, bool leaf>
		void treeNode(const TreeNode<LeafNode, NodeTypes, leaf>* node, Int idx)
		{
			if (stream_ == 0)
			{
				node->template processStream<0>(*this, idx);
			}
		}

		template <typename NodeTypes, bool leaf>
		void treeNode(const TreeNode<BranchNode, NodeTypes, leaf>* node, Int idx)
		{
			node->sum(stream_, block_, 0, idx, prefix_);
		}

		template <Int StreamIdx, typename Tree>
		void stream(const Tree* tree, Int idx)
		{
			prefix_ += tree->sum(block_, idx);
		}
	};


	BigInt global_pos() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 1);

		return self.cache().global_pos();
	}

	BigInt leaf_blob_base() const
	{
		auto& self = this->self();

		PrefixFn fn(1, 0);

		self.ctr().walkUp(self.leaf(), 0, fn);

		MEMORIA_ASSERT_TRUE(fn.prefix_ >= 0);

		return fn.prefix_;
	}


	Int first_entry_base() const
	{
		auto& self = this->self();

		PrefixFn fn(0, 1);

		self.ctr().walkUp(self.leaf(), 0, fn);

		Int base = fn.prefix_ - self.leaf_blob_base();

		MEMORIA_ASSERT_TRUE(base >= 0);

		return base;
	}

	struct EntryBlobBaseFn {
		BigInt prefix_ = 0;

		template <typename Node>
		void treeNode(const Node* node, Int idx)
		{
			node->template processStream<0>(*this, idx);
		}

		template <Int StreamIdx, typename TreeTypes>
		void stream(const PkdFTree<TreeTypes>* tree, Int idx)
		{
			prefix_ += tree->sum(1, idx);
		}
	};


	BigInt entry_blob_base(Int entry_idx) const
	{
		auto& self = this->self();

		EntryBlobBaseFn fn;

		self.ctr().walkUp(self.leaf(), 0, fn);

		MEMORIA_ASSERT_TRUE(fn.prefix_ >= 0);

		return fn.prefix_;
	}

	BigInt pos() const
	{
		auto& self = this->self();

		BigInt global_pos = self.global_pos();
		BigInt blob_base  = self.cache().blob_base();

		return global_pos - blob_base;
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
		MEMORIA_ASSERT_TRUE(self.idx() >= 0);

		MEMORIA_ASSERT(self.idx(), <, self.leaf_size());

		return LeafDispatcher::dispatchConstRtn(self.leaf(), EntryFn(), self.idx());
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
			MEMORIA_ASSERT_TRUE(obj != nullptr);

			return obj->sum(block, idx);
		}
	};

	Int data_offset() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 0);

		Int local_offset = LeafDispatcher::dispatchConstRtn(self.leaf(), LocalDataOffsetFn(), 1, self.idx());

		MEMORIA_ASSERT_TRUE(local_offset >= 0);

		Int local_base = self.first_entry_base();

		return local_base + local_offset;
	}

	Int data_offset_for(Int entry_idx) const
	{
		auto& self = this->self();

		Int local_offset 		= LeafDispatcher::dispatchConstRtn(self.leaf(), LocalDataOffsetFn(), 1, entry_idx);

		BigInt entry_blob_base 	= self.entry_blob_base(entry_idx);

		Int local_base 			= entry_blob_base - self.leaf_blob_base();

		MEMORIA_ASSERT_TRUE(local_base >= 0);

		return local_base + local_offset;
	}

	BigInt findData(BigInt offset = 0)
	{
		return seek(offset);
	}

	BigInt seek(BigInt offset)
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			self.seekLocal();

			if (self.blob_size() > 0)
			{
				return self.skipFw(offset);
			}
			else {
				return 0;
			}
		}
		else
		{
			BigInt pos = self.pos();
			return self.skip(offset - pos);
		}
	}


	void seekLocal()
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			Int data_offset = self.data_offset();

			self.stream() 	= 1;
			self.idx() 		= data_offset;

			self.cache().set_positions(self.cache().blob_base());
		}
		else
		{
			findEntry();
			seekLocal();
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
			return obj->findLTForward(1, 0, offset).idx();
		}
	};

	void findEntry()
	{
		auto& self = this->self();

		if (self.stream() == 1)
		{
			BigInt offset = self.pos();
			self.skip(-offset);

			if (self.leaf_size(0) == 0 || self.idx() < self.data_offset_for(0))
			{
				self.stream() = 0;

				if (self.prevLeaf())
				{
					self.idx() = self.cache().entry_idx();
				}
				else {
					throw Exception(MA_SRC, "VectorMap data integrity error: no entries page");
				}
			}
			else {
				self.stream() 	= 0;
				self.idx() 		= self.cache().entry_idx();
			}

//			else {
//				Int first_entry_offset = self.data_offset_for(0);
//
//
//
//				throw Exception(MA_SRC, "VectorMap: handle this case...");
//			}
		}
	}

	BigInt skip(BigInt offset)
	{
		auto& self = this->self();

		if (offset >= 0)
		{
			return self.skipFw(offset);
		}
		else {
			return self.skipBw(-offset);
		}
	}

	BigInt skipFw(BigInt amount)
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			return self.findData(amount);
		}
		else if (self.blob_size() > 0)
		{
			BigInt size = self.blob_size();
			BigInt pos  = self.pos();

			if (pos + amount < size)
			{
				BigInt offset = self.template _findFw<vmap::SkipForwardWalker>(0, amount);

				self.cache().addToGlobalPos(offset);

				return offset;
			}
			else
			{
				BigInt offset = self.template _findFw<vmap::SkipForwardWalker>(0, size - pos - 1);

				self.idx()++;

				self.cache().addToGlobalPos(offset + 1);

				return offset + 1;
			}
		}
		else {
			return 0;
		}
	}

	BigInt skipBw(BigInt amount)
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 1);

		if (self.blob_size() > 0)
		{
			BigInt pos  = self.pos();

			if (amount >= pos + 1)
			{
				amount = pos + 1;
			}

			BigInt offset = self.template _findBw<vmap::SkipBackwardWalker>(0, amount);

			if (amount > 0)
			{
				if (offset > 0) {
					self.cache().addToGlobalPos(-offset);
				}
				else {
					self.cache().set_positions(-1);
				}
			}

			return offset;
		}
		else {
			return 0;
		}
	}


    BigInt read(DataTarget& tgt)
    {
    	auto& self = this->self();

    	vmap::VectorMapTarget target(&tgt);

    	Position pos;

    	pos[1] = self.idx();

    	return self.ctr().readStreams(self, pos, target)[1];
    }


    void insert(DataSource& src)
    {
    	auto& self = this->self();

    	MEMORIA_ASSERT_TRUE(self.stream() == 1);

    	self.ctr().insertData(self, src);
    }

    void remove(BigInt size)
    {
    	auto& self = this->self();

    	MEMORIA_ASSERT_TRUE(self.stream() == 1);

    	self.ctr().removeData(self, size);
    }



    struct ReadValueFn {
    	typedef Value ReturnType;
    	typedef Value ResultType;

    	template <typename Node>
    	ReturnType treeNode(const Node* node, Int offset)
    	{
    		return node->template processStreamRtn<1>(*this, offset);
    	}

    	template <Int StreamIdx, typename StreamType>
    	ResultType stream(const StreamType* obj, Int offset)
    	{
    		return obj->value(offset);
    	}
    };

    Value value()
    {
    	auto& self = this->self();
    	MEMORIA_ASSERT_TRUE(self.stream() == 1);

    	return LeafDispatcher::dispatchConstRtn(self.leaf(), ReadValueFn(), self.idx());
    }

    struct UpdateFn {

    	template <typename Node>
    	void treeNode(Node* node, Int offset, const Accumulator& keys)
    	{
    		node->template processStream<0>(*this, offset, keys);
    	}

    	template <Int StreamIdx, typename StreamType>
    	void stream(StreamType* obj, Int offset, const Accumulator& keys)
    	{
    		obj->updateUp(0, offset, std::get<0>(keys)[0]);
    		obj->updateUp(1, offset, std::get<0>(keys)[1]);
    		obj->reindex();
    	}
    };

    void update(const Accumulator& accum)
    {
    	auto& self = this->self();

    	MEMORIA_ASSERT_TRUE(self.stream() == 0);

    	NodeBaseG& leaf = self.leaf();

    	leaf.update();
    	LeafDispatcher::dispatch(leaf, UpdateFn(), self.idx(), accum);

    	self.ctr().updateParent(leaf, accum);

    	self.cache().addToEntry(
    		std::get<0>(accum)[0],
    		std::get<0>(accum)[1]
    	);
    }

    bool checkCapacities(const Position& sizes)
    {
    	auto& self = this->self();
    	return self.ctr().checkCapacities(self.leaf(), sizes);
    }

    void dump() const {
    	auto cache = self().cache();

    	cout<<"Cache: id=" <<cache.id()
    					<<" id_prefix="<<cache.id_prefix()
    					<<" id_entry="<<cache.id_entry()
    					<<" base="<<cache.blob_base()
    					<<" size="<<cache.size()
    					<<" idx="<<cache.entry_idx()
    					<<" entries="<<cache.entries()
    					<<endl;
    	Base::dump();
    }

    struct InitStateFn {
    	Accumulator prefix_;

    	template <typename Node>
    	void treeNode(const Node* node, Int idx)
    	{
    		node->template processStream<0>(*this, idx);
    	}

    	template <Int StreamIdx, typename TreeTypes>
    	void stream(const PkdFTree<TreeTypes>* tree, Int idx)
    	{
    		if (tree != nullptr)
    		{
    			std::get<StreamIdx>(prefix_)[0] += tree->sum(0, idx);
    			std::get<StreamIdx>(prefix_)[1] += tree->sum(1, idx);
    		}
    	}
    };

    void initState()
    {
    	auto& self = this->self();

    	InitStateFn fn;

    	self.ctr().walkUp(self.leaf(), self.idx(), fn);

    	BigInt id_prefix 	= std::get<0>(fn.prefix_)[0];
    	BigInt base 		= std::get<0>(fn.prefix_)[1];

    	BigInt id_entry;
    	BigInt size;

    	if (!self.isEnd())
    	{
    		auto entry = self.entry();

    		id_entry = entry.first;
    		size 	 = entry.second;
    	}
    	else {
    		id_entry = 0;
    		size 	 = 0;
    	}

    	BigInt global_pos 	= base;

    	self.cache().setup(id_prefix, id_entry, base, size, self.idx(), self.leaf_size(0), global_pos);
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::vmap::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS

#endif
