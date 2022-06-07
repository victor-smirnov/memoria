
// Copyright 2011-2022 Victor Smirnov
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

#include <memoria/prototypes/bt/iterator/bt_bis_base.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_base.hpp>

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
    using MyType        = Iter<BTIterTypes<Types1>>;
    using Base          = IterStart<BTIterTypes<Types1>>;
    using ContainerType = Ctr<typename Types1::CtrTypes>;
    using TreeNodePtr   = typename ContainerType::Types::TreeNodePtr;
    using CtrPtr        = CtrSharedPtr<ContainerType>;

public:

    using Container = ContainerType;
    
    Iter(): Base() {}

    Iter(CtrPtr ptr): Base(std::move(ptr)) {
        Base::iter_local_pos() = 0;
    }
    
    Iter(const MyType& other): Base(other) {}

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            Base::assign(other);
        }

        return *this;
    }
};



template<
        typename Types1
>
class Iter<BTBlockIterStateTypes<Types1>>: public IterStart<BTBlockIterStateTypes<Types1>>
{
    using MyType        = Iter<BTBlockIterStateTypes<Types1>>;
    using Base          = IterStart<BTBlockIterStateTypes<Types1>>;
    using ContainerType = Ctr<typename Types1::CtrTypes>;
    using TreeNodePtr   = typename ContainerType::Types::TreeNodePtr;
    using CtrPtr        = CtrSharedPtr<ContainerType>;

public:

    using Container = ContainerType;

    Iter(): Base() {}

    void reset_state() {
        Base::reset_state();
        Base::ctr_ptr_.reset();
    }

    void iter_initialize(const CtrPtr& ctr_holder) {
        Base::ctr_ptr_ = ctr_holder;
        Base::model_ = ctr_holder.get();

        Base::iter_initialize(ctr_holder);
    }
};




}
