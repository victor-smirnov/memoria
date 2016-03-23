
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <memoria/v1/containers/table/table_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/containers/table/table_tools.hpp>


#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::table::CtrApiName)

    using Types             = typename Base::Types;

    using NodeBaseG         = typename Types::NodeBaseG;
    using Iterator          = typename Base::Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    using Key                   = typename Types::Key;
    using Value                 = typename Types::Value;
    using CtrSizeT            = typename Types::CtrSizeT;

    using BranchNodeEntry       = typename Types::BranchNodeEntry;
    using Position            = typename Types::Position;

    static const Int Streams = Types::Streams;

    using PageUpdateMgt     = typename Types::PageUpdateMgr;

    Iterator Begin() {
        return self().template _seek<0>(0);
    }

    Iterator End() {
        auto& self = this->self();
        return self.template _seek<0>(self.sizes()[0]);
    }


    CtrSizeT size() const {
        return self().sizes()[0];
    }

    Iterator find(Key key)
    {
        return self().template find_ge<IntList<0>>(0, key);
    }




MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::table::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}