
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_METAMAP_CTR_FIND_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_CTR_FIND_HPP


#include <memoria/prototypes/metamap/metamap_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::metamap::CtrFindName)

    typedef typename Base::Types                                                Types;


    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef ValuePair<Accumulator, Value>                                       Element;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::Entry                                               MapEntry;

    static const Int Streams                                                    = Types::Streams;

    CtrSizeT size() const {
        return self().sizes()[0];
    }


    Iterator seek(CtrSizeT entry_idx)
    {
        typename Types::template SkipForwardWalker<Types, IntList<0>> walker(0, 0, entry_idx);
        return self().find0(0, walker);
    }


    Iterator find(Key key)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (!iter.isEnd())
        {
            if (key == iter.key())
            {
                return iter;
            }
            else {
                return self().End();
            }
        }
        else {
            return iter;
        }
    }

    Iterator findKeyGE(Key key)
    {
        return self().findGE(0, key, 1);
    }

    Iterator findKeyLE(Key key)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (iter.isEnd() || iter.key() > key)
        {
            iter--;

            if (iter.isBegin())
            {
                iter.idx() = 0;
            }
        }

        return iter;
    }

    Iterator findKeyLT(Key key)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (iter.isEnd() || iter.key() >= key)
        {
            iter--;

            if (iter.isBegin())
            {
                iter.idx() = 0;
            }
        }

        return iter;
    }



    Iterator operator[](Key key)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (iter.isEnd() || key != iter.key())
        {
            MapEntry entry;
            entry.key() = key;

            self().insertEntry(iter, entry);

            iter--;
        }

        return iter;
    }

    Iterator selectLabel(Int label_num, Int label, CtrSizeT rank)
    {
        Int label_index = p_label_block_offset(label_num) + label;

        typename Types::template SelectForwardWalker<Types> walker(0, label_num, label_index, label, false, rank);

        return self().find0(0, walker);
    }

    Iterator selectHiddenLabel(Int label_num, Int label, CtrSizeT rank)
    {
        Int label_index = p_hidden_label_block_offset(label_num) + label;

        typename Types::template SelectForwardWalker<Types> walker(0, label_num, label_index, label, true, rank);

        return self().find0(0, walker);
    }

private:

    Int p_hidden_label_block_offset(Int label_num) const
    {
        return 1 + Types::Indexes + Types::HiddenLabelsOffset::offset(label_num);
    }

    Int p_label_block_offset(Int label_num) const
    {
        return p_hidden_label_block_offset(Types::HiddenLabels) + Types::LabelsOffset::offset(label_num);
    }

MEMORIA_CONTAINER_PART_END

}


#endif
