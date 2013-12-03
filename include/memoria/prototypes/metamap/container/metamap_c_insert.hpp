
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_METAMAP_CTR_INSERT_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_CTR_INSERT_HPP


#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::metamap::CtrInsertName)

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

    typedef typename Types::Entry                                         		MapEntry;

    template <typename Entry>
    struct InsertIntoLeafFn {

        const Entry& entry_;
        Accumulator& sums_;
        bool adjust_next_;

        bool next_entry_updated_ = false;

        InsertIntoLeafFn(const Entry& entry, Accumulator& sums, bool adjust_next):
        	entry_(entry), sums_(sums),
        	adjust_next_(adjust_next)
        {}

        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream, Int idx)
        {
            MEMORIA_ASSERT_TRUE(stream);

            metamap::InsertEntry(stream, idx, entry_, std::get<StreamIdx>(sums_));

            if (adjust_next_)
            {
            	next_entry_updated_ = idx < stream->size() - 1;

            	if (next_entry_updated_)
            	{
            		stream->addValue(0, idx + 1, -entry_.key());
            	}
            }
            else {
            	next_entry_updated_ = true;
            }
        }


        template <typename NTypes>
        void treeNode(LeafNode<NTypes>* node, Int idx)
        {
            node->layout(1);
            node->template processStream<0>(*this, idx);
        }
    };


    template <typename Entry>
    std::pair<bool, bool> insertIntoLeaf(
    		NodeBaseG&
    		leaf,
    		Int idx,
    		const Entry& entry,
    		Accumulator& sums,
    		bool adjust_next
    );

    template <typename Entry>
    void insertEntry(Iterator& iter, const Entry& entry, bool adjust_next = true);


    template <typename DataType>
    struct AddLeafSingleFn: bt1::NoRtnNodeWalkerBase<AddLeafSingleFn<DataType>>
    {
        const bt::SingleIndexUpdateData<DataType>& element_;

        AddLeafSingleFn(const bt::SingleIndexUpdateData<DataType>& element): element_(element) {}

        template <Int Idx, typename Stream>
        void stream(Stream* stream, Int idx)
        {
            MEMORIA_ASSERT_TRUE(stream);
            stream->addValue(element_.index() - 1, idx, element_.delta());
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

    Iterator insertIFNotExists(Key key)
    {
    	Iterator iter = self().findGE(0, key, 1);

    	if (iter.isEnd() || key != iter.key())
    	{
    		MapEntry entry;
    		entry.key() = key;

    		self().insertEntry(iter, entry);

    		iter--;
    	}
    	else {
    		throw Exception(MA_SRC, "Inserted Key already exists");
    	}

    	return iter;
    }





MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::metamap::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS


M_PARAMS
template <typename Entry>
std::pair<bool, bool> M_TYPE::insertIntoLeaf(
		NodeBaseG& leaf,
		Int idx,
		const Entry& entry,
		Accumulator& sums,
		bool adjust_next
	)
{
    auto& self = this->self();

    PageUpdateMgr mgr(self);

    self.updatePageG(leaf);

    mgr.add(leaf);

    try {
    	InsertIntoLeafFn<Entry> fn(entry, sums, adjust_next);

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
template <typename Entry>
void M_TYPE::insertEntry(Iterator& iter, const Entry& entry, bool adjust_next)
{
    auto& self      = this->self();
    NodeBaseG& leaf = iter.leaf();
    Int& idx        = iter.idx();

	Entry tmp_entry = entry;

	tmp_entry.key() -= std::get<0>(iter.prefixes())[1];


    Accumulator sums;

    std::pair<bool, bool> result = self.insertIntoLeaf(leaf, idx, tmp_entry, sums, adjust_next);

    if (!result.first)
    {
        iter.split();

        Clear(sums);

        result = self.insertIntoLeaf(leaf, idx, tmp_entry, sums, adjust_next);

        if (!result.first)
        {
            throw Exception(MA_SRC, "Second insertion attempt failed");
        }
    }

    self.addTotalKeyCount(Position::create(0, 1));

    if (adjust_next)
    {
    	if (result.second)
    	{
    		std::get<0>(sums)[1] = 0;

    		self.updateParent(leaf, sums);

    		iter++;
    	}
    	else {
    		self.updateParent(leaf, sums);

    		if (iter++)
    		{
    			iter.adjustIndex(1, -tmp_entry.key());
    		}
    	}
    }
    else {
    	self.updateParent(leaf, sums);
    	iter++;
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
