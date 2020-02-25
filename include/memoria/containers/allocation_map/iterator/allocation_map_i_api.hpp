
// Copyright 2020 Victor Smirnov
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

#include <memoria/containers/allocation_map/allocation_map_names.hpp>
#include <memoria/core/types.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(alcmap::ItrApiName)

    using typename Base::CtrSizeT;

    Result<CtrSizeT> count_fw() noexcept
    {
        typename Types::template CountForwardWalker<Types, IntList<0, 1>> walker(0);
        MEMORIA_TRY(res, self().iter_find_fw(walker));
        return Result<CtrSizeT>::of(res);
    }



    struct SetClearBitsFn {

        template <typename T>
        Result<CtrSizeT> treeNode(T&& node_so, int32_t start, int32_t level, CtrSizeT size, bool set_bits) const noexcept
        {
            using ResultT = Result<CtrSizeT>;
            auto bitmap = node_so.template substream_by_idx<1>();
            int32_t bm_size = bitmap.data()->size(level);

            int32_t limit = (start + size) < bm_size ? (start + size) : bm_size;

            if (set_bits) {
                MEMORIA_TRY_VOID(bitmap.data()->set_bits(level, start, limit - start));
            }
            else {
                MEMORIA_TRY_VOID(bitmap.data()->clear_bits(level, start, limit - start));
            }

            MEMORIA_TRY_VOID(bitmap.data()->reindex());

            return ResultT::of(limit - start);
        }

    };

    Result<CtrSizeT> iter_setup_bits(int32_t level, CtrSizeT size, bool set_bits) noexcept
    {
        using ResultT = Result<CtrSizeT>;
        auto& self = this->self();

        int32_t local_pos = self.iter_local_pos() >> level;

        CtrSizeT accum{};
        CtrSizeT remainder = size;
        while (accum < size)
        {
            MEMORIA_TRY(processed, self.ctr().leaf_dispatcher().dispatch(self.path().leaf(), SetClearBitsFn(), local_pos, level, remainder, set_bits));

            accum += processed;

            MEMORIA_TRY_VOID(self.ctr().ctr_update_path(self.path(), 0));

            if (accum < size)
            {
                MEMORIA_TRY(has_next, self.next_leaf());
                if (!has_next) {
                    break;
                }

                self.iter_local_pos() = local_pos = 0;
            }
            else {
                self.iter_local_pos() = processed << level;
            }
        }

        MEMORIA_TRY(leaf_size, self.iter_leaf_size());

        if (leaf_size == self.iter_local_pos())
        {
            MEMORIA_TRY_VOID(self.next_leaf());
        }

        return ResultT::of(accum);
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(alcmap::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
