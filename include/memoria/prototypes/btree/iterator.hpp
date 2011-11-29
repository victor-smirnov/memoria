
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_H



#include <memoria/prototypes/btree/iterator/api.hpp>
#include <memoria/prototypes/btree/iterator/multiskip.hpp>
#include <memoria/prototypes/btree/iterator/model_api.hpp>
#include <memoria/prototypes/btree/iterator/walk.hpp>
#include <memoria/prototypes/btree/iterator/tools.hpp>

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

    Int kind_;
    ContainerType&      model_;

public:

    enum {GENERIC_ITERATOR, BEGIN_ITERATOR, END_ITERATOR, REVERSE_BEGIN_ITERATOR, REVERSE_END_ITERATOR};

    typedef ContainerType                                                           Container;
    
    Iter(Container &model, Int kind = GENERIC_ITERATOR):Base(*this), kind_(kind), model_(model) {
        Base::state() = 0;
        Base::page() = NULL;
        Base::key_idx() = -1;
    }
    
//    Iter(NodeBase* node, Int idx, Container &model, bool do_init = false): Base(*this), kind_(GENERIC_ITERATOR), model_(model) {
//        Base::page() = node;
//        Base::key_idx() = idx;
//        Base::state() = 0;
//        if (do_init) Base::Init();
//        Base::ReHash();
//    }

    Iter(NodeBaseG node, Int idx, Container &model, bool do_init = false): Base(*this), kind_(GENERIC_ITERATOR), model_(model) {
    	Base::page() = node;
    	Base::key_idx() = idx;
    	Base::state() = 0;
    	if (do_init) Base::Init();
    	Base::ReHash();
    }

    Iter(const MyType& other): Base(*this), kind_(other.kind_), model_(other.model_) {
        setup(other);
    }

    ContainerType& model() {
    	return model_;
    }

    const ContainerType& model() const {
    	return model_;
    }

    const MyType& operator=(const MyType& other)
    {
    	setup(other);
    	Base::ReHash();
    	return *this;
    }

    const Int kind() const {
    	return kind_;
    }

    void update() {
    	switch (kind_) {
    		case BEGIN_ITERATOR: 	(*this) = model().FindStart(); break;
    		case END_ITERATOR: 		(*this) = model().FindEnd(); break;

    		case REVERSE_BEGIN_ITERATOR: 	(*this) = model().FindRStart(); break;
    		case REVERSE_END_ITERATOR: 		(*this) = model().FindREnd(); break;

    		default:; // do nothing
    	};
    }

    void setup(const MyType &other) {
    	Base::setup(other);
    }

    bool operator==(const MyType& other) const
    {
    	if (Base::hash() == other.hash())
    	{
    		return Base::operator==(other);
    	}
    	else {
    		return false;
    	}
    }

    bool operator!=(const MyType& other) const {
    	return !operator==(other);
    }

};

}

#endif
