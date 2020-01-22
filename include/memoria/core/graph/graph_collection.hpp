
// Copyright 2018 Victor Smirnov
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/memory/smart_ptrs.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/pimpl_base.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/graph/graph_commons.hpp>


namespace memoria {

template <typename T>
class Iterator;

template <typename T>
struct IIterator {
    virtual ~IIterator() noexcept {}

    virtual void increment() = 0;
    virtual bool equals(const IIterator* other) const = 0;
    virtual bool is_end() const = 0;

    virtual T& access() = 0;

    virtual Iterator<T> clone() const = 0;
};

template <typename T>
struct ICollection {
    virtual ~ICollection() noexcept {}
    virtual Iterator<T> begin() = 0;
    virtual GraphSizeT size() const = 0;
};



template <typename T>
class Iterator: std::iterator<std::output_iterator_tag, T> {
    std::unique_ptr<IIterator<T>> ptr_;
public:
    Iterator() {}

    Iterator(std::unique_ptr<IIterator<T>>&& ptr):
        ptr_(std::move(ptr))
    {}

    Iterator(const Iterator& other)
    {
        if (other.ptr_.get())
        {
            ptr_ = std::move(other.ptr_->clone().ptr_);
        }
    }

    Iterator(Iterator&& other): ptr_(std::move(other.ptr_)) {}

    Iterator& operator=(Iterator&& other) {
        ptr_ = std::move(other.ptr_);
        return *this;
    }



    operator bool() const
    {
        return ptr_.get() != nullptr;
    }

    Iterator& operator++()
    {
        this->ptr_->increment();
        return *this;
    }

    Iterator operator++(int)
    {
        Iterator tmp(ptr_->clone());
        operator++();
        return tmp;
    }

    bool operator==(const Iterator& rhs) const
    {
        return this->ptr_->equals(rhs.ptr_.get());
    }

    bool operator!=(const Iterator& rhs) const {
        return !this->ptr_->equals(rhs.ptr_.get());
    }

    T& operator*() {
        return this->ptr_->access();
    }

    bool is_end() const {
        return ptr_->is_end();
    }

    Iterator& operator=(const Iterator& other)
    {
        ptr_ = std::move(other.ptr_->clone().ptr_);
        return *this;
    }
};


template <typename T>
class Collection: public PimplBase<ICollection<T>> {
    using Base = PimplBase<ICollection<T>>;

    Iterator<T> begin_;
    Iterator<T> end_;

public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Collection)

    Iterator<T>& begin()
    {
        begin_ = this->ptr_->begin();
        return begin_;
    }

    Iterator<T>& end() {
        return end_;
    }

    uint64_t size() const {
        return this->ptr_->size();
    }
};




template <typename T, typename IteratorT>
class StlIterator: public IIterator<T> {
    using MyType = StlIterator;
    IteratorT iter_;
    IteratorT end_;
public:
    StlIterator(const IteratorT& iter, const IteratorT& end):
        iter_(iter), end_(end)
    {}

    virtual void increment() {
        ++iter_;
    }

    virtual bool equals(const IIterator<T>* other) const
    {
        if (!other) {
            return iter_ == end_;
        }
        else {
            const MyType* iother = ptr_cast<const MyType>(other);
            return iter_ == iother->iter_;
        }
    }

    virtual T& access() {
        return *iter_;
    }

    bool is_end() const {
        return iter_ == end_;
    }

    virtual Iterator<T> clone() const
    {
        return Iterator<T>(std::make_unique<MyType>(iter_, end_));
    }
};


template <typename T>
class EmptyIterator: public IIterator<T> {
    using MyType = EmptyIterator;

public:
    EmptyIterator(){}

    virtual void increment() {
        MMA_THROW(GraphException() << WhatCInfo("Incrementing EmptyIterator"));
    }

    virtual bool equals(const IIterator<T>* other) const
    {
        return other == nullptr;
    }

    virtual T& access() {
        MMA_THROW(GraphException() << WhatCInfo("Accessing EmptyIterator"));
    }

    bool is_end() const {
        return true;
    }

    virtual Iterator<T> clone() const
    {
        return Iterator<T>(std::make_unique<MyType>());
    }
};



template <
    typename T,
    typename StlCollectionT = std::vector<T>
>
class STLCollection: public ICollection<T> {
    using MyType = STLCollection;
    StlCollectionT collection_;
    using StlIteratorT = typename StlCollectionT::iterator;

public:
    STLCollection() {}

    STLCollection(StlCollectionT&& other):
        collection_(std::move(other))
    {}

    StlCollectionT& collection() {return collection_;}
    const StlCollectionT& collection() const {return collection_;}

    virtual Iterator<T> begin() {
        return Iterator<T>(std::make_unique<StlIterator<T, StlIteratorT>>(collection_.begin(), collection_.end()));
    }

    virtual GraphSizeT size() const {
        return collection_.size();
    }

    static Collection<T> make(StlCollectionT&& coll)
    {
        return Collection<T>(MakeLocalShared<MyType>(std::move(coll)));
    }
};


template <typename T>
class EmptyCollection: public ICollection<T> {
    using MyType = EmptyCollection;
public:
    EmptyCollection() {}

    virtual Iterator<T> begin() {
        return Iterator<T>(std::make_unique<EmptyIterator<T>>());
    }

    virtual GraphSizeT size() const {
        return 0;
    }

    static Collection<T> make()
    {
        static thread_local Collection<T> instance(MakeLocalShared<MyType>());

        //return Collection<T>(MakeLocalShared<MyType>());
        return instance;
    }
};



}
