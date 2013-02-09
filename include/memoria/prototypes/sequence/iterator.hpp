
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SEQUENCE_ITERATOR_HPP
#define _MEMORIA_PROTOTYPES_SEQUENCE_ITERATOR_HPP



#include <memoria/prototypes/btree/iterator/btree_i_api.hpp>
#include <memoria/prototypes/btree/iterator/btree_i_base.hpp>

#include <memoria/prototypes/sequence/names.hpp>

#include <vector>
#include <ostream>

namespace memoria {

using namespace std;




template <typename Types>
class Iter<SequenceIterTypes<Types>>: public IterStart<SequenceIterTypes<Types>>
{
    typedef Iter<SequenceIterTypes<Types>>                                          MyType;
    typedef IterStart<SequenceIterTypes<Types>>                                     Base;
    typedef Ctr<typename Types::CtrTypes>                                           ContainerType;
    typedef EmptyType                                                               Txn;

    typedef typename ContainerType::Types::NodeBase                                 NodeBase;
    typedef typename ContainerType::Types::NodeBaseG                                NodeBaseG;

    ContainerType&      model_;

public:

    typedef typename ContainerType::Types::ElementType                              ElementType;

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
bool operator==(const Iter<SequenceIterTypes<Types>>& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<SequenceIterTypes<Types>>& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}







}

#endif
