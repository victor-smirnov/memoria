
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_KEYS_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_KEYS_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrKeysName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;


    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;
    typedef typename Container::Position                                        Position;

    typedef typename Container::Types::IteratorPrefix                           IteratorPrefix;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;




    struct GetIndexFn: bt1::RtnNodeWalkerBase<GetIndexFn, Key>
    {
        template <Int Idx, typename Stream>
        Key stream(const Stream* stream, Int idx, Int index_num)
        {
        	MEMORIA_ASSERT_TRUE(stream);
        	return metamap::GetLeafIndex<Key>(stream, idx, index_num);
        }
    };

    Key index(Int index_num) const
    {
    	auto& self = this->self();

    	GetIndexFn fn;

    	return LeafDispatcher::dispatchConstRtn(self.leaf(), fn, self.idx(), index_num);
    }

    Key key() const
    {
        return self().prefix() + self().index(0);
    }


    std::pair<Key, Value> operator*() const
    {
        return std::pair<Key, Value>(self().key(), self().value());
    }


    BigInt prefix() const
    {
        return std::get<0>(self().cache().prefixes())[1];
    }

    IteratorPrefix prefixes() const
    {
    	return self().cache().prefixes();
    }

    void updateKey(BigInt key)
    {
    	auto& self = this->self();

    	Accumulator sums;

    	std::get<0>(sums)[0] = key;

    	self.updateUp(sums);

    	if (self++)
    	{
    		self.updateUp(-sums);
    	}

    	self--;
    }

MEMORIA_ITERATOR_PART_END

}

#endif