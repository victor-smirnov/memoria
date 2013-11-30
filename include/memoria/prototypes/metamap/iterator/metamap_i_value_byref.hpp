
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_VALUE_BYREF_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_VALUE_BYREF_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrValueByRefName)

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

    template <typename Value>
    struct GetValueRefFn {
    	using ReturnType = Value&;
    	using ResultType = Value&;

    	template <Int Idx, typename Stream>
    	ReturnType stream(Stream* stream, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(stream);
    		return metamap::GetValueRef<Value>(stream, idx);
    	}

    	template <typename Node>
    	ResultType treeNode(Node* node, Int idx)
    	{
    		return node->template processStreamRtn<0>(*this, idx);
    	}
    };



    const Value& getValue() const
    {
    	auto& self = this->self();
    	return LeafDispatcher::dispatchConstRtn(self.leaf(), GetValueRefFn<const Value>(), self.idx());
    }

    Value& getSValue()
    {
    	auto& self = this->self();
    	self.ctr().updatePageG(self.leaf());
    	return LeafDispatcher::dispatchConstRtn(self.leaf(), GetValueRefFn<Value>(), self.idx());
    }


    struct SetValueFn {
        template <Int Idx, typename Stream>
        void stream(Stream* stream, Int idx, const Value& value)
        {
            MEMORIA_ASSERT_TRUE(stream);
            metamap::SetValue(stream, idx, value);
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

    Value& svalue()
    {
    	return self().getSValue();
    }

    const Value& value() const
    {
    	return self().getValue();
    }
MEMORIA_ITERATOR_PART_END

}

#endif
