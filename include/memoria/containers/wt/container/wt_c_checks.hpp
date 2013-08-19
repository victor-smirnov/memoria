
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_WTLBLTREE_C_CHECKS_HPP
#define MEMORIA_CONTAINERS_WTLBLTREE_C_CHECKS_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/wt/wt_names.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::wt::CtrChecksName)

	typedef typename Base::Types                                                Types;
	typedef typename Base::Allocator                                            Allocator;

	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Allocator::Page::ID                                       	ID;

	typedef typename Base::LeafDispatcher                                       LeafDispatcher;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position 											Position;

	typedef typename Types::PageUpdateMgr										PageUpdateMgr;

	typedef typename Base::Types::LabelsTuple									LabelsTuple;

	static const Int Streams                                                    = Types::Streams;


	struct CheckContentFn {

		const MyType* 	ctr_;
		ID				id_;
		bool			root_;

		bool 			errors_ 	= false;


		CheckContentFn(const MyType& ctr, ID id, bool root):
			ctr_(&ctr),
			id_(id),
			root_(root)
		{}


		template <Int Idx, typename StreamTypes>
		void stream(const PkdVTree<StreamTypes>* labels)
		{
			if (labels != nullptr)
			{
				for (Int c = 0; c < labels->size(); c++)
				{
					auto value = labels->value(0, c);

					if (value == 0 && (c > 0 || !root_))
					{
						MEMORIA_ERROR(ctr_,
								"Node",
								id_,
								"has stream ",
								Idx,
								" with zero elements. idx=",
								c
						);

						errors_ = true;
					}
				}
			}
		}

		template <typename Node>
		void treeNode(const Node* node)
		{
			node->template processStream<2>(*this);
		}
	};


	bool checkContent(const NodeBaseG& node) const
	{
		if (!Base::checkContent(node))
		{
			if (node->is_leaf())
			{
				CheckContentFn fn(self(), node->id(), node->is_root());

				LeafDispatcher::dispatchConst(node, fn);

				return fn.errors_;
			}
			else {
				return false;
			}
		}
		else {
			return true;
		}
	}


MEMORIA_CONTAINER_PART_END

}


#endif
