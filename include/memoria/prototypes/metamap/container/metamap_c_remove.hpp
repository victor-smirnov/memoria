
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_MAP_CTR_REMOVE_HPP
#define _MEMORIA_PROTOTYPES_MAP_CTR_REMOVE_HPP


#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_map.hpp>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::metamap::CtrRemoveName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;


    typedef typename Types::Key                                                 Key;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::LeafDispatcher                                       LeafDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    struct RemoveFromLeafFn: bt1::NodeWalkerBase<RemoveFromLeafFn> {
        Accumulator& entry_;

        bool next_entry_updated_ = false;

        RemoveFromLeafFn(Accumulator& sums):entry_(sums) {}

        template <Int Idx, typename Stream>
        void stream(Stream* map, Int idx, bool adjust_next)
        {
            MEMORIA_ASSERT_TRUE(map);

            map->sums(idx, std::get<Idx>(entry_));

            map->remove(idx, idx + 1);

            if (adjust_next)
            {
                next_entry_updated_ = idx < map->size();

                if (next_entry_updated_)
                {
                    map->addValue(0, idx, std::get<0>(entry_)[1]);
                }
            }
        }
    };

    void removeMapEntry(Iterator& iter, Accumulator& sums, bool adjust_next = true)
    {
        auto& self  = this->self();
        auto& leaf  = iter.leaf();
        Int& idx    = iter.idx();

        RemoveFromLeafFn fn(sums);

        self.updatePageG(leaf);

        LeafDispatcher::dispatch(leaf, fn, idx, adjust_next);

        if (fn.next_entry_updated_)
        {
            auto sums1 = fn.entry_;
            std::get<0>(sums1)[1] = 0;

            self.updateParent(leaf, -sums1);
        }
        else {
            self.updateParent(leaf, -fn.entry_);
        }

        self.addTotalKeyCount(Position::create(0, -1));

        self.mergeWithSiblings(leaf, [&](const Position& prev_sizes, Int level) {
            if (level == 0)
            {
                idx += prev_sizes[0];
            }
        });

        if (iter.isEnd())
        {
            iter.nextLeaf();
        }

        self.removeRedundantRootP(leaf);

        if (adjust_next && (!fn.next_entry_updated_) && !iter.isEnd())
        {
            iter.adjustIndex(1, (std::get<0>(fn.entry_)[1]));
        }

        self.markCtrUpdated();
    }




    bool removeMapEntries(Iterator& from, Iterator& to, Accumulator& keys, bool adjust_next_entry);

    bool remove(Key key, bool adjust_next_entry = true)
    {
        Iterator iter = self().findGE(0, key, 1);

        if (key == iter.key())
        {
            iter.remove(adjust_next_entry);
            return true;
        }
        else {
            return false;
        }
    }


MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::metamap::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

M_PARAMS
bool M_TYPE::removeMapEntries(Iterator& from, Iterator& to, Accumulator& keys, bool adjust_next_entry)
{
    auto& ctr = self();

    auto& from_node     = from.leaf();
    Position from_pos   = Position(from.idx());

    auto& to_node       = to.leaf();
    Position to_pos     = Position(to.idx());

    bool result = ctr.removeEntries(from_node, from_pos, to_node, to_pos, keys, true).gtAny(0);

    to.idx() = to_pos.get();

    if (adjust_next_entry && !to.isEnd())
    {
        to.adjustIndex(1, (std::get<0>(keys)[1]));
    }

    to.updatePrefix();

    from = to;

    ctr.markCtrUpdated();

    return result;
}


}

#undef M_TYPE
#undef M_PARAMS


#endif
