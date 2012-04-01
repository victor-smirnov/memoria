
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_H



#include <memoria/prototypes/btree/iterator/api.hpp>
#include <memoria/prototypes/btree/iterator/base.hpp>
#include <memoria/prototypes/btree/names.hpp>

namespace memoria    {

template <typename Types> struct IterTypesT;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;


template <typename Types>
struct BTreeIterTypes: IterTypesT<Types> {};





template<
        typename Types
>
class Iter<BTreeIterTypes<Types> >: public IterStart<BTreeIterTypes<Types> >
{
    typedef Iter<BTreeIterTypes<Types> >                         					MyType;
    typedef IterStart<BTreeIterTypes<Types> >     									Base;
    typedef Ctr<typename Types::CtrTypes>                                       	ContainerType;
    typedef EmptyType																Txn;

    typedef typename ContainerType::Types::NodeBase                                 NodeBase;
    typedef typename ContainerType::Types::NodeBaseG                                NodeBaseG;

    ContainerType&      model_;

public:

    enum {GENERIC_ITERATOR, BEGIN_ITERATOR, END_ITERATOR, REVERSE_BEGIN_ITERATOR, REVERSE_END_ITERATOR};

    typedef ContainerType                                                           Container;
    
    Iter(Container &model, Int levels = 0): Base(), model_(model)
    {
    	Base::key_idx() 	= 0;

        Base::path().Resize(levels);
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
    		Base::Assign(std::move(other));
    	}

    	return *this;
    }

    MyType& operator=(const MyType& other)
    {
    	if (this != &other)
    	{
    		Base::Assign(other);
    	}

    	return *this;
    }

    bool operator==(const MyType& other) const
	{
    	return IsEqual(other);
	}

    bool IsEqual(const MyType& other) const
    {
    	if (other.type() == Base::NORMAL)
    	{
    		return Base::IsEqual(other);
    	}
    	else if (other.type() == Base::END)
    	{
    		return Base::IsEnd();
    	}
    	else if (other.type() == Base::START)
    	{
    		return Base::IsBegin();
    	}
    	else
    	{
    		return Base::IsEmpty();
    	}
    }

    bool operator!=(const MyType& other) const
    {
    	return IsNotEqual(other);
    }

    bool IsNotEqual(const MyType& other) const
    {
    	if (other.type() == Base::NORMAL)
    	{
    		return Base::IsNotEqual(other);
    	}
    	else if (other.type() == Base::END)
    	{
    		return Base::IsNotEnd();
    	}
    	else if (other.type() == Base::START)
    	{
    		return !Base::IsBegin();
    	}
    	else
    	{
    		return !Base::IsEmpty();
    	}
    }

    template <typename T>
    MyType& operator=(const T& value)
    {
    	this->SetData(value);
    	return *this;
    }
};

template <typename Types>
bool operator==(const Iter<BTreeIterTypes<Types> >& iter, const IterEndMark& mark)
{
	return iter.IsEnd();
}

template <typename Types>
bool operator!=(const Iter<BTreeIterTypes<Types> >& iter, const IterEndMark& mark)
{
	return iter.IsNotEnd();
}


}

#endif
