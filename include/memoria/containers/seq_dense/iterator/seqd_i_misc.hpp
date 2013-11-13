
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQD_ITERATOR_MISC_HPP
#define _MEMORIA_CONTAINERS_SEQD_ITERATOR_MISC_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/seq_dense/seqd_names.hpp>
#include <memoria/containers/seq_dense/seqd_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/core/packed/array/packed_fse_bitmap.hpp>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::seq_dense::IterMiscName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::Types::DataSource                               DataSource;
    typedef typename Container::Types::DataTarget                               DataTarget;
    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;


    Int size() const
    {
        return self().leafSize(0);
    }

    struct SymbolFn {
        Int symbol_ = 0;

        template <Int Idx, typename SeqTypes>
        void stream(const PkdFSSeq<SeqTypes>* obj, Int idx)
        {
            MEMORIA_ASSERT_TRUE(obj != nullptr);
            symbol_ = obj->symbol(idx);
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSEArray<StreamTypes>* obj, Int idx)
        {
            MEMORIA_ASSERT_TRUE(obj != nullptr);
            symbol_ = obj->value(idx);
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PkdFTree<StreamTypes>* obj, Int idx)
        {
            MEMORIA_ASSERT_TRUE(obj != nullptr);
            symbol_ = obj->value(0, idx);
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PkdVTree<StreamTypes>* obj, Int idx)
        {
            MEMORIA_ASSERT_TRUE(obj != nullptr);
            symbol_ = obj->value(0, idx);
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSEBitmap<StreamTypes>* obj, Int idx)
        {
            MEMORIA_ASSERT_TRUE(obj != nullptr);
            symbol_ = obj->value(idx);
        }


        template <typename NodeTypes>
        void treeNode(const LeafNode<NodeTypes>* node, Int idx)
        {
            node->process(0, *this, idx);
        }
    };

    struct SetSymbolFn {
    	Int symbol_ = 0;
    	Accumulator accum_;


    	SetSymbolFn(Int symbol): symbol_(symbol) {}

    	template <Int Idx, typename SeqTypes>
    	void stream(PkdFSSeq<SeqTypes>* obj, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(obj != nullptr);

    		Int old_sym = obj->symbol(idx);

    		std::get<Idx>(accum_)[old_sym + 1] = -1;

    		obj->symbol(idx) = symbol_;

    		std::get<Idx>(accum_)[symbol_ + 1] = 1;

    		obj->reindex();
    	}

    	template <typename NodeTypes>
    	void treeNode(LeafNode<NodeTypes>* node, Int idx)
    	{
    		node->process(0, *this, idx);
    	}
    };




    Int symbol() const
    {
        auto& self  = this->self();

        SymbolFn fn;

        Int idx = self.idx();

        LeafDispatcher::dispatchConst(self.leaf(), fn, idx);

        return fn.symbol_;
    }

    void setSymbol(Int symbol)
    {
    	auto& self  = this->self();

    	SetSymbolFn fn(symbol);

    	Int idx = self.idx();

    	self.ctr().updatePageG(self.leaf());

    	LeafDispatcher::dispatch(self.leaf(), fn, idx);

    	self.ctr().updateParent(self.leaf(), fn.accum_);
    }

    BigInt label(Int label_idx) const
    {
        auto& self  = this->self();

        SymbolFn fn;

        Int idx = self.label_idx();

        LeafDispatcher::dispatchConst(self.leaf(), fn, idx);

        return fn.symbol_;
    }

    void insert(Int symbol)
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();

        ctr.insert(self, symbol);
    }

    void remove()
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();

        ctr.remove(self);
    }

    void remove(BigInt size)
    {
        auto& self  = this->self();
        auto& ctr   = self.ctr();

        ctr.removeBlock(self, size);
    }

    Int dataPos() const {
    	return self().idx();
    }

    BigInt read(DataTarget& data)
    {
    	auto& self = this->self();
    	seq_dense::SequenceTarget target(&data);

    	return self.ctr().readStream(self, target);
    }

    void insert(DataSource& data)
    {
    	auto& self = this->self();
    	self.ctr().insertBlock(self, data);
    }

    BigInt update(DataSource& data)
    {
    	auto& self = this->self();
    	return self.ctr().updateBlock(self, data);
    }

    void ComputePrefix(BigInt& accum)
    {

    }

    void ComputePrefix(Accumulator& accum)
    {

    }

    Accumulator prefixes() const {
    	return Accumulator();
    }

    void createEmptyLeaf()
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();

    	NodeBaseG next = ctr.createNextLeaf(self.leaf());

    	self.leaf() = next;
    	self.idx()  = 0;
    }

    Int leaf_capacity()
    {
    	auto& self  = this->self();
    	auto& ctr   = self.ctr();

    	return ctr.getStreamCapacity(self.leaf(), Position::create(0, 0), 0);
    }

MEMORIA_ITERATOR_PART_END


#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::seq_dense::IterMiscName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS


}



#endif
