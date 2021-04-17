
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

#include <memoria/prototypes/bt/iterator/bt_i_api.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_base.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_find.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_select.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_rank.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_skip.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_leaf.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria {

template <typename Types> struct IterTypesT;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;



template<
        typename Types1
>
class Iter<BTIterTypes<Types1>>: public IterStart<BTIterTypes<Types1>>
{
    typedef Iter<BTIterTypes<Types1>>                                            MyType;
    typedef IterStart<BTIterTypes<Types1>>                                       Base;
    typedef Ctr<typename Types1::CtrTypes>                                       ContainerType;

    typedef typename ContainerType::Types::NodeBasePtr                             NodeBasePtr;

    using CtrPtr = CtrSharedPtr<ContainerType>;

public:

    typedef ContainerType                                                       Container;
    
    Iter(): Base() {}

    Iter(CtrPtr ptr): Base(std::move(ptr))
    {
        Base::iter_local_pos() = 0;
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
        return iter_equals(other);
    }

    bool iter_equals(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return Base::iter_equals(other);
        }
        else if (other.type() == Base::END)
        {
            return Base::iter_is_end();
        }
        else if (other.type() == Base::START)
        {
            return Base::iter_is_begin();
        }
        else
        {
            return Base::iter_is_empty();
        }
    }

    bool operator!=(const MyType& other) const
    {
        return iter_not_equals(other);
    }

    bool iter_not_equals(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return Base::iter_not_equals(other);
        }
        else if (other.type() == Base::END)
        {
            return Base::iter_is_not_end();
        }
        else if (other.type() == Base::START)
        {
            return !Base::iter_is_begin();
        }
        else
        {
            return !Base::iter_is_empty();
        }
    }
};

template <typename Types>
bool operator==(const Iter<BTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.iter_is_end();
}

template <typename Types>
bool operator!=(const Iter<BTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.iter_is_not_end();
}


}
