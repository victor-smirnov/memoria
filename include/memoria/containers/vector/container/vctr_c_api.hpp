
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_vctr_C_API_HPP
#define _MEMORIA_CONTAINER_vctr_C_API_HPP


#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::mvector::CtrApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher 	= typename Types::Pages::NodeDispatcher;
    using LeafDispatcher 	= typename Types::Pages::LeafDispatcher;
    using BranchDispatcher 	= typename Types::Pages::BranchDispatcher;

    typedef typename Types::Value                                               Value;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    static const Int Streams                                                    = Types::Streams;



    CtrSizeT size() const {
        return self().sizes()[0];
    }

    Iterator seek(CtrSizeT pos)
    {
    	typename Types::template SkipForwardWalker<Types, IntList<0>> walker(pos);

    	return self().find2(walker);
    }


    MyType& operator<<(vector<Value>& v)
    {
        auto& self = this->self();
        auto i = self.seek(self.size());
        i.insert(v);
        return self;
    }

MEMORIA_CONTAINER_PART_END

}


#endif
