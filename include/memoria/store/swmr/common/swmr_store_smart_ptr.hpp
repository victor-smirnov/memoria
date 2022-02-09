
// Copyright 2020-2021 Victor Smirnov
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

#include <boost/pool/object_pool.hpp>

namespace memoria {

namespace detail {

class SBPtrSharedBase {
    int32_t references_{};
public:
    virtual ~SBPtrSharedBase() noexcept = default;

    virtual void release() noexcept = 0;
    virtual void flush() = 0;

    void ref() {
        references_++;
    }

    void unref() {
        if (--references_ == 0) {
            release();
        }
    }
};

class MMapSBPtrPooledSharedImpl: public SBPtrSharedBase {
    boost::object_pool<MMapSBPtrPooledSharedImpl>* pool_;

public:
    MMapSBPtrPooledSharedImpl(boost::object_pool<MMapSBPtrPooledSharedImpl>* pool)  :
        pool_(pool)
    {}

    void release() noexcept {
        pool_->destroy(this);
    }

    void flush()  {}
};

class MMapSBPtrNewSharedImpl: public SBPtrSharedBase {
public:
    void release() noexcept {
        delete this;
    }

    void flush()  {}
};

}


template <typename BlockT>
class SharedSBPtr {
    BlockT* block_ptr_;
    detail::SBPtrSharedBase* shared_;
public:
    SharedSBPtr() :
        block_ptr_(), shared_()
    {}

    SharedSBPtr(BlockT* ptr, detail::SBPtrSharedBase* shared) :
        block_ptr_(ptr), shared_(shared)
    {
        shared->ref();
    }

    SharedSBPtr(const SharedSBPtr& other) :
        block_ptr_(other.block_ptr_), shared_(other.shared_)
    {
        shared_->ref();
    }

    SharedSBPtr(SharedSBPtr&& other) :
        block_ptr_(other.block_ptr_), shared_(other.shared_)
    {
        other.shared_ = nullptr;
    }

    ~SharedSBPtr() noexcept {
        if (shared_) {
            shared_->unref();
        }
    }

    SharedSBPtr& operator=(const SharedSBPtr& other)
    {
        auto old_shared = shared_;

        block_ptr_ = other.block_ptr_;
        shared_ = other.shared_;

        shared_->ref();

        if (old_shared) {
            old_shared->unref();
        }

        return *this;
    }

    SharedSBPtr& operator=(SharedSBPtr&& other)
    {
        if (&other != this)
        {
            auto old_shared = shared_;

            block_ptr_ = other.block_ptr_;
            shared_ = other.shared_;
            other.shared_ = nullptr;

            shared_->ref();

            if (old_shared) {
                old_shared->unref();
            }
        }

        return *this;
    }

    BlockT* operator->() const  {
        return block_ptr_;
    }

    BlockT* get() const  {
        return block_ptr_;
    }

    void reset()  {
        if (shared_)
        {
            shared_->unref();
        }
    }

    BlockT& operator*() const {
        return *block_ptr_;
    }

    void flush() {
        shared_->flush();
    }

    operator bool() const  {
        return (bool)shared_;
    }
};


}
