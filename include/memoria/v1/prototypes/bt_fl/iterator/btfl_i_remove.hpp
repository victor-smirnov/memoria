
// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorRemoveName)


    using typename Base::NodeBaseG;
    using Container = typename Base::Container;
    using typename Base::Position;

    using CtrSizeT  = typename Container::Types::CtrSizeT;
    using CtrSizesT = typename Container::Types::CtrSizesT;

    using BranchNodeEntry               = typename Container::Types::BranchNodeEntry;
    using IteratorBranchNodeEntry       = typename Container::Types::IteratorBranchNodeEntry;

    static const int32_t Streams          		= Container::Types::Streams;
    static const int32_t DataStreams      		= Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:
    Position removeGE(CtrSizeT n)
    {
        auto& self = this->self();
        CtrSizesT sizes;

        if (!self.isEnd())
        {
        	auto ii = self.clone();
        	ii->selectGEFw(n, self.data_stream());

        	auto start = self.leafrank();
        	auto end   = ii->leafrank();

        	self.ctr().removeEntries(self.leaf(), start, ii->leaf(), end, sizes, true);

        	self.local_pos() = end[StructureStreamIdx];

            self.leaf().assign(ii->leaf());

        	self.refresh();
        }

        return sizes;
    }
    
    CtrSizeT remove_next(CtrSizeT n)
    {
        auto& self = this->self();
        CtrSizesT sizes;
        CtrSizeT size{};

        if (!self.isEnd())
        {
        	auto ii = self.clone();

        	size = ii->skipFw(n);

        	auto start = self.leafrank();
        	auto end   = ii->leafrank();

        	self.ctr().removeEntries(self.leaf(), start, ii->leaf(), end, sizes, true);

        	self.local_pos() = end[StructureStreamIdx];

            self.leaf().assign(ii->leaf());

        	self.refresh();
        }

        return size;
    }


    bool remove_all(MyType& to)
    {
        auto& self = this->self();
        CtrSizesT sizes;

        if (!self.isEnd())
        {
            auto start = self.leafrank();
            auto end   = to.leafrank();

            self.ctr().removeEntries(self.leaf(), start, to.leaf(), end, sizes, true);

            self.local_pos() = end[StructureStreamIdx];

            self.leaf().assign(to.leaf());

            self.refresh();
        }

        return sizes.sum() != 0;
    }


protected:


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorRemoveName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
