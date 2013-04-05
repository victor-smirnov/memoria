
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_MACROS1_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_MACROS1_HPP

#include <memoria/core/container/macros.hpp>

#define MEMORIA_BALTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeContainerBaseClassName)     \
template <                                                                              \
        typename TypesType                                                              \
>                                                                                       \
class BTreeContainerBaseClassName: public ContainerBase<TypesType> {                    \
                                                                                        \
    typedef BTreeContainerBaseClassName<TypesType>                              ThisType;\
    typedef ContainerBase<TypesType>                                            Base;   \
    typedef Ctr<TypesType>                                                      MyType; \
                                                                                        \
public:

#define MEMORIA_BALTREE_MODEL_BASE_CLASS_BEGIN(BTreeContainerBaseClassName)             \
    MEMORIA_BALTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeContainerBaseClassName)         \
                                                                                        \
    BTreeContainerBaseClassName(const CtrInitData& data): Base(data) {}                 \
    BTreeContainerBaseClassName(const ThisType& other): Base(other) {}                  \
    BTreeContainerBaseClassName(ThisType&& other): Base(std::move(other)) {}            \
    BTreeContainerBaseClassName(ThisType&& other, typename TypesType::Allocator* allocator): \
        Base(std::move(other), allocator)  {}                                           \
    BTreeContainerBaseClassName(const ThisType& other, typename TypesType::Allocator* allocator): \
        Base(other, allocator)        {}                                                \


#define MEMORIA_BALTREE_MODEL_BASE_CLASS_END                                            \
    MyType* me() {                                                                      \
        return static_cast<MyType*>(this);                                              \
    }                                                                                   \
    const MyType* me() const {                                                          \
        return static_cast<const MyType*>(this);                                        \
    }                                                                                   \
    																					\
    MyType& self() {                                                                    \
        return *static_cast<MyType*>(this);                                             \
    }                                                                                   \
    const MyType& self() const {                                                        \
        return *static_cast<const MyType*>(this);                                       \
    }                                                                                   \
};



#define MEMORIA_BALTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)     \
template <                                                                              \
        typename TypesType                                                              \
>                                                                                       \
class BTreeIteratorBaseClassName: public IteratorBase<TypesType>                        \
{                                                                                       \
    typedef IteratorBase<TypesType>                                             Base;   \
    typedef BTreeIteratorBaseClassName<TypesType>                               ThisType;\
public:                                                                                 \
    typedef Iter<TypesType>                                                     MyType;



        
#define MEMORIA_BALTREE_ITERATOR_BASE_CLASS_BEGIN(BTreeIteratorBaseClassName)      		\
MEMORIA_BALTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)      		\
    BTreeIteratorBaseClassName(): Base() {}


#define MEMORIA_BALTREE_ITERATOR_BASE_CLASS_END                                         \
    MyType* me() {                                                                      \
        return static_cast<MyType*>(this);                                              \
    }                                                                                   \
    const MyType* me() const {                                                          \
        return static_cast<const MyType*>(this);                                        \
    }                                                                                   \
    																					\
    MyType& self() {                                                                    \
        return *static_cast<MyType*>(this);                                             \
    }                                                                                   \
                                                                                        \
    const MyType& self() const {                                                        \
        return *static_cast<const MyType*>(this);                                       \
    }                                                                                   \
};



#define MEMORIA_FN_WRAPPER(WrapperName, TargetMethod) 	\
struct WrapperName {									\
	MyType* me_;										\
	WrapperName(MyType* v): me_(v) {}					\
	template <typename T, typename... Args>				\
	void operator()(T arg, Args... args) 				\
	{													\
		me_->TargetMethod(arg, args...);				\
	}													\
}

#define MEMORIA_FN_WRAPPER_RTN(WrapperName, TargetMethod, ReturnType_)\
struct WrapperName {									\
	typedef ReturnType_	ReturnType;						\
	MyType* me_;										\
	WrapperName(MyType* v): me_(v) {}					\
	template <typename T, typename... Args>				\
	ReturnType operator()(T arg, Args... args) 			\
	{													\
		return me_->TargetMethod(arg, args...);			\
	}													\
}

#define MEMORIA_CONST_FN_WRAPPER(WrapperName, TargetMethod) \
struct WrapperName {										\
	const MyType* me_;										\
	WrapperName(const MyType* v): me_(v) {}					\
	template <typename T, typename... Args>					\
	void operator()(T arg, Args... args) const				\
	{														\
		me_->TargetMethod(arg, args...);					\
	}														\
}

#define MEMORIA_CONST_FN_WRAPPER_RTN(WrapperName, TargetMethod, ReturnType_)\
struct WrapperName {									\
	typedef ReturnType_	ReturnType;						\
	const MyType* me_;									\
	WrapperName(const MyType* v): me_(v) {}				\
	template <typename T, typename... Args>				\
	ReturnType operator()(T arg, Args... args) const	\
	{													\
		return me_->TargetMethod(arg, args...);			\
	}													\
}




#define MEMORIA_CONST_STATIC_FN_WRAPPER_RTN(WrapperName, TargetMethod, ReturnType_)\
struct WrapperName {									\
	typedef ReturnType_	ReturnType;						\
	const MyType* me_;									\
	WrapperName(const MyType* v): me_(v) {}				\
	template <typename T, typename... Args>				\
	ReturnType operator()(Args... args) const	\
	{													\
		return me_->template TargetMethod<T>(args...);	\
	}													\
}


#define MEMORIA_DECLARE_NODE_FN(WrapperName, NodeMethodName)\
struct WrapperName {										\
	template <typename T, typename... Args>					\
	void operator()(T node, Args... args) const				\
	{														\
		node->NodeMethodName(args...);						\
	}														\
}

#define MEMORIA_DECLARE_NODE2_FN(WrapperName, NodeMethodName)\
struct WrapperName {										\
	template <typename T, typename... Args>					\
	void operator()(T node1, T node2, Args... args) const	\
	{														\
		node1->NodeMethodName(node2, args...);				\
	}														\
}

#define MEMORIA_DECLARE_NODE_FN_RTN(WrapperName, NodeMethodName, ReturnType_) \
struct WrapperName {										\
	typedef ReturnType_ ReturnType;							\
	template <typename T, typename... Args>					\
	ReturnType operator()(T node, Args... args) const		\
	{														\
		return node->NodeMethodName(args...);				\
	}														\
}

#define MEMORIA_DECLARE_NODE2_FN_RTN(WrapperName, NodeMethodName, ReturnType_) \
struct WrapperName {										\
	typedef ReturnType_ ReturnType;							\
	template <typename T, typename... Args>					\
	ReturnType operator()(T node1, T node2, Args... args) const \
	{														\
		return node1->NodeMethodName(node2, args...);		\
	}														\
}



#endif
