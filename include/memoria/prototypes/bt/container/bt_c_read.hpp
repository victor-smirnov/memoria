
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_READ_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_CTR_READ_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

using namespace memoria::bt;
using namespace memoria::core;

using namespace std;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::ReadName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;
    typedef typename Types::DataTarget                                          DataTarget;
    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::Target                                            	Target;

    static const Int Streams                                                    = Types::Streams;

    Position getRemainder(ITarget& target)
    {
        Position size;

        for (Int c = 0; c < target.streams(); c++)
        {
            IDataBase* data = T2T<IDataBase*>(target.stream(c));
            size[c] = data->getRemainder();
        }

        return size;
    }

    UBigInt getTargetActiveStreams(ITarget& target)
    {
        UBigInt streams = 0;

        for (Int c = 0; c < Streams; c++)
        {
            IDataBase* data = T2T<IDataBase*>(target.stream(c));
            UBigInt active  = data->getRemainder() > 0;

            streams |= (active << c);
        }

        return streams;
    }

    UBigInt getTargetActiveStreams(Target& target)
    {
    	return self().getRemainderSize(target).gtZero();
    }

    MEMORIA_DECLARE_NODE_FN(ReadFn, read);

    Position readStreams(Iterator& iter, const Position& start, ITarget& data_target)
    {
        auto& self = this->self();

        Position pos = start;

        Position sum;
        Position len = getRemainder(data_target);

        while (len.gtAny(0))
        {
            Position to_read = self.getNodeSizes(iter.leaf()) - pos;

            for (Int c = 0; c < Streams; c++)
            {
                if (to_read[c] > len[c])
                {
                    to_read[c] = len[c];
                }
            }

            LeafDispatcher::dispatchConst(
                    iter.leaf(),
                    ReadFn(),
                    &data_target,
                    pos,
                    to_read
            );

            len     -= to_read;
            sum     += to_read;

            UBigInt active_streams = getTargetActiveStreams(data_target);

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
                iter.key_idx() += to_read[iter.stream()];
                break;
            }
        }

        return sum;
    }



    Position readStreams(Iterator& iter, const Position& start, Target& data_target)
    {
        auto& self = this->self();

        Position pos = start;

        Position sum;
        Position len = self.getRemainderSize(data_target);

        while (len.gtAny(0))
        {
            Position to_read = self.getNodeSizes(iter.leaf()) - pos;

            for (Int c = 0; c < Streams; c++)
            {
                if (to_read[c] > len[c])
                {
                    to_read[c] = len[c];
                }
            }

            LeafDispatcher::dispatchConst(
                    iter.leaf(),
                    ReadFn(),
                    data_target,
                    pos,
                    to_read
            );

            len     -= to_read;
            sum     += to_read;

            UBigInt active_streams = getTargetActiveStreams(data_target);

            if (len.gtAny(0))
            {
                iter.nextLeafMs(active_streams);
                pos.clear();

                if (iter.isEnd())
                {
                    break;
                }
            }
            else {
                iter.idx() += to_read[iter.stream()];
                break;
            }
        }

        return sum;
    }



    CtrSizeT readStream(Iterator& iter, ITarget& data_target)
    {
        MEMORIA_ASSERT(iter.dataPos(), >=, 0);

        IDataBase* data = T2T<IDataBase*>(data_target.stream(iter.stream()));

        CtrSizeT sum = 0;
        CtrSizeT len = data->getRemainder();

        while (len > 0)
        {
            Int to_read = iter.size() - iter.dataPos();

            if (to_read > len) to_read = len;

            LeafDispatcher::dispatchConst(
                    iter.leaf(),
                    ReadFn(),
                    &data_target,
                    Position(iter.dataPos()),
                    Position(to_read)
            );

            len     -= to_read;
            sum     += to_read;

            iter.skipFw(to_read);

            if (iter.isEof())
            {
                break;
            }
        }

        return sum;
    }


    CtrSizeT readStream(Iterator& iter, Target& data_target)
    {
    	MEMORIA_ASSERT(iter.idx(), >=, 0);

    	CtrSizeT sum = 0;
    	CtrSizeT len = self().getRemainderSize(data_target)[iter.stream()];

    	while (len > 0)
    	{
    		Int to_read = iter.leaf_size(iter.stream()) - iter.idx();

    		if (to_read > len) to_read = len;

    		LeafDispatcher::dispatchConst(
    				iter.leaf(),
    				ReadFn(),
    				data_target,
    				Position(iter.idx()),
    				Position(to_read)
    		);

    		len     -= to_read;
    		sum     += to_read;

    		iter.skipFw(to_read);

    		if (iter.isEnd())
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
