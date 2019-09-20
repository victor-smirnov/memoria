
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


#include <memoria/v1/containers/wt/wt_names.hpp>

#include <memoria/v1/core/container/container.hpp>

namespace memoria {
namespace v1 {

template <typename Types>
class Iter<WTIterTypes<Types> >: public IterStart<WTIterTypes<Types> >
{
    typedef IterStart<WTIterTypes<Types> >                                      Base;
    typedef Iter<WTIterTypes<Types> >                                           MyType;
    typedef Ctr<WTCtrTypes<Types> >                                             ContainerType;

    typedef typename ContainerType::Seq::Iterator                               SeqIterator;
    typedef typename ContainerType::Tree::Iterator                              TreeIterator;

    typedef typename Types::Profile                                             Profile;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Allocator::CtrShared                                CtrShared;
    typedef typename Types::Allocator::BlockG                                    BlockG;
    typedef typename BlockG::Page::ID                                            ID;


    ContainerType&      model_;

    TreeIterator        tree_iter_;
    SeqIterator         seq_iter_;
    bool                exists_;

public:

    Iter(ContainerType &model):
        model_(model), tree_iter_(model.tree()), seq_iter_(model.seq()), exists_(false) {}

    Iter(const MyType& other):
        model_(other.model_), tree_iter_(other.tree_iter_), seq_iter_(other.seq_iter_), exists_(other.exists_) {}

    Iter(ContainerType &model, const TreeIterator& tree_iter, const SeqIterator& seq_iter, bool exists = false):
        model_(model), tree_iter_(tree_iter), seq_iter_(seq_iter), exists_(exists) {}

    Iter(ContainerType &model, const TreeIterator& tree_iter, bool exists = false):
        model_(model), tree_iter_(tree_iter), seq_iter_(model.seq()), exists_(exists) {}

    Iter(ContainerType &model, const SeqIterator& seq_iter, bool exists = false):
        model_(model), tree_iter_(model.tree()), seq_iter_(seq_iter), exists_(exists) {}

    //We have no move constructors for iterator

    MyType* me() {
        return this;
    }

    const MyType* me() const {
        return this;
    }

    MyType& self() {
        return *this;
    }

    const MyType& self() const {
        return *this;
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
            tree_iter_ = std::move(other.tree_iter_);
            seq_iter_ = std::move(other.seq_iter_);

            Base::assign(other);
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            tree_iter_ = other.tree_iter_;
            seq_iter_ = other.seq_iter_;

            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(SeqIterator&& seq_iter)
    {
        seq_iter_ = seq_iter;
        return *this;
    }

    MyType& operator=(TreeIterator&& tree_iter)
    {
        tree_iter_ = tree_iter;
        return *this;
    }

    TreeIterator& tree_iter()
    {
        return tree_iter_;
    }

    const TreeIterator& tree_iter() const
    {
        return tree_iter_;
    }

    SeqIterator& seq_iter()
    {
        return seq_iter_;
    }

    const SeqIterator& seq_iter() const
    {
        return seq_iter_;
    }

    bool exists() const
    {
        return exists_;
    }


    bool operator==(const MyType& other) const
    {
        return iter_equals(other);
    }

    bool iter_equals(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return seq_iter_ == other.seq_iter_ && tree_iter_ == other.tree_iter_ && Base::iter_equals(other);
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
            return seq_iter_ != other.seq_iter_ || tree_iter_ != other.tree_iter_ || Base::iter_not_equals(other);
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

    bool iter_is_end() const {
        return seq_iter_.iter_is_end();
    }

    bool iter_is_not_end() const {
        return seq_iter_.iter_is_not_end();
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
bool operator==(const Iter<WTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.iter_is_end();
}

template <typename Types>
bool operator!=(const Iter<WTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.iter_is_not_end();
}

}}