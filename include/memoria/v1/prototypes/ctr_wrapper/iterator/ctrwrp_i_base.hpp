
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

#include <memoria/v1/prototypes/bt/bt_macros.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(CtrWrapperIteratorBase)

    typedef TypesType                                                           Types;
    typedef Ctr<typename Types::CtrTypes>                                       Container;
    typedef typename Container::WrappedCtr                                      WrappedCtr;
    typedef typename WrappedCtr::Iterator                                       WrappedIter;


    CtrWrapperIteratorBase():
        Base()
    {

    }

    CtrWrapperIteratorBase(ThisType&& other):
        Base(std::move(other))
    {

    }

    CtrWrapperIteratorBase(const ThisType& other): Base(other)
    {

    }

    void assign(ThisType&& other)
    {
        Base::assign(std::move(other));
    }

    void assign(const ThisType& other)
    {
        Base::assign(other);
    }

    bool isEqual(const ThisType& other) const
    {
        return Base::isEqual(other);
    }

    bool isNotEqual(const ThisType& other) const
    {
        return Base::isNotEqual(other);
    }

    void dump(ostream& out = cout, const char* header = NULL)
    {

    }

MEMORIA_BT_ITERATOR_BASE_CLASS_END;

}}