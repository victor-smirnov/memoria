
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTOR2_C_INSERT_HPP
#define _MEMORIA_CONTAINER_VECTOR2_C_INSERT_HPP


#include <memoria/containers/vector2/vector_names.hpp>
#include <memoria/containers/vector2/vector_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::balanced_tree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector2::CtrInsertName)

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
	typedef typename Base::DefaultDispatcher                                    DefaultDispatcher;


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

	typedef typename Types::DataSource											DataSource;
	typedef typename Types::DataTarget											DataTarget;






	template <typename Node>
	class LayoutManager: public INodeLayoutManager {
		Int block_size_;
	public:
		LayoutManager(Int block_size): block_size_(block_size) {}

		virtual Int getNodeCapacity(const Int* sizes, Int stream)
		{
			return Node::capacity(block_size_, sizes, stream);
		}
	};

	struct GetTotalNodesFn {
		typedef Int ReturnType;

		template <typename Node>
		ReturnType treeNode(const Node*, Int block_size, const Int* sizes, Int stream)
		{
			return Node::capacity(block_size, sizes, stream);
		}
	};

	class LeafLayoutManager: public INodeLayoutManager {
		Int block_size_;
	public:
		LeafLayoutManager(Int block_size): block_size_(block_size) {}
		virtual Int getNodeCapacity(const Int* sizes, Int stream)
		{
			return LeafDispatcher::dispatchStatic2Rtn(false, true, GetTotalNodesFn(), block_size_, sizes, stream);
		}
	};

	class VectorSubtreeProvider: public Base::DefaultSubtreeProviderBase {

		typedef typename Base::DefaultSubtreeProviderBase 		ProviderBase;
		typedef typename Base::BalTreeNodeTraits				BalTreeNodeTraits;

		MyType& 	ctr_;
		Position 	total_;
		Position 	inserted_;

		ISource& data_source_;

	public:
		VectorSubtreeProvider(MyType& ctr, const Position& total, ISource& data_source):
			ProviderBase(ctr),
			ctr_(ctr),
			total_(total),
			data_source_(data_source)
		{}

		virtual BigInt getTotalKeyCount()
		{
			LeafLayoutManager manager(ctr_.getRootMetadata().page_size());
			return data_source_.getTotalNodes(&manager);
		}

		virtual Position getTotalSize()
		{
			return total_;
		}

		virtual Position getTotalInserted()
		{
			return inserted_;
		}

		virtual Position remainder()
		{
			return total_ - inserted_;
		}

		struct InsertIntoLeafFn {

			typedef Accumulator ReturnType;

			template <typename Node>
			Accumulator treeNode(
				Node* node,
				VectorSubtreeProvider* provider,
				const Position* pos,
				const Position* remainder
			)
			{
				Position size;
				if (remainder->lteAll(node->capacities()))
				{
					size = *remainder;
				}
				else {
					size = node->capacities();
				}

				LayoutManager<Node> manager(node->page_size());

				provider->data_source_.newNode(&manager);

				node->insert(provider->data_source_, *pos, size);

				node->reindex();

				provider->inserted_ += size;

				return node->sum(*pos, (*pos) + size);
			}
		};

		virtual Accumulator insertIntoLeaf(NodeBaseG& leaf)
		{
			Position pos;
			Position remainder = this->remainder();
			return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, &pos, &remainder);
		}

		virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from)
		{
			Position remainder = this->remainder();
			return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, &from, &remainder);
		}

		virtual Accumulator insertIntoLeaf(NodeBaseG& leaf, const Position& from, const Position& size)
		{
			return LeafDispatcher::dispatchRtn(leaf, InsertIntoLeafFn(), this, &from, &size);
		}
	};




    void insert(Iterator& iter, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::mvector2::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
void M_TYPE::insert(Iterator& iter, DataSource& data)
{
	auto& self = this->self();
	auto& ctr  = self;

	TreePath& path = iter.path();
	Position idx(iter.key_idx());

	mvector2::VectorSource source(&data);

	VectorSubtreeProvider provider(self, Position(data.getSize()), source);

	ctr.insertSubtree(path, idx, provider);

	ctr.addTotalKeyCount(data.getSize());

	if (iter.isEof())
	{
		iter.nextLeaf();
	}

	iter.skipFw(data.getSize());
}





#undef M_PARAMS
#undef M_TYPE

}


#endif
