
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

#include <memoria/v1/core/config.hpp>

#define MEMORIA_V1_CONTAINER_PART_NO_CTR_BEGIN(PartName)                        \
template <typename Base1, typename TypesType>                                   \
class CtrPart<PartName, Base1, TypesType>: public Base1 {                       \
    typedef Base1 Base;                                                         \
    using ThisType = CtrPart<PartName, Base1, TypesType>;                       \
    typedef Ctr<TypesType> MyType;                                              \
    template <typename, typename, typename> friend class CtrPart1;               \
    template <typename, typename, typename> friend class IterPart;              \
    template <typename> class BTreeCtrBase;										\
public:


#define MEMORIA_V1_CONTAINER_PART_BEGIN(PartName)                               \
    MEMORIA_V1_CONTAINER_PART_NO_CTR_BEGIN(PartName)                            \
public:                                                                         \
    CtrPart(                                                                    \
        const CtrInitData& data,                                                \
        const CtrSharedPtr<typename TypesType::Allocator>& allocator):          \
        Base(data, allocator)                                                   \
    {}                                                                          \
    virtual ~CtrPart() noexcept {}                                              \
public:


#define MEMORIA_V1_CONTAINER_PART_END                                           \
    private:                                                                    \
    MyType& self() {                                                            \
        return *static_cast<MyType*>(this);                                     \
    }                                                                           \
                                                                                \
    const MyType& self() const {                                                \
        return *static_cast<const MyType*>(this);                               \
    }                                                                           \
};






#define MEMORIA_V1_CONTAINER_TYPE(PartName)                                     \
CtrPart<PartName, Base, Types>

#define MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS                                    \
template <typename Base, typename Types>



#define MEMORIA_V1_ITERATOR_PART_NO_CTOR_BEGIN(PartName)                        \
template <typename Base1, typename Types1>                                      \
class IterPart<PartName, Base1, Types1>: public Base1 {                         \
    typedef IterPart<PartName, Base1, Types1> ThisPartType;                     \
    typedef Base1 Base;                                                         \
    typedef Iter<Types1> MyType;                                                \
    using Types = Types1;														\
                                                                                \
    template <typename, typename, typename> friend class CtrPart1;               \
    template <typename, typename, typename> friend class IterPart;              \
    template <typename> friend class BTIteratorBase;                            \
public:



#define MEMORIA_V1_ITERATOR_PART_BEGIN(PartName)                                \
    MEMORIA_V1_ITERATOR_PART_NO_CTOR_BEGIN(PartName)                            \
public:                                                                         \
    IterPart(): Base() {}                                                       \
    IterPart(ThisPartType&& other): Base(std::move(other)) {}                   \
    IterPart(const ThisPartType& other): Base(other) {}                         \
    virtual ~IterPart() noexcept {}                                              \
public:




#define MEMORIA_V1_ITERATOR_PART_END                                            \
    MyType& self() {                                                            \
        return *static_cast<MyType*>(this);                                     \
    }                                                                           \
                                                                                \
    const MyType& self() const {                                                \
        return *static_cast<const MyType*>(this);                               \
    }                                                                           \
};


#define MEMORIA_V1_ITERATOR_TYPE(PartName)                                      \
IterPart<PartName, Base, Types>

#define MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS                                     \
template <typename Base, typename Types>

