
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_H
#define __MEMORIA_PROTOTYPES_BALANCEDTREE_ITERATOR_H



#include <memoria/prototypes/bt/iterator/bt_i_api.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_base.hpp>
#include <memoria/prototypes/bt/iterator/bt_i_find.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>

namespace memoria    {

template <typename Types> struct IterTypesT;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;



template<
        typename Types
>
class Iter<BTIterTypes<Types>>: public IterStart<BTIterTypes<Types>>
{
    typedef Iter<BTIterTypes<Types>>                                           MyType;
    typedef IterStart<BTIterTypes<Types>>                                      Base;
    typedef Ctr<typename Types::CtrTypes>                                           ContainerType;
    typedef EmptyType                                                               Txn;

    typedef typename ContainerType::Types::NodeBase                                 NodeBase;
    typedef typename ContainerType::Types::NodeBaseG                                NodeBaseG;

    ContainerType&      model_;

public:

    enum {GENERIC_ITERATOR, BEGIN_ITERATOR, END_ITERATOR, REVERSE_BEGIN_ITERATOR, REVERSE_END_ITERATOR};

    typedef ContainerType                                                           Container;
    
    Iter(Container &model): Base(), model_(model)
    {
        Base::idx() = 0;
    }
    
    Iter(const MyType& other): Base(other), model_(other.model_) {}

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

    template <typename T>
    MyType& operator=(const T& value)
    {
        this->setData(value);
        return *this;
    }
};

template <typename Types>
bool operator==(const Iter<BTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<BTIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}


}

#endif