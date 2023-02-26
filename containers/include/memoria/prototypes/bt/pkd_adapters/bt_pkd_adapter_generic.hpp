
// Copyright 2011 Victor Smirnov
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
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

namespace memoria {
namespace bt {

template <typename PkdStructSO, PkdSearchType SearchType = PkdKeySearchType<typename PkdStructSO::PkdStructT>>
class BTPkdStructAdaper {};

template <typename PkdStructSO>
class BTPkdStructAdaperBase {
protected:
    PkdStructSO pkd_struct_;
public:
    BTPkdStructAdaperBase(PkdStructSO pkd_struct) :
        pkd_struct_(pkd_struct)
    {}

    template <typename T, int32_t Indexes>
    void assign_to(core::StaticVector<T, Indexes>& tgt, size_t idx)
    {
        for (int32_t c = 0; c < Indexes; c++) {
            tgt[c] = pkd_struct_.access(c, idx);
        }
    }

    template <typename T, int32_t Indexes>
    void assign_to(core::StaticVector<Optional<T>, Indexes>& tgt, size_t idx)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            tgt[c] = pkd_struct_.access(c, idx);
        }
    }

protected:
    template <typename TT>
    void set_empty(TT& dest) const  {
        dest = TT{};
    }

    template <typename TT>
    void set_empty(Optional<TT>& dest) const  {
        dest = Optional<TT>{};
    }
};



template <typename PkdStructSO>
class BTPkdStructAdaper<PkdStructSO, PkdSearchType::MAX>: public BTPkdStructAdaperBase<PkdStructSO> {

    using Base = BTPkdStructAdaperBase<PkdStructSO>;

    static constexpr int32_t Indexes = PkdStructIndexes<typename PkdStructSO::PkdStructT>;

protected:
    using Base::set_empty;
    using Base::pkd_struct_;
public:
    BTPkdStructAdaper(PkdStructSO pkd_struct) :
        Base(pkd_struct)
    {}

    template <typename T>
    void branch_max_entry(core::StaticVector<T, Indexes>& accum) const
    {
        psize_t size = pkd_struct_.size();

        if (size > 0)
        {
            psize_t max_element_idx = pkd_struct_.max_element_idx();
            for (int32_t c = 0; c < Indexes; c++) {
                accum[c] = pkd_struct_.access(c, max_element_idx);
            }
        }
        else {
            set_empty(accum);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void leaf_max_entry(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset + Indexes <= Size, "Invalid balanced tree structure");
        psize_t size = pkd_struct_.size();

        if (size > 0)
        {
            psize_t max_element_idx = pkd_struct_.max_element_idx();
            for (int32_t c = Offset; c < Offset + Indexes; c++) {
                accum[c] = pkd_struct_.access(c, max_element_idx);
            }
        }
        else {
            for (int32_t c = Offset; c < Offset + Indexes; c++) {
                set_empty(accum[c]);
            }
        }
    }
};



template <typename PkdStructSO>
class BTPkdStructAdaper<PkdStructSO, PkdSearchType::SUM>: public BTPkdStructAdaperBase<PkdStructSO> {

    using Base = BTPkdStructAdaperBase<PkdStructSO>;

    static constexpr int32_t Indexes = PkdStructIndexes<typename PkdStructSO::PkdStructT>;

protected:
    using Base::set_empty;
    using Base::pkd_struct_;
public:
    BTPkdStructAdaper(PkdStructSO pkd_struct) :
        Base(pkd_struct)
    {}

    template <typename T>
    void branch_max_entry(core::StaticVector<T, Indexes>& accum) const
    {
        auto size = pkd_struct_.size();

        if (size)
        {
            for (int32_t c = 0; c < Indexes; c++) {
                accum[c] = pkd_struct_.sum(c);
            }
        }
        else {
            set_empty(accum);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void leaf_max_entry(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset + Indexes <= Size, "Invalid balanced tree structure");
        auto size = pkd_struct_.size();

        if (size)
        {
            for (int32_t c = 0; c < Indexes; c++) {
                accum[c + Offset] = pkd_struct_.sum(c);
            }
        }
        else {
            for (int32_t c = 0; c < Indexes; c++) {
                set_empty(accum[c + Offset]);
            }
        }
    }


    template <typename T>
    void branch_sum_entries(int32_t start, int32_t end, core::StaticVector<T, Indexes>& accum) const
    {
        for (int32_t c = 0; c < Indexes; c++) {
            accum[c] += pkd_struct_.sum(c, start, end);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void leaf_sum_entries(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset + Indexes <= Size, "Invalid balanced tree structure");

        for (int32_t c = 0; c < Indexes; c++) {
            accum[c + Offset] += pkd_struct_.sum(c, start, end);
        }
    }
};





}
}
