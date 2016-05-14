
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

#include <memoria/v1/prototypes/bt/iterator/bt_i_api.hpp>
#include <memoria/v1/prototypes/bt/iterator/bt_i_base.hpp>
#include <memoria/v1/prototypes/bt/iterator/bt_i_find.hpp>
#include <memoria/v1/prototypes/bt/iterator/bt_i_select.hpp>
#include <memoria/v1/prototypes/bt/iterator/bt_i_rank.hpp>
#include <memoria/v1/prototypes/bt/iterator/bt_i_skip.hpp>
#include <memoria/v1/prototypes/bt/iterator/bt_i_leaf.hpp>
#include <memoria/v1/prototypes/bt/bt_names.hpp>

namespace memoria {
namespace v1 {

template <typename Types> struct IterTypesT;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;



template<
        typename Types
>
class Iter<BTIterTypes<Types>>: public IterStart<BTIterTypes<Types>>
{
    typedef Iter<BTIterTypes<Types>>                                            MyType;
    typedef IterStart<BTIterTypes<Types>>                                       Base;
    typedef Ctr<typename Types::CtrTypes>                                       ContainerType;

    typedef typename ContainerType::Types::NodeBaseG                            NodeBaseG;

    using CtrPtr = std::shared_ptr<ContainerType>;

public:

    typedef ContainerType                                                       Container;
    
    Iter(): Base() {}

    Iter(const CtrPtr& ptr): Base(ptr)
    {
        Base::idx() = 0;
    }
    
    Iter(const MyType& other): Base(other) {}



    MyType& operator=(MyType&& other)
    {
        if (this != &other)
        {
            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            Base::assign(other);
        }

        return *this;
    }

    bool operator==(const MyType& other) const
    {
        return isEqual(other);
    }

    bool isEqual(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return Base::isEqual(other);
        }
        else if (other.type() == Base::END)
        {
            return Base::isEnd();
        }
        else if (other.type() == Base::START)
        {
            return Base::isBegin();
        }
        else
        {
            return Base::isEmpty();
        }
    }

    bool operator!=(const MyType& other) const
    {
        return isNotEqual(other);
    }

    bool isNotEqual(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return Base::isNotEqual(other);
        }
        else if (other.type() == Base::END)
        {
            return Base::isNotEnd();
        }
        else if (other.type() == Base::START)
        {
            return !Base::isBegin();
        }
        else
        {
            return !Base::isEmpty();
        }
    }

    template <typename T>
    MyType& operator=(const T& value)
    {
        AssignToItem(*this, value);
        return *this;
    }
};

template <typename Types>
bool operator==(const Iter<BTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<BTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}


}}
