
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_ITREE_MACROS_HPP
#define	_MEMORIA_PROTOTYPES_ITREE_MACROS_HPP

#include <memoria/prototypes/btree/iterator/base.hpp>

#define MEMORIA_ITREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(IteratorBaseClassName)			\
template <                                                                      		\
        typename TypesType																\
>                                                                               		\
class IteratorBaseClassName: public memoria::BTreeIteratorBase<                   		\
        TypesType>                                                    					\
{																						\
	typedef memoria::BTreeIteratorBase<TypesType> 								Base;	\
	typedef typename Base::MyType												MyType; \
    MyType&             me_;


        
#define MEMORIA_ITREE_ITERATOR_BASE_CLASS_BEGIN(IteratorBaseClassName)      			\
MEMORIA_ITREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(IteratorBaseClassName)      			\
    IteratorBaseClassName(MyType &me): Base(me), me_(me) {}


#define MEMORIA_ITREE_ITERATOR_BASE_CLASS_END };

#endif
