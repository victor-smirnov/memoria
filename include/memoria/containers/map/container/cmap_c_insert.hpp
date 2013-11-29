
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_CMAP_CTR_INSERT_HPP
#define _MEMORIA_CONTAINERS_CMAP_CTR_INSERT_HPP


#include <memoria/containers/map/map_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::map::CtrCInsertName)

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

    typedef ValuePair<Accumulator, Value>                                   	Element;

    struct InsertIntoLeafFn {

        const Element& element_;
        bool next_entry_updated_ = false;

        InsertIntoLeafFn(const Element& element): element_(element) {}

        template <Int Idx, typename StreamTypes>
        void stream(PackedVLEMap<StreamTypes>* map, Int idx)
        {
            MEMORIA_ASSERT_TRUE(map);

            using Values = typename PackedVLEMap<StreamTypes>::Values;

            Values values;

            values.assignDown(std::get<Idx>(element_.first));

            map->insert(idx, values, element_.second);

            next_entry_updated_ = idx < map->size() - 1;

            if (next_entry_updated_)
            {
            	map->addValue(0, idx + 1, -values[0]);
            }
        }


        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx)
        {
            node->layout(1);
            node->template processStream<0>(*this, idx);
        }
    };


    std::pair<bool, bool> insertIntoLeaf(NodeBaseG& leaf, Int idx, const Element& element);

    bool insertMapEntry(Iterator& iter, const Element& element);


    struct AddLeafFn {

        const Accumulator& element_;

        AddLeafFn(const Accumulator& element): element_(element) {}

        template <Int Idx, typename StreamTypes>
        void stream(PackedVLEMap<StreamTypes>* map, Int idx)
        {
            MEMORIA_ASSERT_TRUE(map);
            map->addValues(idx, std::get<Idx>(element_));
        }

        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx)
        {
            node->template processStream<0>(*this, idx);
        }
    };

    template <typename DataType>
    struct AddLeafSingleFn {

        const bt::SingleIndexUpdateData<DataType>& element_;

        AddLeafSingleFn(const bt::SingleIndexUpdateData<DataType>& element): element_(element) {}

        template <Int Idx, typename StreamTypes>
        void stream(PackedVLEMap<StreamTypes>* map, Int idx)
        {
            MEMORIA_ASSERT_TRUE(map);
            map->addValue(element_.index() - 1, idx, element_.delta());
        }

        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx)
        {
            node->template processStream<0>(*this, idx);
        }
    };


    template <typename DataType>
    void updateLeafNode(
    		NodeBaseG& node,
    		Int idx,
    		const bt::SingleIndexUpdateData<DataType>& sums,
    		std::function<void (Int, Int)> fn
    	);

    template <typename DataType>
    void updateUp(
    		NodeBaseG& node,
    		Int idx,
    		const bt::SingleIndexUpdateData<DataType>& sums,
    		std::function<void (Int, Int)> fn
    	);

    void initLeaf(NodeBaseG& node) const
    {
    	auto& self = this->self();

    	self.updatePageG(node);
        self.layoutNode(node, 1);
    }


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map::CtrCInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
std::pair<bool, bool> M_TYPE::insertIntoLeaf(NodeBaseG& leaf, Int idx, const Element& element)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    self.updatePageG(leaf);

    mgr.add(leaf);

    try {
    	InsertIntoLeafFn fn(element);

        LeafDispatcher::dispatch(leaf, fn, idx);

        return std::pair<bool, bool>(true, fn.next_entry_updated_);
    }
    catch (PackedOOMException& e)
    {
        mgr.rollback();
        return std::pair<bool, bool>(false, false);
    }
}



M_PARAMS
bool M_TYPE::insertMapEntry(Iterator& iter, const Element& element)
{
    auto& self      = this->self();
    NodeBaseG& leaf = iter.leaf();
    Int& idx        = iter.idx();

    std::pair<bool, bool> result = self.insertIntoLeaf(leaf, idx, element);

    if (!result.first)
    {
        iter.split();

        result = self.insertIntoLeaf(leaf, idx, element);

        if (!result.first)
        {
            throw Exception(MA_SRC, "Second insertion attempt failed");
        }
    }

    self.addTotalKeyCount(Position::create(0, 1));

    if (result.second)
    {
    	auto entry_sums = element.first;
    	std::get<0>(entry_sums)[1] = 0;

    	self.updateParent(leaf, entry_sums);

    	return iter++;
    }
    else {
    	self.updateParent(leaf, element.first);

    	if (iter++)
    	{
    		iter.updateUp(1, -(std::get<0>(element.first)[1]));
    		return true;
    	}
    	else {
    		return false;
    	}
    }
}


M_PARAMS
template <typename DataType>
void M_TYPE::updateLeafNode(
				NodeBaseG& node,
				Int idx,
				const bt::SingleIndexUpdateData<DataType>& sums,
				std::function<void (Int, Int)> fn
	)
{
    auto& self = this->self();

    self.updatePageG(node);

    PageUpdateMgr mgr(self);

    try {
        LeafDispatcher::dispatch(node, AddLeafSingleFn<DataType>(sums), idx);
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

        LeafDispatcher::dispatch(node, AddLeafSingleFn<DataType>(sums), idx);
    }
}


M_PARAMS
template <typename DataType>
void M_TYPE::updateUp(
		NodeBaseG& node,
		Int idx,
		const bt::SingleIndexUpdateData<DataType>& counters,
		std::function<void (Int, Int)> fn
	)
{
    auto& self = this->self();

    self.updateLeafNode(node, idx, counters, fn);
    self.updateParent(node, counters);
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
