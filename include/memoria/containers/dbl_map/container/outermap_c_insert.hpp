
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_OUTERMAP_C_INSERT_HPP
#define MEMORIA_CONTAINERS_DBLMAP2_OUTERMAP_C_INSERT_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::dblmap::OuterCtrInsertName)

	typedef typename Base::Types                                                Types;

	typedef typename Types::NodeBaseG                                           NodeBaseG;
	typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

	typedef typename Types::Accumulator                                         Accumulator;
	typedef typename Types::Position                                            Position;

	static const Int Streams                                                    = Types::Streams;

	typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

	struct InsertIntoLeafFn {

		const Accumulator& element_;

		InsertIntoLeafFn(const Accumulator& element): element_(element) {}


		template <Int Idx, typename StreamTypes>
		void stream(PkdFTree<StreamTypes>* map, Int idx)
		{
			MEMORIA_ASSERT_TRUE(map != nullptr);
			map->insert(idx, std::get<Idx>(element_));
		}


		template <typename NTypes>
		void treeNode(LeafNode<NTypes>* node, Int idx)
		{
			node->layout(1);
			node->template processStream<0>(*this, idx);
		}
	};

	bool insertIntoLeaf(NodeBaseG& leaf, Int idx, const Accumulator& element);
	bool insertMapEntry(Iterator& iter, const Accumulator& element);


	struct AddLeafFn {

		const Accumulator& element_;

		AddLeafFn(const Accumulator& element): element_(element) {}

		template <Int Idx, typename StreamTypes>
		void stream(PkdFTree<StreamTypes>* map, Int idx)
		{
			MEMORIA_ASSERT_TRUE(map != nullptr);

			map->addValues(idx, std::get<Idx>(element_));
		}

		template <typename NTypes>
		void treeNode(LeafNode<NTypes>* node, Int idx)
		{
			node->template processStream<0>(*this, idx);
		}
	};


	void updateLeafNode(NodeBaseG& node, Int idx, const Accumulator& sums, std::function<void (Int, Int)> fn);
	void updateUp(NodeBaseG& node, Int idx, const Accumulator& sums, std::function<void (Int, Int)> fn);

	void initLeaf(NodeBaseG& node) const
	{
		auto& self = this->self();

		self.updatePageG(node);
		self.layoutNode(node, 1);
	}

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::dblmap::OuterCtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



M_PARAMS
bool M_TYPE::insertIntoLeaf(NodeBaseG& leaf, Int idx, const Accumulator& element)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    self.updatePageG(leaf);

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
bool M_TYPE::insertMapEntry(Iterator& iter, const Accumulator& element)
{
    auto& self      = this->self();
    NodeBaseG& leaf = iter.leaf();
    Int& idx        = iter.idx();

    if (!self.insertIntoLeaf(leaf, idx, element))
    {
        iter.split();
        if (!self.insertIntoLeaf(leaf, idx, element))
        {
            throw Exception(MA_SRC, "Second insertion attempt failed");
        }
    }

    self.updateParent(leaf, element);

    self.addTotalKeyCount(Position::create(0, 1));

    self.markCtrUpdated();

    return iter++;
}


M_PARAMS
void M_TYPE::updateLeafNode(NodeBaseG& node, Int idx, const Accumulator& sums, std::function<void (Int, Int)> fn)
{
    auto& self = this->self();

    self.updatePageG(node);

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
