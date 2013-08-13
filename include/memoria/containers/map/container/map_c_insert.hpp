
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAP_CTR_INSERT_HPP
#define _MEMORIA_CONTAINERS_MAP_CTR_INSERT_HPP


#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map::CtrInsert1Name)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::NodeDispatcher                                       NodeDispatcher;
	typedef typename Base::RootDispatcher                                       RootDispatcher;
	typedef typename Base::LeafDispatcher                                       LeafDispatcher;
	typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


	typedef typename Base::Key                                                  Key;
	typedef typename Base::Value                                                Value;
	typedef typename Base::Element                                              Element;

	typedef typename Base::Metadata                                             Metadata;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Base::TreePath                                             TreePath;
	typedef typename Base::TreePathItem                                         TreePathItem;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;

	typedef std::pair<Key, Value> 												KVPair;

	typedef std::vector<KVPair> 												LeafPairsVector;
	typedef std::function<KVPair ()>											LeafPairProviderFn;

	typedef typename Types::PageUpdateMgr 										PageUpdateMgr;

	static const UBigInt ActiveStreams											= -1ull;


	class MapSubtreeProvider: public Base::AbstractSubtreeProviderBase {

		typedef typename Base::AbstractSubtreeProviderBase 		ProviderBase;
		typedef typename Base::BTNodeTraits						BTNodeTraits;

		MyType& ctr_;
		Int total_;
		Int inserted_;

		Int page_size_;

		LeafPairProviderFn pair_provider_fn_;

	public:
		MapSubtreeProvider(MyType& ctr, Int total, LeafPairProviderFn pair_provider_fn):
			ProviderBase(ctr),
			ctr_(ctr),
			total_(total),
			inserted_(0),
			pair_provider_fn_(pair_provider_fn)
		{
			page_size_ = ctr.getNewPageSize();
		}

		virtual BigInt getTotalKeyCount()
		{
			Int remainder = total_ - inserted_;

			Int leaf_capacity = ctr_.getNodeTraitInt(BTNodeTraits::MAX_CHILDREN, true);

			return remainder / leaf_capacity + (remainder % leaf_capacity ? 1 : 0);
		}

		virtual Position getTotalSize()
		{
			return Position(total_);
		}

		virtual Position getTotalInserted()
		{
			return Position(inserted_);
		}

		virtual Position remainder()
		{
			return Position(total_ - inserted_);
		}

		struct InsertIntoLeafFn {

			typedef Accumulator ReturnType;

			template <typename Node>
			Accumulator treeNode(Node* node, MapSubtreeProvider* provider, Int pos, Int remainder)
			{
				if (node->is_empty())
				{
					node->layout(provider->getActiveStreams());
				}

				Int size;
				if (remainder <= node->capacity())
				{
					size = remainder;
				}
				else {
					size = node->capacity();
				}

				node->insertSpace(Position(pos), Position(size));

				auto* tree 	= node->tree0();
				auto* value = node->values();

				for (Int c = pos; c < pos + size; c++)
				{
					auto pair 		= provider->pair_provider_fn_();
					tree->value(c) 	= pair.first;
					value[c]		= pair.second;
				}

				node->reindex();

				provider->inserted_ += size;

				return node->sum(pos, pos + size);
			}
		};

		virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)
		{
			return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, 0, remainder().get());
		}

		virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)
		{
			return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, from.get(), remainder().get());
		}

		virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size)
		{
			return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, from.get(), size.get());
		}
	};


    void insertBatch(Iterator& iter, const LeafPairsVector& data);




	struct InsertIntoLeafFn {

    	const Element& element_;

    	InsertIntoLeafFn(const Element& element): element_(element) {}

		template <Int Idx, typename StreamTypes>
		void stream(PackedFSEMap<StreamTypes>* map, Int idx)
		{
			MEMORIA_ASSERT_TRUE(map!= nullptr);
			map->insert(idx, std::get<Idx>(element_.first), element_.second);
		}

		template <typename NTypes>
		void treeNode(TreeNode<LeafNode, NTypes, true>* node, Int idx)
		{
			node->layout(1);
			node->template processStream<0>(*this, idx);
		}
	};


	bool insertIntoLeaf(NodeBaseG& leaf, Int idx, const Element& element);

	bool insertMapEntry(Iterator& iter, const Element& element);


	struct AddLeafFn {

    	const Accumulator& element_;

    	AddLeafFn(const Accumulator& element): element_(element) {}

		template <Int Idx, typename StreamTypes>
		void stream(PackedFSEMap<StreamTypes>* map, Int idx)
		{
			MEMORIA_ASSERT_TRUE(map != nullptr);

			map->tree()->addValues(idx, std::get<Idx>(element_));
		}

		template <typename NTypes>
		void treeNode(TreeNode<LeafNode, NTypes, true>* node, Int idx)
		{
			node->template processStream<0>(*this, idx);
		}
	};


	void updateLeafNode(NodeBaseG& node, Int idx, const Accumulator& sums, std::function<void (Int, Int)> fn);
	void updateUp(NodeBaseG& node, Int idx, const Accumulator& sums, std::function<void (Int, Int)> fn);

	void initLeaf(NodeBaseG& node) const
	{
		node.update();
		self().layoutNode(node, 1);
	}


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map::CtrInsert1Name)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::insertBatch(Iterator& iter, const LeafPairsVector& data)
{
	auto& self = this->self();
	auto& ctr  = self;

	Position idx(iter.entry_idx());

	Int pos = 0;

	MapSubtreeProvider provider(self, data.size(), [&](){
		return data[pos++];
	});

	if (ctr.isNodeEmpty(iter.leaf()))
	{
		ctr.layoutNode(iter.leaf(), ActiveStreams);
	}

	ctr.insertSubtree(iter.leaf(), idx, provider);

	ctr.addTotalKeyCount(Position(data.size()));

	if (iter.isEnd())
	{
		auto next = ctr.getNextNodeP(iter.leaf());

		if (next)
		{
			iter.leaf() = next;
			iter.idx() = 0;
		}
	}

	for (UInt c = 0; c < data.size(); c++)
	{
		iter++;
	}
}

