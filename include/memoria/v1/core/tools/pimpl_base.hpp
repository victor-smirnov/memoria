
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

#include <memoria/v1/core/memory/smart_ptrs.hpp>


#include <memory>



namespace memoria {
namespace v1 {

    
template <typename T>
struct PimplBase {
    using TargetType = T;
    using PtrType = LocalSharedPtr<TargetType>;
protected:
    PtrType ptr_;
public:
    PimplBase(PtrType ptr): ptr_(ptr) {}
    PimplBase(const PimplBase&) = default;
    PimplBase(PimplBase&&) = default;
    ~PimplBase() noexcept = default;

    
    PimplBase& operator=(const PimplBase&) = default;
    PimplBase& operator=(PimplBase&&) = default;
    
    bool operator==(const PimplBase& other) const {
        return ptr_ == other.ptr_;
    }

    void reset() {
        ptr_.reset();
    }
};


#define MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(ClassName)     \
    ClassName(const typename Base::PtrType& ptr): Base(ptr) {}\
    ClassName(const ClassName& other): Base(other) {}       \
    ClassName(ClassName&& other): Base(std::move(other)) {} \
    ~ClassName() noexcept {}                                \
                                                            \
    ClassName& operator=(const ClassName& other) {          \
        Base::operator=(other);                             \
        return *this;                                       \
    }                                                       \
                                                            \
    ClassName& operator=(ClassName&& other) {               \
        Base::operator=(std::move(other));                  \
        return *this;                                       \
    }                                                       \
                                                            \
    bool operator==(const ClassName& other) const {         \
        return this->ptr_ == other.ptr_;                    \
    }


}}
