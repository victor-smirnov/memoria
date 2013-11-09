
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SMRKMAP_ITER_API_HPP
#define _MEMORIA_CONTAINERS_SMRKMAP_ITER_API_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/containers/smrk_map/smrkmap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_smark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::smrk_map::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;
    typedef typename Base::TreePath                                             TreePath;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;


    void updateUp(const Accumulator& keys)
    {
        auto& self = this->self();

        self.ctr().updateUp(self.leaf(), self.idx(), keys, [&](Int, Int idx) {
            self.idx() = idx;
            self.updatePrefix();
        });
    }


    struct KeyFn {
        Key value_ = 0;

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx)
        {
        	if (map != nullptr)
        	{
        		value_ = map->tree()->value(0, idx);
        	}
        }

        template <typename Node>
        void treeNode(const Node* node, Int idx)
        {
            node->template processStream<0>(*this, idx);
        }
    };


    Key raw_key() const
    {
        auto& self = this->self();

        KeyFn fn;

        LeafDispatcher::dispatchConst(self.leaf(), fn, self.idx());

        return fn.value_;
    }

    Key key() const
    {
        return self().prefix() + raw_key();
    }


    std::pair<Key, Value> operator*() const
    {
        return std::pair<Key, Value>(key(), value());
    }


    bool next() {
        return self().nextKey();
    }

    bool prev() {
        return self().prevKey();
    }

    BigInt prefix() const
    {
        return self().cache().prefix();
    }

    Accumulator prefixes() const {
        Accumulator acc;

        std::get<0>(acc)[1] = self().prefix();

        return acc;
    }

    Int entry_idx() const
    {
        return self().idx();
    }


    struct SetValueFn {
    	template <Int Idx, typename StreamTypes>
        void stream(PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx, const Value& value)
        {
        	MEMORIA_ASSERT_TRUE(map != nullptr);
        	map->value(idx) = value;
        }

        template <typename Node>
        void treeNode(Node* node, Int idx, const Value& value)
        {
            node->template processStream<0>(*this, idx, value);
        }
    };


    void setValue(const Value& value)
    {
        auto& self = this->self();
        self.ctr().updatePageG(self.leaf());
        LeafDispatcher::dispatch(self.leaf(), SetValueFn(), self.idx(), value);

        self.ctr().markCtrUpdated();
    }

    class ValueAccessor {
        MyType& iter_;
    public:
        ValueAccessor(MyType& iter): iter_(iter) {}

        operator Value() const {
            return iter_.getValue();
        }

        Value operator=(const Value& value) {
            iter_.setValue(value);
            return value;
        }

        Value value() const {
        	return iter_.getValue();
        }
    };

    class ConstValueAccessor {
        const MyType& iter_;
    public:
        ConstValueAccessor(const MyType& iter): iter_(iter) {}

        operator Value() const {
            return iter_.getValue();
        }

        Value value() const {
        	return iter_.getValue();
        }
    };


    struct GetValueFn {
        Value value_;

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx)
        {
        	MEMORIA_ASSERT_TRUE(map)

        	value_ = map->value(idx);
        }

        template <typename Node>
        void treeNode(const Node* node, Int idx)
        {
            node->template processStream<0>(*this, idx);
        }
    };




    Value getValue() const
    {
        auto& self = this->self();

        GetValueFn fn;

        LeafDispatcher::dispatchConst(self.leaf(), fn, self.idx());

        return fn.value_;
    }

    ValueAccessor value() {
        return ValueAccessor(self());
    }

    ConstValueAccessor value() const {
        return ConstValueAccessor(self());
    }


    void setData(const Value& value)
    {
        self().value() = value;
    }


    struct SetMarkFn {

    	Int old_mark_;

    	template <Int Idx, typename StreamTypes>
    	void stream(PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx, Int mark)
    	{
    		MEMORIA_ASSERT_TRUE(map != nullptr);

    		old_mark_ = map->mark(idx);

    		map->mark(idx) = mark;

    		map->bitmap()->reindex();
    	}


    	template <typename NTypes>
    	void treeNode(LeafNode<NTypes>* node, Int idx, Int mark)
    	{
    		node->layout(1);
    		node->template processStream<0>(*this, idx, mark);
    	}
    };


    void setMark(Int mark)
    {
    	auto& self = this->self();

    	self.ctr().updatePageG(self.leaf());

    	SetMarkFn fn;

    	LeafDispatcher::dispatch(self.leaf(), fn, self.idx(), mark);

    	Accumulator sums;

    	std::get<0>(sums)[2 + fn.old_mark_] = -1;
    	std::get<0>(sums)[2 + mark] = 1;

    	self.ctr().updateParent(self.leaf(), sums);

    	self.ctr().markCtrUpdated();
    }

    struct AssingFn {
    	template <Int Idx, typename StreamTypes>
    	void stream(PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx, Int mark, const Value& value)
    	{
    		MEMORIA_ASSERT_TRUE(map != nullptr);

    		map->mark(idx)  = mark;
    		map->value(idx) = value;

    		map->bitmap()->reindex();
    	}


    	template <typename NTypes>
    	void treeNode(LeafNode<NTypes>* node, Int idx, Int mark, const Value& value)
    	{
    		node->layout(1);
    		node->template processStream<0>(*this, idx, mark, value);
    	}
    };


    void assing(Int mark, const Value& value)
    {
    	auto& self = this->self();

    	self.ctr().updatePageG(self.leaf());

    	LeafDispatcher::dispatch(self.leaf(), AssingFn(), self.idx(), mark, value);

    	Accumulator sums;

    	std::get<0>(sums)[2 + mark] = 1;

    	self.ctr().updateParent(self.leaf(), sums);

    	self.ctr().markCtrUpdated();
    }



    struct GetMarkFn {
    	Int mark_;

    	template <Int Idx, typename StreamTypes>
    	void stream(const PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(map != nullptr);

    		this->mark_ =  map->mark(idx);
    	}


    	template <typename NTypes>
    	void treeNode(const LeafNode<NTypes>* node, Int idx)
    	{
    		node->template processStream<0>(*this, idx);
    	}
    };

    Int mark() const
    {
    	GetMarkFn fn;

    	LeafDispatcher::dispatchConst(self().leaf(), fn, self().idx());

    	return fn.mark_;
    }





    void remove()
    {
        auto& self = this->self();

        Accumulator keys;
        self.ctr().removeMapEntry(self, keys);

        if (!self.isEnd())
        {
            std::get<0>(keys)[0] = 0;

            for (Int c = 0; c < 1<<Types::BitsPerMark; c++)
            {
            	std::get<0>(keys)[c + 2] = 0;
            }

        	self.updateUp(keys);
        }
    }

    void insert(const Key& key, const Value& value, Int mark)
    {
    	auto& self = this->self();

    	Accumulator sums;
    	std::get<0>(sums)[1] = key;

    	self.ctr().insert(self, Element(sums, value), mark);
    }



    void ComputePrefix(BigInt& accum)
    {
        auto& self = this->self();

        PrefixFn fn;

        if (self.idx() >= 0)
        {
            self.ctr().walkUp(self.leaf(), self.idx(), fn);
        }

        accum = fn.prefix_;
    }

    void ComputePrefix(Accumulator& accum)
    {
        ComputePrefix(std::get<0>(accum)[1]);
    }

    void dump(std::ostream& out = std::cout)
    {
        out<<"Prefix="<<self().cache().prefix()<<endl;
        Base::dump(out);
    }

    struct PrefixFn {
        BigInt prefix_ = 0;

        PrefixFn() {}

//        template <Int Idx, typename StreamTypes>
//        void stream(const PackedFSEMap<StreamTypes>* map, Int idx)
//        {
//            if (map != nullptr)
//            {
//                prefix_ += map->tree()->sum(0, idx);
//            }
//        }
//
//        template <Int Idx, typename StreamTypes>
//        void stream(const PackedVLEMap<StreamTypes>* map, Int idx)
//        {
//            if (map != nullptr)
//            {
//                prefix_ += map->tree()->sum(0, idx);
//            }
//        }
//
        template <Int Idx, typename StreamTypes>
        void stream(const PkdFTree<StreamTypes>* tree, Int idx)
        {
            if (tree != nullptr)
            {
                prefix_ += tree->sum(1, idx);
            }
        }
//
//        template <Int Idx, typename StreamTypes>
//        void stream(const PkdVTree<StreamTypes>* tree, Int idx)
//        {
//            if (tree != nullptr)
//            {
//                prefix_ += tree->sum(1, idx);
//            }
//        }

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx)
        {
        	if (map != nullptr)
        	{
        		prefix_ += map->tree()->sum(0, idx);
        	}
        }

        template <typename Node>
        void treeNode(const Node* node, Int idx)
        {
            node->template processStream<0>(*this, idx);
        }
    };

    void updatePrefix()
    {
        auto& self = this->self();

        PrefixFn fn;

        if (self.idx() >= 0)
        {
            self.ctr().walkUp(self.leaf(), self.idx(), fn);
        }

        self.cache().setup(fn.prefix_);
    }

    void split()
    {
        auto& self = this->self();

        NodeBaseG& leaf = self.leaf();
        Int& idx        = self.idx();

        Int size        = self.leaf_size(0);
        Int split_idx   = size/2;

        auto right = self.ctr().splitLeafP(leaf, Position::create(0, split_idx));

        if (idx > split_idx)
        {
        	leaf = right;
        	idx -= split_idx;

        	self.updatePrefix();
        }
    }

    bool is_found_eq(Key key) const
    {
    	auto& self = this->self();
    	return (!self.isEnd()) && self.key() == key;
    }

    bool is_found_le(Key key) const
    {
    	auto& self = this->self();

    	return self.isContent() && self.key() <= key;
    }

    bool is_found_lt(Key key) const
    {
    	auto& self = this->self();
    	return self.isContent() && self.key() <= key;
    }

    bool is_found_ge(Key key) const
    {
    	auto& self = this->self();
    	return self.isContent() && self.key() >= key;
    }

    bool is_found_gt(Key key) const
    {
    	auto& self = this->self();
    	return self.isContent() && self.key() > key;
    }



MEMORIA_ITERATOR_PART_END

}

#endif
