
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
    Result<Position> iter_remove_ge(CtrSizeT n) noexcept
    {
        using ResultT = Result<Position>;

        auto& self = this->self();
        CtrSizesT sizes;

        if (!self.iter_is_end())
        {
            auto ii = self.iter_clone();
            auto res0 = ii->iter_select_ge_fw(n, self.iter_data_stream());
            MEMORIA_RETURN_IF_ERROR(res0);

        	auto start = self.iter_leafrank();
        	auto end   = ii->iter_leafrank();

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr().ctr_remove_entries(self.iter_leaf(), start, ii->iter_leaf(), end, sizes, true));

            self.iter_local_pos() = end[StructureStreamIdx];

            self.iter_leaf().assign(ii->iter_leaf());

            MEMORIA_RETURN_IF_ERROR_FN(self.iter_refresh());
        }

        return ResultT::of(sizes);
    }
    
    Result<CtrSizeT> iter_remove_next(CtrSizeT n) noexcept
    {
        using ResultT = Result<CtrSizeT>;

        auto& self = this->self();
        CtrSizesT sizes;
        CtrSizeT size{};

        if (!self.iter_is_end())
        {
            auto ii = self.iter_clone();

            auto res = ii->iter_skip_fw(n);
            MEMORIA_RETURN_IF_ERROR(res);

            size = res.get();

        	auto start = self.iter_leafrank();
        	auto end   = ii->iter_leafrank();

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr().ctr_remove_entries(self.iter_leaf(), start, ii->iter_leaf(), end, sizes, true));

            self.iter_local_pos() = end[StructureStreamIdx];
            self.iter_leaf().assign(ii->iter_leaf());

            MEMORIA_RETURN_IF_ERROR_FN(self.iter_refresh());
        }

        return ResultT::of(size);
    }


    BoolResult iter_remove_all(MyType& to) noexcept
    {
        auto& self = this->self();
        CtrSizesT sizes;

        if (!self.iter_is_end())
        {
            auto start = self.iter_leafrank();
            auto end   = to.iter_leafrank();

            MEMORIA_RETURN_IF_ERROR_FN(self.ctr().ctr_remove_entries(self.iter_leaf(), start, to.iter_leaf(), end, sizes, true));

            self.iter_local_pos() = end[StructureStreamIdx];
            self.iter_leaf().assign(to.iter_leaf());

            MEMORIA_RETURN_IF_ERROR_FN(self.iter_refresh());
        }

        return BoolResult::of(sizes.sum() != 0);
    }


protected:


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorRemoveName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
