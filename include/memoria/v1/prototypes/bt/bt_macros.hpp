
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/container/macros.hpp>

#define MEMORIA_V1_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeCtrBaseClassName)             \
template <                                                                              \
        typename TypesType                                                              \
>                                                                                       \
class BTreeCtrBaseClassName: public CtrBase<TypesType> {                                \
                                                                                        \
    typedef BTreeCtrBaseClassName<TypesType>                              ThisType;     \
    typedef CtrBase<TypesType>                                            Base;         \
    typedef Ctr<TypesType>                                                MyType;       \
                                                                                        \
    template <typename, typename, typename > friend class CtrPart;                      \
public:

#define MEMORIA_V1_BT_MODEL_BASE_CLASS_BEGIN(BTreeCtrBaseClassName)                     \
    MEMORIA_V1_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(BTreeCtrBaseClassName)                 \
                                                                                        \
    BTreeCtrBaseClassName() {}




#define MEMORIA_V1_BT_MODEL_BASE_CLASS_END                                              \
private:                                                                                \
    MyType& self() {                                                                    \
        return *static_cast<MyType*>(this);                                             \
    }                                                                                   \
    const MyType& self() const {                                                        \
        return *static_cast<const MyType*>(this);                                       \
    }                                                                                   \
};



#define MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)     \
template <                                                                              \
        typename TypesType                                                              \
>                                                                                       \
class BTreeIteratorBaseClassName: public IteratorBase<TypesType>                        \
{                                                                                       \
    typedef IteratorBase<TypesType>                                             Base;   \
    typedef BTreeIteratorBaseClassName<TypesType>                               ThisType;\
    template <typename, typename, typename> friend class CtrPart1;                       \
    template <typename, typename, typename> friend class IterPart;                      \
public:                                                                                 \
    typedef Iter<TypesType>                                                     MyType;



        
#define MEMORIA_V1_BT_ITERATOR_BASE_CLASS_BEGIN(BTreeIteratorBaseClassName)             \
MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBaseClassName)             \
    BTreeIteratorBaseClassName(): Base() {}


#define MEMORIA_BT_ITERATOR_BASE_CLASS_END                                              \
    MyType& self() {                                                                    \
        return *static_cast<MyType*>(this);                                             \
    }                                                                                   \
                                                                                        \
    const MyType& self() const {                                                        \
        return *static_cast<const MyType*>(this);                                       \
    }                                                                                   \
};



#define MEMORIA_V1_FN_WRAPPER(WrapperName, TargetMethod)   \
struct WrapperName {                                    \
    MyType& me_;                                        \
    WrapperName(MyType& v): me_(v) {}                   \
    template <typename T, typename... Args>             \
    auto treeNode(T&& arg, Args&&... args)                \
    {                                                   \
        return me_.TargetMethod(std::forward<T>(arg), std::forward<Args>(args)...);\
    }                                                   \
}

#define MEMORIA_V1_FN_WRAPPER_RTN(WrapperName, TargetMethod, ReturnType_)\
struct WrapperName {                                    \
    typedef ReturnType_ ReturnType;                     \
    MyType& me_;                                        \
    WrapperName(MyType& v): me_(v) {}                   \
    template <typename T, typename... Args>             \
    ReturnType treeNode(T arg, Args&&... args)          \
    {                                                   \
        return me_.TargetMethod(arg, std::forward<Args>(args)...);\
    }                                                   \
}

#define MEMORIA_V1_CONST_FN_WRAPPER(WrapperName, TargetMethod) \
struct WrapperName {                                        \
    const MyType& me_;                                      \
    WrapperName(const MyType& v): me_(v) {}                 \
    template <typename T, typename... Args>                 \
    auto treeNode(T&& arg, Args&&... args) const              \
    {                                                       \
        return me_.TargetMethod(std::forward<T>(arg), std::forward<Args>(args)...);\
    }                                                       \
}

#define MEMORIA_V1_CONST_FN_WRAPPER_RTN(WrapperName, TargetMethod, ReturnType_)\
struct WrapperName {                                    \
    typedef ReturnType_ ReturnType;                     \
    const MyType& me_;                                  \
    WrapperName(const MyType& v): me_(v) {}             \
    template <typename T, typename... Args>             \
    ReturnType treeNode(T&& arg, Args&&... args) const    \
    {                                                   \
        return me_.TargetMethod(std::forward<T>(arg), std::forward<Args>(args)...);\
    }                                                   \
}




#define MEMORIA_V1_CONST_STATIC_FN_WRAPPER_RTN(WrapperName, TargetMethod, ReturnType_)\
struct WrapperName {                                    \
    typedef ReturnType_ ReturnType;                     \
    const MyType& me_;                                  \
    WrapperName(const MyType& v): me_(v) {}             \
    template <typename T, typename... Args>             \
    ReturnType treeNode(T&&, Args&&... args) const \
    {                                                   \
        return me_.template TargetMethod<std::decay_t<T>>(std::forward<Args>(args)...);\
    }                                                   \
}


#define MEMORIA_V1_DECLARE_NODE_FN(WrapperName, NodeMethodName)\
struct WrapperName {                                        \
    template <typename T, typename... Args>                 \
    auto treeNode(T&& node, Args&&... args) const           \
    {                                                       \
        return node.NodeMethodName(std::forward<Args>(args)...);   \
    }                                                       \
}

#define MEMORIA_V1_DECLARE_NODE2_FN(WrapperName, NodeMethodName)\
struct WrapperName {                                            \
    template <typename T, typename... Args>                     \
    auto treeNode(T&& node1, T&& node2, Args&&... args) const   \
    {                                                           \
        return node1.NodeMethodName(node2, std::forward<Args>(args)...);\
    }                                                           \
}

#define MEMORIA_V1_DECLARE_NODE_FN_RTN(WrapperName, NodeMethodName, ReturnType_) \
struct WrapperName {                                            \
    typedef ReturnType_ ReturnType;                             \
    template <typename T, typename... Args>                     \
    ReturnType treeNode(T&& node, Args&&... args) const         \
    {                                                           \
        return node.NodeMethodName(std::forward<Args>(args)...);\
    }                                                           \
}

#define MEMORIA_V1_DECLARE_NODE2_FN_RTN(WrapperName, NodeMethodName, ReturnType_) \
struct WrapperName {                                        \
    typedef ReturnType_ ReturnType;                         \
    template <typename T, typename... Args>                 \
    ReturnType treeNode(T&& node1, T&& node2, Args&&... args) const \
    {                                                       \
        return node1.NodeMethodName(node2, std::forward<Args>(args)...);\
    }                                                       \
}
