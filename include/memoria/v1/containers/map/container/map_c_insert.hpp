
// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#pragma once


#include <memoria/v1/containers/map/map_names.hpp>
#include <memoria/v1/containers/map/map_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_CONTAINER_PART_BEGIN(v1::map::CtrInsertName)

    using typename Base::Types;

    using typename Base::NodeBaseG;
    using typename Base::IteratorPtr;

    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::PageUpdateMgr;

    using Key = typename Types::Key;
    using Value = typename Types::Value;


    template <typename LeafPath>
    using TargetType = typename Types::template TargetType<LeafPath>;

//    using CtrSizeT = typename Types::CtrSizeT;

//    CtrSizeT size() const {
//      return self().sizes()[0];
//    }

    template <typename T>
    IteratorPtr find(T&& k)
    {
        return self().template find_ge<IntList<0, 0, 1>>(0, k);
    }

    template <typename K, typename V>
    IteratorPtr assign(K&& key, V&& value)
    {
        auto iter = self().find(key);

        if (iter->is_found(key))
        {
            iter->assign(value);
        }
        else {
            iter->insert_(key, value);
        }

        return iter;
    }

    template <typename T>
    bool remove(T&& k)
    {
        auto iter = find(k);

        if (iter->key() == k)
        {
            iter->remove();
            return true;
        }
        else {
            return false;
        }
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(v1::map::CtrRemoveName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS

#undef M_PARAMS
#undef M_TYPE

}}