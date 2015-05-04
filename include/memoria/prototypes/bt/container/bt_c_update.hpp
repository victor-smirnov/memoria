
// Copyright Victor Smirnov 2011-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_UPDATE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_UPDATE_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::UpdateName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::PageUpdateMgr                                       PageUpdateMgr;


    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::DataTarget                                          DataTarget;


    using CtrSizeT = typename Types::CtrSizeT;

    template <Int Stream>
    using StreamInputTuple = typename Types::template StreamInputTuple<Stream>;




    Position getRemainder(ISource& source)
    {
        Position size;

        for (Int c = 0; c < source.streams(); c++)
        {
            IDataBase* data = T2T<IDataBase*>(source.stream(c));
            size[c] = data->getRemainder();
        }

        return size;
    }

    UBigInt getSourceActiveStreams(ISource& source)
    {
        UBigInt streams = 0;

        for (Int c = 0; c < Streams; c++)
        {
            IDataBase* data = T2T<IDataBase*>(source.stream(c));
            UBigInt active  = data->getRemainder() > 0;

            streams |= (active << c);
        }

        return streams;
    }

    MEMORIA_DECLARE_NODE_FN(UpdateFn, update);

    Position updateStreams(Iterator& iter, const Position& start, ISource& data_source)
    {
        auto& self = this->self();

        Position pos = start;

        Position sum;
        Position len = getRemainder(data_source);

        while (len.gtAny(0))
        {
            Position to_update = self.getNodeSizes(iter.leaf()) - pos;

            for (Int c = 0; c < Streams; c++)
            {
                if (to_update[c] > len[c])
                {
                    to_update[c] = len[c];
                }
            }

            LeafDispatcher::dispatch(
                    iter.leaf(),
                    UpdateFn(),
                    &data_source,
                    pos,
                    to_update
            );

            len     -= to_update;
            sum     += to_update;

            UBigInt active_streams = getSourceActiveStreams(data_source);

            if (len.gtAny(0))
            {
                iter.nextLeafMs(active_streams);
                pos.clear();

                if (iter.isEof())
                {
                    break;
                }
            }
            else {
                iter.key_idx() += to_update[iter.stream()];
                break;
            }
        }

        return sum;
    }



    CtrSizeT updateStream(Iterator& iter, ISource& data_source)
    {
        IDataBase* data = T2T<IDataBase*>(data_source.stream(iter.stream()));

        CtrSizeT sum = 0;
        CtrSizeT len = data->getRemainder();

        while (len > 0)
        {
            Int to_update = iter.size() - iter.dataPos();

            if (to_update > len) to_update = len;

            LeafDispatcher::dispatch(
                    iter.leaf(),
                    UpdateFn(),
                    &data_source,
                    Position(iter.dataPos()),
                    Position(to_update)
            );

            len     -= to_update;
            sum     += to_update;

            iter.skipFw(to_update);

            if (iter.isEof())
            {
                break;
            }
        }

        return sum;
    }



MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::ReadName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

} //memoria



#endif
