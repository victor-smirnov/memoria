
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_MACROS_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_MACROS_HPP

#define MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeContainerBaseClassName)       \
template <                                                                              \
        typename TypesType																\
>                                                         								\
class BTreeContainerBaseClassName: public ContainerBase<                                \
                                        TypesType> {                            		\
                                                                                        \
    typedef ContainerBase<TypesType>                							Base;   \
    typedef Ctr<TypesType>														MyType; \
                                                                                        \
    MyType& me_;                                     									\
                                                                                        \
public:

#define MEMORIA_BTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBaseClassName)               \
    MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeContainerBaseClassName)           \
                                                                                        \
    BTreeContainerBaseClassName(MyType &me): Base(me), me_(me) {}                       \
                                                                                        \
    MyType &me() {                                                                      \
        return me_;                                                                     \
    }                                                                                   \
                                                                                        \
    const MyType &me() const {                                                          \
        return me_;                                                                     \
    }


#define MEMORIA_BTREE_MODEL_BASE_CLASS_END };




#define MEMORIA_BTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)		\
template <                                                                      		\
        typename TypesType																\
>                                                                               		\
class BTreeIteratorBaseClassName: public IteratorBase<                   				\
        TypesType>                                                    					\
{																						\
	typedef IteratorBase<TypesType> 											Base;	\
public:																					\
	typedef Iter<TypesType>														MyType; \
    MyType&             me_;


        
#define MEMORIA_BTREE_ITERATOR_BASE_CLASS_BEGIN(BTreeIteratorBaseClassName)      \
MEMORIA_BTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)      \
    BTreeIteratorBaseClassName(MyType &me): Base(me), me_(me) {}


#define MEMORIA_BTREE_ITERATOR_BASE_CLASS_END };

#endif
