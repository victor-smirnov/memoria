
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_CMAP_CTR_INSERT_HPP
#define _MEMORIA_CONTAINERS_CMAP_CTR_INSERT_HPP


#include <memoria/containers/cmap/cmap_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::cmap::CtrInsertName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Base::ID                                                   ID;

	typedef typename Types::NodeBase                                            NodeBase;
	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::TreeNodePage                                         TreeNodePage;
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

	typedef typename Types::PageUpdateMgr 										PageUpdateMgr;

	static const Int Indexes                                                    = Types::Indexes;
	static const Int Streams                                                    = Types::Streams;

	typedef std::pair<Key, Value> 												KVPair;

	typedef std::vector<KVPair> 												LeafPairsVector;
	typedef std::function<KVPair ()>											LeafPairProviderFn;

	static const UBigInt ActiveStreams											= -1ull;


	class MapSubtreeProvider: public Base::AbstractSubtreeProviderBase {

		typedef typename Base::AbstractSubtreeProviderBase 		ProviderBase;
		typedef typename Base::BalTreeNodeTraits				BalTreeNodeTraits;

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

			Int leaf_capacity = ctr_.getNodeTraitInt(BalTreeNodeTraits::MAX_CHILDREN, false, true);

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


    bool insert(Iterator& iter, const Element& element);

    void insertBatch(Iterator& iter, const LeafPairsVector& data);

    MEMORIA_DECLARE_NODE_FN(InsertLeafFn, insert);
    bool insertLeafEntry(Iterator& iter, const Element& element);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::cmap::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::insertLeafEntry(Iterator& iter, const Element& element)
{
	auto& self 		= this->self();

	TreePath&  path = iter.path();
	NodeBaseG& leaf = path.leaf();
	Int idx			= iter.idx();

	leaf.update();

	PageUpdateMgr mgr(self);

	try {
		mgr.add(leaf);

		LeafDispatcher::dispatch(leaf, InsertLeafFn(), idx, element.first, element.second);
	}
	catch (PackedOOMException ex)
	{
		mgr.rollback();
		return false;
	}

	self.updateUp(path, 1, path.leaf().parent_idx(), element.first, true);

	return true;
}


M_PARAMS
bool M_TYPE::insert(Iterator& iter, const Element& element)
{
	auto& self = this->self();

	TreePath&   path    = iter.path();
	NodeBaseG&  leaf    = path.leaf();
	Int&        idx     = iter.idx();
	Int 		stream  = iter.stream();



	Position leaf_sizes = self.getNodeSizes(leaf);

	if (self.isNodeEmpty(leaf))
	{
		self.initLeaf(leaf);
	}

	if (!self.insertLeafEntry(iter, element))
	{
		Position split_idx = leaf_sizes / 2;

		TreePath next = path;
		self.splitPath(path, next, 0, split_idx, ActiveStreams);

		if (idx >= split_idx[stream])
		{
			idx -= split_idx[stream];
			path = next;
		}

		MEMORIA_ASSERT_TRUE(self.insertLeafEntry(iter, element));
	}

	self.addTotalKeyCount(Position::create(stream, 1));

	return iter.next();
}





M_PARAMS
void M_TYPE::insertBatch(Iterator& iter, const LeafPairsVector& data)
{
	auto& self = this->self();
	auto& ctr  = self;

	TreePath& path = iter.path();
	Position idx(iter.entry_idx());

	Int pos = 0;

	MapSubtreeProvider provider(self, data.size(), [&](){
		return data[pos++];
	});

	if (ctr.isNodeEmpty(iter.leaf()))
	{
		ctr.layoutNode(iter.leaf(), ActiveStreams);
	}

	ctr.insertSubtree(path, idx, provider);

	ctr.addTotalKeyCount(Position(data.size()));

	if (iter.isEnd())
	{
		ctr.getNextNode(path);
		iter.key_idx() = 0;
	}

	for (UInt c = 0; c < data.size(); c++)
	{
		iter++;
	}
}







#undef M_PARAMS
#undef M_TYPE

}


#endif
