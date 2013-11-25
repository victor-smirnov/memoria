
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_DBLMAP2_INNER_I_API_HPP
#define _MEMORIA_CONTAINER_DBLMAP2_INNER_I_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {




MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::dblmap::InnerItrApiName)

    typedef Ctr<typename Types::CtrTypes>                       				Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;


    template <typename T>
    using Find2ndGEWalker = typename Container::Types::template FindGEWalker<T>;

    template <typename T>
    using Find2ndGTWalker = typename Container::Types::template FindGTWalker<T>;


    struct ReadValueFn {
        typedef Value ReturnType;
        typedef Value ResultType;

        template <typename Node>
        ReturnType treeNode(const Node* node, Int offset)
        {
            return node->template processStreamRtn<0>(*this, offset);
        }

        template <Int StreamIdx, typename StreamType>
        ResultType stream(const StreamType* obj, Int offset)
        {
            MEMORIA_ASSERT(offset, <, obj->size());
            MEMORIA_ASSERT(offset, >=, 0);

        	return obj->value(offset);
        }
    };

    Value value()
    {
        auto& self = this->self();
        return LeafDispatcher::dispatchConstRtn(self.leaf(), ReadValueFn(), self.idx());
    }

    struct SetValueFn {
        template <typename Node>
        void treeNode(Node* node, Int offset, const Value& value)
        {
            node->template processStream<0>(*this, offset, value);
        }

        template <Int StreamIdx, typename StreamType>
        void stream(StreamType* obj, Int offset, const Value& value)
        {
        	MEMORIA_ASSERT(offset, <, obj->size());
        	MEMORIA_ASSERT(offset, >=, 0);

        	obj->value(offset) = value;
        }
    };

    void setValue(const Value& value)
    {
        auto& self = this->self();
        self.ctr().updatePageG(self.leaf());
        self.ctr().markCtrUpdated();

        LeafDispatcher::dispatch(self.leaf(), SetValueFn(), self.idx(), value);
    }


    struct Key2Fn {
    	typedef Key ReturnType;
    	typedef Key ResultType;

    	template <typename Node>
    	ReturnType treeNode(const Node* node, Int offset)
    	{
    		return node->template processStreamRtn<0>(*this, offset);
    	}

    	template <Int StreamIdx, typename StreamType>
    	ResultType stream(const StreamType* obj, Int offset)
    	{
    		MEMORIA_ASSERT(offset, <, obj->size());
    		MEMORIA_ASSERT(offset, >=, 0);

    		return obj->tree()->value(0, offset);
    	}
    };

    BigInt pos() const
    {
    	return std::get<0>(self().cache().prefixes())[0];
    }

    BigInt key_prefix() const
    {
    	return std::get<0>(self().cache().prefixes())[1];
    }

    Key key_entry() const
    {
    	auto& self = this->self();
    	return LeafDispatcher::dispatchConstRtn(self.leaf(), Key2Fn(), self.idx());
    }

    Key key() const {
    	return self().key_prefix() + self().key_entry();
    }

    void resetKeyPrefix()
    {
    	std::get<0>(self().cache().prefixes())[1] = 0;
    }


    bool findKeyGE(Key key)
    {
    	auto& self = this->self();

    	self.template _findFw<Find2ndGEWalker>(1, key);

    	return !self.isEnd();
    }

    bool findKeyGT(Key key)
    {
    	auto& self = this->self();

    	self.template _findFw<Find2ndGTWalker>(1, key);

    	return !self.isEnd();
    }


//    struct AddKey2Fn {
//
//    	template <typename Node>
//    	void treeNode(Node* node, Int offset, Key key)
//    	{
//    		node->template processStream<1>(*this, offset, key);
//    	}
//
//    	template <Int StreamIdx, typename StreamType>
//    	void stream(StreamType* obj, Inc offset, Key key)
//    	{
//    		MEMORIA_ASSERT(offset, <, obj->size());
//
//    		obj->tree()->value(0, offset) += key;
//    		obj->reindex();
//    	}
//    };



    BigInt removeRange(BigInt size)
    {
        auto& self 	= this->self();
        auto  to 	= this->self();

        to.skipFw(size);

        Accumulator sums;

        self.ctr().removeMapEntries(self, to, sums);

        BigInt removed = std::get<0>(sums)[0];

        self.addTotalKeyCount(Position::create(0, -removed));

        return std::get<0>(sums)[1];
    }

    struct UpdateFn {
        template <typename Node>
        void treeNode(Node* node, Int offset, const Accumulator& keys)
        {
            node->template processStream<0>(*this, offset, keys);
        }

        template <Int StreamIdx, typename StreamType>
        void stream(StreamType* obj, Int offset, const Accumulator& keys)
        {
            obj->tree()->addValue(0, offset, std::get<0>(keys)[1]);
            obj->reindex();
        }
    };

    void updateKey(BigInt delta)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 0);

        NodeBaseG& leaf = self.leaf();

        self.ctr().updatePageG(leaf);

        Accumulator sums;

        std::get<0>(sums)[1] = delta;

        LeafDispatcher::dispatch(leaf, UpdateFn(), self.idx(), sums);

        self.ctr().updateParent(leaf, sums);
    }

    void dump() const
    {
        auto cache = self().cache();

        cout<<"Cache: prefixes=" <<cache.prefixes()<<endl;

        Base::dump();
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
    	}
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::dblmap::InnerItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS

#endif
