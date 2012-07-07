
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
class BTreeContainerBaseClassName: public ContainerBase<TypesType> {                    \
                                                                                        \
    typedef BTreeContainerBaseClassName<TypesType>								ThisType;\
    typedef ContainerBase<TypesType>                							Base;   \
    typedef Ctr<TypesType>														MyType; \
                                                                                        \
public:

#define MEMORIA_BTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBaseClassName)               \
    MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeContainerBaseClassName)           \
                                                                                        \
    BTreeContainerBaseClassName(): Base() {}                       						\
    BTreeContainerBaseClassName(const ThisType& other): Base(other) {}                  \
    BTreeContainerBaseClassName(ThisType&& other): Base(std::move(other)) {}            \
    BTreeContainerBaseClassName(ThisType&& other, typename TypesType::Allocator* allocator): Base(std::move(other), allocator)  {} 	\
    BTreeContainerBaseClassName(const ThisType& other, typename TypesType::Allocator* allocator): Base(other, allocator)        {} 	\


#define MEMORIA_BTREE_MODEL_BASE_CLASS_END												\
	MyType* me() {																		\
    	return static_cast<MyType*>(this);												\
    }																					\
    																					\
    const MyType* me() const {															\
    	return static_cast<const MyType*>(this);										\
    }																					\
};



#define MEMORIA_BTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)		\
template <                                                                      		\
        typename TypesType																\
>                                                                               		\
class BTreeIteratorBaseClassName: public IteratorBase<TypesType>                        \
{																						\
	typedef IteratorBase<TypesType> 											Base;	\
	typedef BTreeIteratorBaseClassName<TypesType>								ThisType;\
public:																					\
	typedef Iter<TypesType>														MyType;



        
#define MEMORIA_BTREE_ITERATOR_BASE_CLASS_BEGIN(BTreeIteratorBaseClassName)      \
MEMORIA_BTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)      \
    BTreeIteratorBaseClassName(): Base() {}


#define MEMORIA_BTREE_ITERATOR_BASE_CLASS_END											\
	MyType* me() {																		\
    	return static_cast<MyType*>(this);												\
    }																					\
    																					\
    const MyType* me() const {															\
    	return static_cast<const MyType*>(this);										\
    }																					\
};

#endif
