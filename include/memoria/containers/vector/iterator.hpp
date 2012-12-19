
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_CONTAINERS_VECTOR_ITERATOR_H
#define __MEMORIA_CONTAINERS_VECTOR_ITERATOR_H



#include <memoria/prototypes/btree/iterator/api.hpp>
#include <memoria/prototypes/btree/iterator/base.hpp>

#include <memoria/containers/vector/names.hpp>

#include <vector>
#include <ostream>

namespace memoria {

using namespace std;




template <typename Types>
class Iter<VectorIterTypes<Types>>: public IterStart<VectorIterTypes<Types>>
{
    typedef Iter<VectorIterTypes<Types>>                                            MyType;
    typedef IterStart<VectorIterTypes<Types>>                                       Base;
    typedef Ctr<typename Types::CtrTypes>                                           ContainerType;
    typedef EmptyType                                                               Txn;

    typedef typename ContainerType::Types::NodeBase                                 NodeBase;
    typedef typename ContainerType::Types::NodeBaseG                                NodeBaseG;

    typedef typename ContainerType::Types::ElementType                              ElementType;

    ContainerType&      model_;

public:

    typedef ContainerType                                                           Container;

    Iter(Container &model, Int levels = 0): Base(), model_(model)
    {
        Base::key_idx()     = 0;

        Base::path().resize(levels);
    }

    Iter(const MyType& other): Base(other), model_(other.model_) {}

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
            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            Base::assign(other);
        }

        return *this;
    }

    template <typename T>
    MyType& operator=(const T& value)
    {
        AssignToItem(*this, value);
        return *this;
    }

    bool operator==(const MyType& other) const
    {
        return isEqual(other);
    }

    bool isEqual(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return Base::isEqual(other);
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
            return Base::isNotEqual(other);
        }
        else if (other.type() == Base::END)
        {
            return !Base::isEof();
        }
        else if (other.type() == Base::START)
        {
            return !Base::isBof();
        }
        else
        {
            return !Base::isEmpty();
        }
    }

    MyType& operator=(const ElementType& value)
    {
        this->assignElement(value);
        return *this;
    }

    MyType& operator*() {
        return *this;
    }

    const MyType& operator*() const {
        return *this;
    }
};


template <typename Types>
bool operator==(const Iter<VectorIterTypes<Types>>& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<VectorIterTypes<Types>>& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}







}

#endif
