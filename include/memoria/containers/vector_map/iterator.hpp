
// Copyright Victor Smirnov, Ivan Yurchenko 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_BLOB_MAP_ITERATOR_ITERATOR_HPP
#define _MEMORIA_CONTAINERS_BLOB_MAP_ITERATOR_ITERATOR_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/vector_map/names.hpp>

namespace memoria {

using namespace memoria::vector_map;

template <typename Types>
class Iter<VectorMapIterTypes<Types> >: public IterStart<VectorMapIterTypes<Types> >
{
    typedef IterStart<VectorMapIterTypes<Types> >               Base;
    typedef Iter<VectorMapIterTypes<Types> >                    MyType;
    typedef Ctr<VectorMapCtrTypes<Types> >                      ContainerType;

    typedef typename ContainerType::IdxSet::Iterator            IdxSetIterator;
    typedef typename ContainerType::ByteArray::Iterator         ByteArrayIterator;

    typedef typename ContainerType::IdxSet::Accumulator         IdxsetAccumulator;

    typedef typename Types::Profile                             Profile;
    typedef typename Types::Allocator                           Allocator;
    typedef typename Types::Allocator::CtrShared                CtrShared;
    typedef typename Types::Allocator::PageG                    PageG;
    typedef typename Allocator::Page                            Page;
    typedef typename Page::ID                                   ID;

    typedef typename ContainerType::Key                         Key;
    typedef typename ContainerType::Value                       Value;

    ContainerType&      model_;

    ByteArrayIterator   ba_iter_;
    IdxSetIterator      is_iter_;
    bool                exists_;

public:

    Iter(ContainerType &model):
        model_(model), ba_iter_(model.array()), is_iter_(model.set()), exists_(false) {}

    Iter(const MyType& other):
        model_(other.model_), ba_iter_(other.ba_iter_), is_iter_(other.is_iter_), exists_(other.exists_) {}

    Iter(ContainerType &model, const IdxSetIterator& is_iter, const ByteArrayIterator& ba_iter, bool exists = false):
        model_(model), ba_iter_(ba_iter), is_iter_(is_iter), exists_(exists) {}

    Iter(ContainerType &model, const IdxSetIterator& is_iter, bool exists = false):
        model_(model), ba_iter_(model), is_iter_(is_iter), exists_(exists) {}

    Iter(ContainerType &model, const ByteArrayIterator& ba_iter, bool exists = false):
        model_(model), ba_iter_(ba_iter), is_iter_(model), exists_(exists) {}

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

    MyType& operator=(MyType&& other)
    {
        if (this != &other)
        {
            ba_iter_ = std::move(other.ba_iter_);
            is_iter_ = std::move(other.is_iter_);

            Base::assign(other);
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            ba_iter_ = other.ba_iter_;
            is_iter_ = other.is_iter_;

            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(IdxSetIterator&& is_iter)
    {
        is_iter_ = is_iter;
        return *this;
    }

    MyType& operator=(ByteArrayIterator&& ba_iter)
    {
        ba_iter_ = ba_iter;
        return *this;
    }

    ByteArrayIterator& ba_iter()
    {
        return ba_iter_;
    }

    const ByteArrayIterator& ba_iter() const
    {
        return ba_iter_;
    }

    IdxSetIterator& is_iter()
    {
        return is_iter_;
    }

    const IdxSetIterator& is_iter() const
    {
        return is_iter_;
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
            return is_iter_ == other.is_iter_ && ba_iter_ == other.ba_iter_ && Base::isEqual(other);
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
            return is_iter_ != other.is_iter_ || ba_iter_ != other.ba_iter_ || Base::isNotEqual(other);
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
        return is_iter_.isEnd();
    }

    bool isNotEnd() const {
        return is_iter_.isNotEnd();
    }

    MyType& operator=(const IData<Value>& value)
    {
    	this->setValue(value);
    	return *this;
    }

    template <typename T>
    MyType& operator=(const T& value)
    {
        this->setValue(value);
        return *this;
    }
};


template <typename Types>
bool operator==(const Iter<VectorMapIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<VectorMapIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}

}



#endif
