
// Copyright 2013 Victor Smirnov
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

#include <memoria/prototypes/bt/walkers/bt_find_walkers.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

namespace memoria {
namespace bt {


/**********************************************************************/

template <
    typename Types,
    typename MyType
>
class SelectForwardWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    using Base  = FindForwardWalkerBase<Types, MyType>;
    using CtrSizeT   = typename Types::CtrSizeT;

    SeqOpType op_type_;

public:

    SelectForwardWalkerBase(int32_t symbol, CtrSizeT rank, SeqOpType op_type):
        Base(symbol, rank, SearchType::GE),
        op_type_(op_type)
    {}

    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(const Tree& tree, bool root, int32_t index, int32_t start)
    {
        auto size = tree.size();

        if (start < size)
        {
            auto rank = Base::target_ - Base::sum_;

            auto result = tree.find_for_select_fw(start, rank, index, op_type_);

            Base::sum_ += result.rank;

            return StreamOpResult(result.idx, start, result.idx >= size, false);
        }
        else {
            return StreamOpResult(size, start, true, true);
        }
    }


    template <int32_t StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq& seq, int32_t start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq);

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();

        CtrSizeT rank   = Base::target_ - sum;
        auto result     = self().template select<StreamIdx>(seq, start, symbol, rank);

        if (result.is_found())
        {
            sum  += rank;
            return StreamOpResult(result.local_pos(), start, false);
        }
        else {
            int32_t size = seq.size();

            sum  += result.rank;
            return StreamOpResult(size, start, true);
        }
    }

    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}
};




template <
    typename Types
>
class SelectForwardWalker: public SelectForwardWalkerBase<Types,SelectForwardWalker<Types>> {

    using Base  = SelectForwardWalkerBase<Types,SelectForwardWalker<Types>>;
    using CtrSizeT   = typename Base::CtrSizeT;

protected:
    using Base::direction_;
    using Base::op_type_;

public:
    SelectForwardWalker(int32_t symbol, CtrSizeT rank, SeqOpType op_type):
        Base(symbol, rank, op_type)
    {}

    template <int32_t StreamIdx, typename Seq>
    SelectResult select(const Seq& seq, int32_t start, int32_t symbol, CtrSizeT rank)
    {
        return seq.select_fw_out(start, rank, symbol, op_type_);
    }
};





template <
    typename Types,
    typename MyType
>
class SelectBackwardWalkerBase: public FindBackwardWalkerBase<Types, MyType> {
protected:
    using Base  = FindBackwardWalkerBase<Types, MyType>;
    using CtrSizeT = typename Types::CtrSizeT;

    SeqOpType op_type_;

public:

    SelectBackwardWalkerBase(int32_t symbol, CtrSizeT rank, SeqOpType op_type):
        Base(symbol, rank, SearchType::GE),
        op_type_(op_type)
    {}


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_non_leaf(Tree&& tree, bool root, int32_t index, int32_t start)
    {
        if (start > tree.size()) start = tree.size() - 1;

        if (start >= 0)
        {
            auto rank       = Base::target_ - Base::sum_;

            auto result     = tree.find_for_select_bw(start, rank, index, op_type_);
            Base::sum_      += result.prefix();

            return StreamOpResult(result.idx, start, result.is_end());
        }
        else {
            return StreamOpResult(start, start, true, true);
        }
    }

    template <int32_t StreamIdx, typename Seq>
    StreamOpResult find_leaf(const Seq& seq, int32_t start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq);

        auto size = seq.size();

        if (start > size) start = size;

        CtrSizeT target = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;
        auto symbol     = Base::leaf_index();
        auto result     = self().template select<StreamIdx>(seq, start, symbol, target);

        if (result.is_found())
        {
            sum += target;
            return StreamOpResult(result.local_pos(), start, false);
        }
        else {
            sum += result.rank;
            return StreamOpResult(-1, start, true);
        }
    }

    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}
};




template <
    typename Types
>
class SelectBackwardWalker: public SelectBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {

    using Base  = SelectBackwardWalkerBase<Types, SelectBackwardWalker<Types>>;
    using CtrSizeT   = typename Base::CtrSizeT;

protected:
    using Base::op_type_;
public:

    SelectBackwardWalker(int32_t symbol, CtrSizeT target, SeqOpType op_type):
        Base(symbol, target, op_type)
    {}

    template <int32_t StreamIdx, typename Seq>
    SelectResult select(const Seq& seq, int32_t start, int32_t symbol, CtrSizeT rank)
    {
        return seq.select_bw_out(start, rank, symbol, op_type_);
    }
};




}}
