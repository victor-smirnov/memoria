
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_ITERATOR_HPP_
#define MEMORIA_CONTAINERS_DBLMAP2_ITERATOR_HPP_


#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <memoria/core/container/container.hpp>

namespace memoria {

template <typename Types>
class Iter<DblMap2IterTypes<Types> >: public IterStart<DblMap2IterTypes<Types> >
{
    typedef IterStart<DblMap2IterTypes<Types> >                                 Base;
    typedef Iter<DblMap2IterTypes<Types> >                                      MyType;
    typedef Ctr<DblMap2CtrTypes<Types> >                                        ContainerType;

    typedef typename ContainerType::OuterMap::Iterator                          OuterMapIterator;
    typedef typename ContainerType::InnerMap::Iterator                          InnerMapIterator;

    typedef typename Types::Profile                                             Profile;
    typedef typename Types::Allocator                                           Allocator;
    typedef typename Types::Allocator::CtrShared                                CtrShared;
    typedef typename Types::Allocator::PageG                                    PageG;
    typedef typename PageG::Page::ID                                            ID;


    ContainerType&      model_;

    OuterMapIterator    outer_iter_;
    InnerMapIterator    inner_iter_;
    bool                exists_;

public:

    Iter(ContainerType &model):
        model_(model), outer_iter_(model.tree()), inner_iter_(model.seq()), exists_(false) {}

    Iter(const MyType& other):
        model_(other.model_), outer_iter_(other.outer_iter_), inner_iter_(other.inner_iter_), exists_(other.exists_) {}

    Iter(ContainerType &model, const OuterMapIterator& outer_iter, const InnerMapIterator& inner_iter, bool exists = false):
        model_(model), outer_iter_(outer_iter), inner_iter_(inner_iter), exists_(exists) {}

    Iter(ContainerType &model, const OuterMapIterator& outer_iter, bool exists = false):
        model_(model), outer_iter_(outer_iter), inner_iter_(model.seq()), exists_(exists) {}

    Iter(ContainerType &model, const InnerMapIterator& inner_iter, bool exists = false):
        model_(model), outer_iter_(model.tree()), inner_iter_(inner_iter), exists_(exists) {}

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
            outer_iter_ = std::move(other.outer_iter_);
            inner_iter_ = std::move(other.inner_iter_);

            Base::assign(other);
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            outer_iter_ = other.outer_iter_;
            inner_iter_ = other.inner_iter_;

            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(InnerMapIterator&& inner_iter)
    {
        inner_iter_ = inner_iter;
        return *this;
    }

    MyType& operator=(OuterMapIterator&& outer_iter)
    {
        outer_iter_ = outer_iter;
        return *this;
    }

    OuterMapIterator& outer_iter()
    {
        return outer_iter_;
    }

    const OuterMapIterator& outer_iter() const
    {
        return outer_iter_;
    }

    InnerMapIterator& inner_iter()
    {
        return inner_iter_;
    }

    const InnerMapIterator& inner_iter() const
    {
        return inner_iter_;
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
            return inner_iter_ == other.inner_iter_ && outer_iter_ == other.outer_iter_ && Base::isEqual(other);
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
            return inner_iter_ != other.inner_iter_ || outer_iter_ != other.outer_iter_ || Base::isNotEqual(other);
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
        return outer_iter_.isEnd();
    }

    bool isNotEnd() const {
        return outer_iter_.isNotEnd();
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
bool operator==(const Iter<DblMap2IterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<DblMap2IterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}

}




#endif
