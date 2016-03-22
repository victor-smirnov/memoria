
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::map::CtrInsertMaxName)

public:
    using typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::PageUpdateMgr;

    using Key = typename Types::Key;
    using Value = typename Types::Value;

    static const Int Streams = Types::Streams;

    using Element   = ValuePair<BranchNodeEntry, Value>;
    using MapEntry  = typename Types::Entry;

    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

    using CtrSizeT = typename Types::CtrSizeT;

public:
    CtrSizeT size() const {
        return self().sizes()[0];
    }


    IteratorPtr find(const Key& k)
    {
        return self().template find_max_ge<IntList<0, 1>>(0, k);
    }


    bool remove(const Key& k)
    {
        auto iter = find(k);

        if (iter->is_found(k))
        {
            iter->remove();
            return true;
        }
        else {
            return false;
        }
    }

    IteratorPtr assign(const Key& key, const Value& value)
    {
        auto iter = self().find(key);

        if (iter->is_found(key))
        {
            iter->assign(value);
        }
        else {
            iter->insert(key, value);
        }

        return iter;
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::map::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}
