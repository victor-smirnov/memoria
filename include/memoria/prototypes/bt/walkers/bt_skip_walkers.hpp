
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

/***************************************************************************************/


template <
    typename Types,
    typename MyType
>
class SkipForwardWalkerBase: public FindForwardWalkerBase<Types, MyType> {
protected:
    using Base  = FindForwardWalkerBase<Types, MyType>;
    using CtrSizeT = typename Types::CtrSizeT;

public:

    SkipForwardWalkerBase(CtrSizeT target):
        Base(0, target, SearchType::GT)
    {}


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree& tree, int32_t start)
    {
        auto& sum = Base::sum_;

        int64_t offset = Base::target_ - sum;

        if (tree)
        {
            int32_t size = tree.size();

            if (start + offset < size)
            {
                sum += offset;

                return StreamOpResult(start + offset, start, false);
            }
            else {
                sum += (size - start);

                return StreamOpResult(size, start, true);
            }
        }
        else {
            return StreamOpResult(0, start, true);
        }
    }

    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}
};



template <
    typename Types
>
class SkipForwardWalker: public SkipForwardWalkerBase<Types, SkipForwardWalker<Types>> {
    using Base  = SkipForwardWalkerBase<Types, SkipForwardWalker<Types>>;

    using CtrSizeT   = typename Base::CtrSizeT;

public:

    SkipForwardWalker(CtrSizeT target):
        Base(target)
    {}
};




template <
    typename Types,
    typename MyType
>
class SkipBackwardWalkerBase: public FindBackwardWalkerBase<Types,MyType> {
protected:
    using Base  = FindBackwardWalkerBase<Types,MyType>;

    using CtrSizeT = typename Types::CtrSizeT;

public:

    SkipBackwardWalkerBase(CtrSizeT target):
        Base(0, target, SearchType::GE)
    {}


    template <int32_t StreamIdx, typename Tree>
    StreamOpResult find_leaf(const Tree& tree, int32_t start)
    {
        auto size = tree ? tree.size() : 0;

        if (start > size) start = size;

        if (start >= 0)
        {
            int64_t offset = Base::target_ - Base::sum_;

            auto& sum = Base::sum_;

            if (tree != nullptr)
            {
                if (start - offset >= 0)
                {
                    sum += offset;

                    return StreamOpResult(start - offset, start, false);
                }
                else {
                    sum += start;

                    return StreamOpResult(-1, start, true);
                }
            }
            else {
                return StreamOpResult(-1, start, true, true);
            }
        }
        else {
            return StreamOpResult(-1, start, true, true);
        }
    }

    MyType& self() {return *ptr_cast<MyType>(this);}
    const MyType& self() const {return *ptr_cast<const MyType>(this);}
};


template <
    typename Types
>
class SkipBackwardWalker: public SkipBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
    using Base  = SkipBackwardWalkerBase<Types,SkipBackwardWalker<Types>>;

    using CtrSizeT = typename Base::CtrSizeT;

public:
    SkipBackwardWalker(CtrSizeT target):
        Base(target)
    {}
};




}}
