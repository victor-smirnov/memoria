
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_VALUE_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_VALUE_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrValueName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;

    struct SetValueFn {
        template <Int Idx, typename StreamTypes>
        void stream(PackedVLEMap<StreamTypes>* map, Int idx, const Value& value)
        {
            MEMORIA_ASSERT_TRUE(map);
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
    };

    class ConstValueAccessor {
        const MyType& iter_;
    public:
        ConstValueAccessor(const MyType& iter): iter_(iter) {}

        operator Value() const {
            return iter_.getValue();
        }
    };


    struct GetValueFn {
        Value value_ = 0;

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSEMap<StreamTypes>* map, Int idx)
        {
            if (map != nullptr)
            {
                value_ = map->value(idx);
            }
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PackedVLEMap<StreamTypes>* map, Int idx)
        {
            if (map != nullptr)
            {
                value_ = map->value(idx);
            }
        }

        template <Int Idx, typename StreamTypes>
        void stream(const PackedFSESearchableMarkableMap<StreamTypes>* map, Int idx)
        {
        	if (map != nullptr)
        	{
        		value_ = map->value(idx);
        	}
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
MEMORIA_ITERATOR_PART_END

}

#endif
