
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

//	bool operator++() {
//		return self().skipFw(1);
//	}
//
//	bool operator--() {
//		return self().skipBw(1);
//	}
//
//	bool operator++(int) {
//		return self().skipFw(1);
//	}
//
//	bool operator--(int) {
//		return self().skipFw(1);
//	}
//
//	BigInt operator+=(BigInt size)
//	{
//		return self().skipFw(size);
//	}
//
//	BigInt operator-=(BigInt size)
//	{
//		return self().skipBw(size);
//	}
//
//	bool isEof() const {
//		return self().key_idx() >= self().size();
//	}
//
//	bool isBof() const {
//		return self().key_idx() < 0;
//	}

	BigInt id() const
	{
		return self().cache().id();
	}

	BigInt global_pos() const
	{
		auto& self = this->self();

		MEMORIA_ASSERT_TRUE(self.stream() == 1);

		return self.cache().blob_leaf_base() + self.key_idx();
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

	void findData(BigInt offset = 0)
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
//			BigInt global_data_offset = self.cache().blob_base();

			self.stream() = 1;
		}
	}

	void findEntry()
	{
		auto& self = this->self();

		if (self.stream() == 1)
		{
//			BigInt global_pos = self.global_pos();
//
//
//
//			self.stream() = 0;
		}
	}

	BigInt skipFw(BigInt amount)
	{
		auto& self = this->self();

		if (self.stream() == 0)
		{
			self.findData(amount);
		}

		return self.template _findFw<vmap::FindLTForwardWalker>(0, amount);
	}

	BigInt skipBw(BigInt amount)
	{
		auto& self = this->self();
		return self.template _findFw<vmap::FindLTForwardWalker>(0, amount);
	}

	void init()
	{
//		auto& self = this->self();
//
//		BlobDescriptorEntry entry = self.entry();
//		self.cache().setup(entry.first);
	}


	void ComputePrefix(BigInt& accum)
	{
//		TreePath&   path0 = self().path();
//		Int         idx   = self().key_idx();
//
//		for (Int c = 1; c < path0.getSize(); c++)
//		{
//			idx = path0[c - 1].parent_idx();
//			self().model().sumKeys(path0[c].node(), 0, 0, idx, accum);
//		}
	}

	void ComputePrefix(Accumulator& accum)
	{
//		TreePath&   path0 = self().path();
//		Int         idx   = self().key_idx();
//
//		for (Int c = 1; c < path0.getSize(); c++)
//		{
//			idx = path0[c - 1].parent_idx();
//			self().model().sumKeys(path0[c].node(), 0, idx, accum);
//		}
	}

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::vmap::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS

#endif
