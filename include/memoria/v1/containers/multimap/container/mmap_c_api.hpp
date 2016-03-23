
// Copyright Victor Smirnov 2015+.
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

MEMORIA_CONTAINER_PART_BEGIN(v1::mmap::CtrApiName)
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
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

public:
    IteratorPtr begin() {
        return self().template seek_stream<0>(0);
    }

    IteratorPtr end() {
        auto& self = this->self();
        return self.template seek_stream<0>(self.sizes()[0]);
    }

    using Base::seek;

    IteratorPtr seek(CtrSizeT idx) {
        auto& self = this->self();
        return self.template seek_stream<0>(idx);
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    IteratorPtr find(Key key)
    {
        return self().template find_max_ge<IntList<0, 0, 1>>(0, key);
    }

protected:

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(v1::mmap::CtrApiName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
