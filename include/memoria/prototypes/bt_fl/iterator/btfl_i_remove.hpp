
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

#include <memoria/core/types.hpp>
#include <memoria/core/types/algo/for_each.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorRemoveName)

    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;
    using typename Base::Container;
    using typename Base::TreeNodePtr;
    using typename Base::Position;

    static const int32_t Streams          		= Container::Types::Streams;
    static const int32_t DataStreams      		= Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:
    Position iter_remove_eq_nlt(CtrSizeT n)
    {
        auto& self = this->self();
        CtrSizesT sizes{};

        if (n > 0 && !self.iter_is_end())
        {
            auto ii = self.iter_clone();
            ii->iter_btfl_select_fw(n, self.iter_data_stream(), SeqOpType::EQ_NLT);

            auto start = self.iter_leafrank();
            auto end   = ii->iter_leafrank();

            self.ctr().ctr_remove_entries(self.path(), start, ii->path(), end, true);

            self.iter_local_pos() = end[StructureStreamIdx];

            // FIXME: refresh iovector view
            //self.path() = ii->path();
            //self.iter_leaf().assign(ii->iter_leaf());

            self.iter_refresh();
        }

        return sizes;
    }
    
    CtrSizeT iter_remove_next(CtrSizeT n)
    {
        auto& self = this->self();
        CtrSizesT sizes;
        CtrSizeT size{};

        if (!self.iter_is_end())
        {
            auto ii = self.iter_clone();

            auto distance = ii->iter_skip_fw(n);
            size = distance;

        	auto start = self.iter_leafrank();
        	auto end   = ii->iter_leafrank();

            self.ctr().ctr_remove_entries(self.path(), start, ii->path(), end, sizes, true);

            self.iter_local_pos() = end[StructureStreamIdx];
            //self.path() = ii->path();
            //self.iter_leaf().assign(ii->iter_leaf());

            self.iter_refresh();
        }

        return size;
    }


    bool iter_remove_all(MyType& to)
    {
        auto& self = this->self();
        CtrSizesT sizes;

        if (!self.iter_is_end())
        {
            auto start = self.iter_leafrank();
            auto end   = to.iter_leafrank();

            self.ctr().ctr_remove_entries(self.path(), start, to.path(), end, true);

            self.iter_local_pos() = end[StructureStreamIdx];
            self.iter_leaf().assign(to.iter_leaf());

            self.iter_refresh();
        }

        return sizes.sum() != 0;
    }


protected:


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorRemoveName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
