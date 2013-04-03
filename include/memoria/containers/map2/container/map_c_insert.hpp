
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_IDX_MAP2_C_INSERT_HPP
#define _MEMORIA_MODELS_IDX_MAP2_C_INSERT_HPP


#include <memoria/containers/map2/map_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::map2::CtrInsertName)

	typedef typename Base::WrappedCtr::Types                                  	WTypes;
	typedef typename Base::WrappedCtr::Allocator                              	Allocator;

	typedef typename Base::WrappedCtr::ID                                     	ID;

	typedef typename WTypes::NodeBase                                           NodeBase;
	typedef typename WTypes::NodeBaseG                                          NodeBaseG;

	typedef typename Base::Iterator                                             Iterator;

	typedef typename WTypes::Pages::NodeDispatcher                              NodeDispatcher;
	typedef typename WTypes::Pages::RootDispatcher                              RootDispatcher;
	typedef typename WTypes::Pages::LeafDispatcher                              LeafDispatcher;
	typedef typename WTypes::Pages::NonLeafDispatcher                           NonLeafDispatcher;


	typedef typename WTypes::Key                                                Key;
	typedef typename WTypes::Value                                              Value;
	typedef typename WTypes::Element                                            Element;

	typedef typename WTypes::Metadata                                           Metadata;

	typedef typename WTypes::Accumulator                                        Accumulator;
	typedef typename WTypes::Position											Position;

	typedef typename WTypes::TreePath                                           TreePath;
	typedef typename WTypes::TreePathItem                                       TreePathItem;

	typedef std::pair<Key, Value> 												KVPair;

	typedef std::vector<KVPair> 												LeafPairsVector;
	typedef std::function<KVPair ()>											LeafPairProviderFn;



	static const Int Indexes = WTypes::Indexes;

	class MapSubtreeProvider: public Base::WrappedCtr::DefaultSubtreeProviderBase {

		typedef typename Base::WrappedCtr::DefaultSubtreeProviderBase 		ProviderBase;
		typedef typename Base::WrappedCtr::BalTreeNodeTraits				BalTreeNodeTraits;

		MyType& ctr_;
		Int total_;
		Int inserted_;

		Int page_size_;

		LeafPairProviderFn pair_provider_fn_;

	public:
		MapSubtreeProvider(MyType& ctr, Int total, LeafPairProviderFn pair_provider_fn):
			ProviderBase(ctr.ctr()),
			ctr_(ctr),
			total_(total),
			inserted_(0),
			pair_provider_fn_(pair_provider_fn)
		{
			page_size_ = ctr.ctr().getNewPageSize();
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
			Accumulator operator()(Node* node, MapSubtreeProvider* provider, Int pos, Int remainder)
			{
				Int size;
				if (remainder <= node->capacity())
				{
					size = remainder;
				}
				else {
					size = node->capacity();
				}

				node->insertSpace(Position(pos), Position(size));

				auto* tree 	= node->map().tree();
				auto* value = node->map().values();

				for (Int c = pos; c < pos + size; c++)
				{
					auto pair 		= provider->pair_provider_fn_();
					tree->value(c) 	= pair.first;
					value[c]		= pair.second;
				}

				node->reindex();

//				node->map().dump();

				provider->inserted_ += size;

				return node->maxKeys();
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



    void insertEntry(Iterator& iter, const Element&);

    bool insert(Iterator& iter, const Element& element)
    {
    	insertEntry(iter, element);
    	return iter.next();
    }

    void insertBatch(Iterator& iter, const LeafPairsVector& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map2::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::insertBatch(Iterator& iter, const LeafPairsVector& data)
{
	auto& self = this->self();
	auto& ctr  = self.ctr();

	TreePath& path = iter.path();
	Position idx(iter.entry_idx());

	Int pos = 0;

	MapSubtreeProvider provider(self, data.size(), [&](){
		return data[pos++];
	});

	ctr.insertSubtree(path, idx, provider);

	ctr.addTotalKeyCount(data.size());

	if (iter.isEnd())
	{
		ctr.getNextNode(path);
		iter.iter().key_idx() = 0;
	}

	for (UInt c = 0; c < data.size(); c++)
	{
		iter++;
	}
}




M_PARAMS
void M_TYPE::insertEntry(Iterator &iter, const Element& element)
{
    TreePath&   path    = iter.iter().path();
    NodeBaseG&  node    = path.leaf().node();
    Int&        idx     = iter.iter().key_idx();

    auto& ctr  = self().ctr();

    if (ctr.getCapacity(node) > 0)
    {
        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }
    else if (idx == 0)
    {
        TreePath next = path;
        ctr.splitPath(path, next, 0, Position(node->children_count() / 2));
        idx = 0;

        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }
    else if (idx < node->children_count())
    {
        //FIXME: does it necessary to split the page at the middle ???
        TreePath next = path;
        ctr.splitPath(path, next, 0, Position(idx));
        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }
    else {
        TreePath next = path;

        ctr.splitPath(path, next, 0, Position(node->children_count() / 2));

        path = next;

        idx = node->children_count();
        ctr.makeRoom(path, 0, Position(idx), Position(1));
    }

    self().setLeafDataAndReindex(node, idx, element);

    ctr.updateParentIfExists(path, 0, element.first);

    ctr.addTotalKeyCount(1);
}



#undef M_PARAMS
#undef M_TYPE

}


#endif
