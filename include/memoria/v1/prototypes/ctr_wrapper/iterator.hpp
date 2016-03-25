
// Copyright 2011 Victor Smirnov, Ivan Yurchenko
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

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/prototypes/ctr_wrapper/ctrwrapper_names.hpp>

namespace memoria {
namespace v1 {



template <typename Types>
class Iter<IterWrapperTypes<Types> >: public IterStart<IterWrapperTypes<Types> >
{
    typedef IterStart<IterWrapperTypes<Types> >                 Base;
    typedef Iter<IterWrapperTypes<Types> >                      MyType;

public:

    typedef Ctr<typename Types::CtrTypes>                                       ContainerType;

    typedef typename ContainerType::WrappedCtr::Iterator                        WrappedIterator;

    typedef typename Types::Profile                                             Profile;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Allocator::CtrShared                                CtrShared;
    typedef typename Types::Allocator::PageG                                    PageG;
    typedef typename PageG::Page::ID                                            ID;

private:
    ContainerType&      model_;
    WrappedIterator     iter_;

    bool                exists_;

public:

    Iter(ContainerType &model):
        model_(model), iter_(model.ctr()), exists_(false) {}

    Iter(const MyType& other):
        Base(other), model_(other.model_), iter_(other.iter_), exists_(other.exists_) {}

    Iter(ContainerType &model, const WrappedIterator& iter, bool exists = false):
        model_(model), iter_(iter), exists_(exists) {}

    //We have no move constructors for iterator

    MyType* me() {
        return this;
    }

    const MyType* me() const {
        return this;
    }

    ContainerType& model() {
        return model_;
    }

    const ContainerType& model() const {
        return model_;
    }

    ContainerType& ctr() {
        return model_;
    }

    const ContainerType& ctr() const {
        return model_;
    }

    MyType& operator=(MyType&& other)
    {
        if (this != &other)
        {
            iter_ = std::move(other.iter_);

            Base::assign(other);
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            iter_ = other.iter_;

            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(WrappedIterator&& iter)
    {
        iter_ = iter;
        return *this;
    }


    WrappedIterator& iter()
    {
        return iter_;
    }

    const WrappedIterator& iter() const
    {
        return iter_;
    }

    bool exists() const
    {
        return exists_;
    }


    bool operator==(const MyType& other) const
    {
        return isEqual(other);
    }

    bool isEqual(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return iter_ == other.iter_ && Base::isEqual(other);
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
            return iter_ != other.iter_ || Base::isNotEqual(other);
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

    bool isEnd() const {
        return iter_.isEnd();
    }

    bool isNotEnd() const {
        return iter_.isNotEnd();
    }

    template <typename T>
    T operator=(const T & value)
    {
        AssignToItem(*this, value);
        return value;
    }

    template <typename T>
    T operator=(T&& value)
    {
        AssignToItem(*this, std::forward<T>(value));
        return value;
    }
};


template <typename Types>
bool operator==(const Iter<IterWrapperTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<IterWrapperTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}

}}