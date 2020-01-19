
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

#include <memoria/core/memory/smart_ptrs.hpp>


#include <memory>



namespace memoria {
    
template <typename T, template <typename> class SharedPtrT = LocalSharedPtr>
struct PimplBase {
    using TargetType = T;
    using PtrType = SharedPtrT<TargetType>;
protected:
    PtrType ptr_;
public:
    PimplBase() noexcept {}
    PimplBase(PtrType ptr) noexcept: ptr_(ptr) {}
    PimplBase(const PimplBase&) noexcept = default;
    PimplBase(PimplBase&&) noexcept = default;
    ~PimplBase() noexcept = default;

    
    PimplBase& operator=(const PimplBase&) noexcept = default;
    PimplBase& operator=(PimplBase&&) noexcept = default;
    
    bool operator==(const PimplBase& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    void reset() noexcept {
        ptr_.reset();
    }

    operator bool () const noexcept {
        return ptr_.get() != nullptr;
    }

    PtrType ptr() noexcept {
        return ptr_;
    }
};


#define MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(ClassName)     \
    ClassName() noexcept :Base(){}                                    \
    ClassName(const typename Base::PtrType& ptr) noexcept: Base(ptr) {}\
    ClassName(const ClassName& other) noexcept: Base(other) {}       \
    ClassName(ClassName&& other) noexcept: Base(std::move(other)) {} \
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


#define MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS_NO_DTR(ClassName)  \
    ClassName():Base(){}										\
    ClassName(const typename Base::PtrType& ptr): Base(ptr) {}  \
    ClassName(const ClassName& other): Base(other) {}       \
    ClassName(ClassName&& other): Base(std::move(other)) {} \
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


#define MMA1_UNIQUE_PTR_DECLARE_DEFAULT_FUNCTIONS(ClassName)\
    ClassName(const ClassName& ptr) = delete;               \
    ClassName(ClassName&& other): ptr_(std::move(other.ptr_)) {} \
    ~ClassName() noexcept {}                                \
                                                            \
    ClassName& operator=(const ClassName& other) = delete;  \
                                                            \
                                                            \
    ClassName& operator=(ClassName&& other) {               \
        ptr_ = std::move(other);                            \
        return *this;                                       \
    }                                                       \
                                                            \
    bool operator==(const ClassName& other) const {         \
        return ptr_ == other.ptr_;                          \
    }

#define MMA1_UNIQUE_PTR_DECLARE_DEFAULT_FUNCTIONS_NO_DTR(ClassName)\
    ClassName(const ClassName& ptr) = delete;               \
    ClassName(ClassName&& other): ptr_(std::move(other.ptr_)) {} \
                                                            \
    ClassName& operator=(const ClassName& other) = delete;  \
                                                            \
    ClassName& operator=(ClassName&& other) {               \
        ptr_ = std::move(other);                            \
        return *this;                                       \
    }                                                       \
                                                            \
    bool operator==(const ClassName& other) const {         \
        return ptr_ == other.ptr_;                          \
    }

}
