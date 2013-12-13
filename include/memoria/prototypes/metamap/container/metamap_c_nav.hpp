
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_METAMAP_CTR_NAVIGATE_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_CTR_NAVIGATE_HPP


#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::metamap::CtrNavName)

    typedef typename Base::Types                                                Types;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;

    typedef ValuePair<Accumulator, Value>                                       Element;

    static const Int Streams                                                    = Types::Streams;

    MEMORIA_PUBLIC Iterator Begin()
    {
        typename Types::template FindBeginWalker<Types> walker(0, self());
        return self().find0(0, walker);
    }


    Iterator End()
    {
        auto& self = this->self();

        auto stream = 0;

        typename Types::template SkipForwardWalker<Types> walker(stream, 0, self.size());
        return self.find0(stream, walker);
    }

    Iterator RBegin()
    {
        auto& self = this->self();

        auto stream = 0;

        auto size = self.size();

        if (size > 0)
        {
            typename Types::template SkipForwardWalker<Types> walker(stream, 0, size - 1);
            return self.find0(stream, walker);
        }
        else {
            return self.End();
        }
    }

    Iterator REnd()
    {
        auto iter = self().Begin();

        iter.idx() = -1;

        return iter;
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::metamap::CtrNavName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}


#endif
