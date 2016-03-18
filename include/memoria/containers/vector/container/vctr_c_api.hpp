
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

public:
    using typename Base::Types;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::PageUpdateMgr;
    using typename Base::CtrSizeT;

    using Value = typename Types::Value;

public:
    CtrSizeT size() const {
        return self().sizes()[0];
    }

    auto seek(CtrSizeT pos)
    {
        typename Types::template SkipForwardWalker<Types, IntList<0>> walker(pos);

        return self().find_(walker);
    }


    MyType& operator<<(const vector<Value>& v)
    {
        auto& self = this->self();
        auto i = self.seek(self.size());
        i.insert_v(v);
        return self;
    }

MEMORIA_CONTAINER_PART_END

}


#endif
