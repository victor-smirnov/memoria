
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_SUMTREE_MACROS_HPP
#define	_MEMORIA_PROTOTYPES_SUMTREE_MACROS_HPP

#include <memoria/prototypes/btree/iterator/base.hpp>

#define MEMORIA_BSTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(IteratorBaseClassName)			\
template <                                                                      		\
        typename TypesType																\
>                                                                               		\
class IteratorBaseClassName: public memoria::BTreeIteratorBase<                   		\
        TypesType>                                                    					\
{																						\
	typedef IteratorBaseClassName<TypesType>									ThisType;\
	typedef memoria::BTreeIteratorBase<TypesType> 								Base;	\
	typedef typename Base::MyType												MyType;


        
#define MEMORIA_BSTREE_ITERATOR_BASE_CLASS_BEGIN(IteratorBaseClassName)      			\
MEMORIA_BSTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(IteratorBaseClassName)      			\
public:																					\
    IteratorBaseClassName(): Base() {}													\
    ITreeIteratorBase(const ThisType& other): Base(other) {}							\
    IteratorBaseClassName(ThisType&& other): Base(std::move(other)) {}					\



#define MEMORIA_BSTREE_ITERATOR_BASE_CLASS_END									\
	MyType* me() {																\
    	return static_cast<MyType*>(this);										\
    }																			\
    																			\
    const MyType* me() const {													\
    	return static_cast<const MyType*>(this);								\
    }																			\
};


#endif
