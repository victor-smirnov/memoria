
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_H
#define __MEMORIA_PROTOTYPES_DYNVECTOR_ITERATOR_H



#include <memoria/prototypes/btree/iterator/api.hpp>
#include <memoria/prototypes/btree/iterator/base.hpp>

#include <memoria/prototypes/btree/names.hpp>

namespace memoria    {

// We don't use custom iterator for DynVector yet

//template <typename Types> struct IterTypesT;
//template <typename Types> class Iter;
//template <typename Name, typename Base, typename Types> class IterPart;
//
//
//template <typename Types>
//struct DynVectorIterTypes: IterTypesT<Types> {};
//
//
//
//
//
//template<
//        typename Types
//>
//class Iter<DynVectorIterTypes<Types> >: public IterStart<DynVectorIterTypes<Types> >
//{
//    typedef Iter<DynVectorIterTypes<Types> >                         				MyType;
//    typedef IterStart<DynVectorIterTypes<Types> >     								Base;
//    typedef Ctr<typename Types::CtrTypes>                                       	ContainerType;
//    typedef EmptyType																Txn;
//
//    typedef typename ContainerType::Types::NodeBase                                 NodeBase;
//    typedef typename ContainerType::Types::NodeBaseG                                NodeBaseG;
//    typedef typename ContainerType::Types::DataPageG                                DataPageG;
//
//    Int kind_;
//    ContainerType&      model_;
//
//public:
//
//    enum {GENERIC_ITERATOR, BEGIN_ITERATOR, END_ITERATOR, REVERSE_BEGIN_ITERATOR, REVERSE_END_ITERATOR};
//
//    typedef ContainerType                                                           Container;
//
//    Iter(Container &model, Int kind = GENERIC_ITERATOR):Base(), kind_(kind), model_(model)
//    {
//    	Base::SetupAllocator(&model.allocator());
//    	Base::state() 		= 0;
//        Base::key_idx() 	= -1;
//    }
//
//    Iter(NodeBaseG node, Int idx, Container &model, bool do_init = false): Base(), kind_(GENERIC_ITERATOR), model_(model)
//    {
//    	Base::SetupAllocator(&model.allocator());
//    	Base::page() 		= node;
//    	Base::data()		= model.GetValuePage(node, idx, ContainerType::Allocator::READ);
//    	Base::key_idx() 	= idx;
//    	Base::state() 		= 0;
//
//    	if (do_init) Base::Init();
//    	Base::ReHash();
//    }
//
//    Iter(NodeBaseG node, Int idx, DataPageG data, Int data_pos, Container &model, bool do_init = false): Base(), kind_(GENERIC_ITERATOR), model_(model)
//    {
//    	Base::SetupAllocator(&model.allocator());
//    	Base::page() 		= node;
//    	Base::data() 		= data;
//    	Base::data_pos()	= data_pos;
//
//    	Base::key_idx() 	= idx;
//    	Base::state() 		= 0;
//
//    	if (do_init) Base::Init();
//    	Base::ReHash();
//    }
//
//    Iter(const MyType& other): Base(other), kind_(other.kind_), model_(other.model_) {}
//
//    ContainerType& model() {
//    	return model_;
//    }
//
//    const ContainerType& model() const {
//    	return model_;
//    }
//
//    MyType& operator=(MyType&& other)
//    {
//    	Base::operator=(std::move(other));
//    	Base::ReHash();
//    	return *this;
//    }
//
//    MyType& operator=(const MyType& other)
//    {
//    	Base::operator=(other);
//    	Base::ReHash();
//    	return *this;
//    }
//
//    const Int kind() const {
//    	return kind_;
//    }
//
//    void update() {
//    	switch (kind_) {
//    		case BEGIN_ITERATOR: 	(*this) = model().FindStart(); break;
//    		case END_ITERATOR: 		(*this) = model().FindEnd(); break;
//
//    		case REVERSE_BEGIN_ITERATOR: 	(*this) = model().FindRStart(); break;
//    		case REVERSE_END_ITERATOR: 		(*this) = model().FindREnd(); break;
//
//    		default:; // do nothing
//    	};
//    }
//
//    bool operator==(const MyType& other) const
//    {
//    	if (Base::hash() == other.hash())
//    	{
//    		return Base::operator==(other);
//    	}
//    	else {
//    		return false;
//    	}
//    }
//
//    bool operator!=(const MyType& other) const {
//    	return !operator==(other);
//    }
//
//};

}

#endif
