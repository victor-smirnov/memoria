
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

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/types/typehash.hpp>

#include <memoria/api/common/ctr_api_btss.hpp>

#include <memoria/core/datatypes/io_vector_traits.hpp>

namespace memoria {

template <typename T>
class Vector {
    T element_;
public:
    using Element = T;

    Vector(): element_() {}
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
            ICtrApiSubstream<Value, io::ColumnWise>
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

    static constexpr bool HasTypeConstructors = false;
    static constexpr bool isSdnDeserializable = false;

    static void create_signature(SBuf& buf, const Vector<T>& obj)
    {
        buf << "Vector<";
        DataTypeTraits<T>::create_signature(buf, obj.element());
        buf << ">";
    }

    static void create_signature(SBuf& buf)
    {
        buf << "Vector<";
        DataTypeTraits<T>::create_signature(buf);
        buf << ">";
    }
};

}
