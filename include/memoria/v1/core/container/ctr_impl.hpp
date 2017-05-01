
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename CtrName, typename Allocator, typename Profile>
class SharedCtr: public CtrTF<Profile, CtrName, CtrName>::Type {
    using Base = typename CtrTF<Profile, CtrName, CtrName>::Type;
public:
    SharedCtr(const std::shared_ptr<Allocator>& allocator, Int command, const UUID& name):
        Base(allocator.get(), command, name)
    {
        Base::alloc_holder_ = allocator;
    }

    auto snapshot() const {
        return Base::alloc_holder_;
    }
};


template <typename CtrName, typename Profile>
class SharedIter: public Iter<typename CtrTF<Profile, CtrName, CtrName>::Types::IterTypes> {
    
    using MyType1 = SharedIter<CtrName,Profile>;
    
    using CtrT = typename CtrTF<Profile, CtrName, CtrName>::Type;
    using IterT = Iter<typename CtrTF<Profile, CtrName, CtrName>::Types::IterTypes>;

    using CtrPtr = CtrSharedPtr<Profile, CtrT>;
    
    using Base = Iter<typename CtrTF<Profile, CtrName, CtrName>::Types::IterTypes>;
public:
    SharedIter(): Base(){}

    SharedIter(CtrPtr ptr): Base(std::move(ptr))
    {
        Base::idx() = 0;
    }
    
    SharedIter(const MyType1& other): Base(other) {}
    SharedIter(const IterT& other): Base(other) {}
};

    
}
}
