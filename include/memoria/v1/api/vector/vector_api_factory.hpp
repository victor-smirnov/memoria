
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

namespace memoria {
namespace v1 {

template <typename T>
class Vector {
    T element_;
public:
    Vector(T element):
        element_(element)
    {}

    const T& element() const {return element_;}
};

template <typename Value_, typename Profile>
struct ICtrApiTypes<Vector<Value_>, Profile> {

    using Value = Value_;

    using IOVSchema = TL<
        TL<
            ICtrApiSubstream<Value, io::ColumnWise1D>
        >
    >;
};


template <typename T>
struct TypeHash<Vector<T>>: UInt64Value<HashHelper<1300, TypeHashV<T>>> {};

template <typename T>
struct DataTypeTraits<Vector<T>>: DataTypeTraitsBase<Vector<T>> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    using Parameters = TL<T>;

    static constexpr size_t MemorySize        = sizeof(EmptyType);
    static constexpr bool IsParametrised      = true;
    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;

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