M_PARAMS
bool M_TYPE::insertIntoLeaf(NodeBaseG& leaf, Int idx, const Element& element)
{
	auto& self = this->self();

	PageUpdateMgr mgr(self);

	leaf.update();

	mgr.add(leaf);

	try {
		LeafDispatcher::dispatch(leaf, InsertIntoLeafFn(element), idx);
		return true;
	}
	catch (PackedOOMException& e)
	{
		mgr.rollback();
		return false;
	}
}



M_PARAMS
bool M_TYPE::insertMapEntry(Iterator& iter, const Element& element)
{
	auto& self 		= this->self();
	NodeBaseG& leaf = iter.leaf();
	Int& idx		= iter.idx();

	if (!self.insertIntoLeaf(leaf, idx, element))
	{
		iter.split();
		if (!self.insertIntoLeaf(leaf, idx, element))
		{
			throw Exception(MA_SRC, "Second insertion attempt failed");
		}
	}

	self.updateParent(leaf, element.first);

	self.addTotalKeyCount(Position::create(0, 1));

	return iter++;
}


M_PARAMS
void M_TYPE::updateLeafNode(NodeBaseG& node, Int idx, const Accumulator& sums, std::function<void (Int, Int)> fn)
{
	auto& self = this->self();

	node.update();

	PageUpdateMgr mgr(self);

	try {
		LeafDispatcher::dispatch(node, AddLeafFn(sums), idx);
	}
	catch (PackedOOMException ex)
	{
		Position sizes = self.getNodeSizes(node);

		Position split_idx = sizes / 2;

		auto next = self.splitLeafP(node, split_idx);

		if (idx >= split_idx[0])
		{
			idx -= split_idx[0];
			fn(0, idx);
			node = next;
		}

		LeafDispatcher::dispatch(node, AddLeafFn(sums), idx);
	}
}


M_PARAMS
void M_TYPE::updateUp(NodeBaseG& node, Int idx, const Accumulator& counters, std::function<void (Int, Int)> fn)
{
	auto& self = this->self();

	self.updateLeafNode(node, idx, counters, fn);
	self.updateParent(node, counters);
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
