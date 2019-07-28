
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/vector/vector_input.hpp>

#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>

#include <memory>
#include <vector>

namespace memoria {
namespace v1 {

namespace detail01 {
    template <typename V>
    struct VectorValueHelper: HasType<V> {};
    
    template <Granularity G, typename V>
    struct VectorValueHelper<VLen<G, V>>: HasType<V> {};
}

template <typename T>
class Vector {
    T element_;
public:
    Vector(T element):
        element_(element)
    {}

    const T& element() const {return element_;}
};
    
template <typename Value, typename Profile> 
class CtrApi<Vector<Value>, Profile>: public CtrApiBTSSBase<Vector<Value>, Profile>  {
    using Base = CtrApiBTSSBase<Vector<Value>, Profile>;
public:    
    using typename Base::CtrID;
    using typename Base::AllocatorT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    

    using DataValue = typename detail01::VectorValueHelper<Value>::Type;
    
    MMA1_DECLARE_CTRAPI_BASIC_METHODS()
};


template <typename Value, typename Profile> 
class IterApi<Vector<Value>, Profile>: public IterApiBTSSBase<Vector<Value>, Profile> {
    
    using Base = IterApiBTSSBase<Vector<Value>, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    
    using DataValue = typename detail01::VectorValueHelper<Value>::Type;

    using Base::insert;
    
    MMA1_DECLARE_ITERAPI_BASIC_METHODS()
    
    DataValue value();
    
    std::vector<DataValue> read(size_t size);

    void insert(Span<const DataValue> span)
    {
        io::VectorIOVector<DataValue> iov(span.data(), span.size());

        this->insert(iov);
    }
};
    

template <typename T>
struct TypeHash<Vector<T>>: UInt64Value<HashHelper<1300, TypeHashV<T>>> {};

template <typename T>
struct DataTypeTraits<Vector<T>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<T>;

    static constexpr size_t MemorySize        = sizeof(EmptyType);
    static constexpr bool IsParametrised      = true;
    static constexpr bool HasTypeConstructors = false;

    static void create_signature(SBuf& buf, const Vector<T>& obj)
    {
        buf << "Vector<";
        DataTypeTraits<T>::create_signature(buf, obj.key());
        buf << ">";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Vector<";
        DataTypeTraits<T>::create_signature(buf);
        buf << ">";
    }
};

}}
