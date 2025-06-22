
// Copyright 2011-2025 Victor Smirnov
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

#include <memoria/core/tools/md5.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/integer/accumulator_common.hpp>

#include <string.h>
#include <cstring>
#include <tuple>

namespace memoria {


template <typename T> struct FieldFactory;


template <typename Type>
struct CompositeFieldFactory {

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        field.serialize(data);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            field[c].serialize(data);
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        field.deserialize(data);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            field[c].deserialize(data);
        }
    }
};


template <typename Type>
struct FieldFactoryBase {
    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field) {
        std::memcpy(data.buf, &field, sizeof(Type));
        data.buf += sizeof(Type);
        data.total += sizeof(Type);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field) {
        std::memcpy(&field, data.buf, sizeof(Type));
        data.buf += sizeof(Type);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field, int32_t count) {
        std::memcpy(data.buf, &field, count*sizeof(Type));
        data.buf += count * sizeof(Type);
        data.total += count * sizeof(Type);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field, int32_t count) {
        std::memcpy(&field, data.buf, count*sizeof(Type));
        data.buf += count*sizeof(Type);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t count) {
        std::memcpy(data.buf, field, count*sizeof(Type));
        data.buf += count * sizeof(Type);
        data.total += count * sizeof(Type);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t count) {
        std::memcpy(field, data.buf, count*sizeof(Type));
        data.buf += count*sizeof(Type);
    }
};


template <> struct FieldFactory<char>: FieldFactoryBase<char> {};
template <> struct FieldFactory<int8_t>: FieldFactoryBase<int8_t> {};
template <> struct FieldFactory<int16_t>: FieldFactoryBase<int16_t> {};
template <> struct FieldFactory<int32_t>: FieldFactoryBase<int32_t> {};
template <> struct FieldFactory<int64_t>: FieldFactoryBase<int64_t> {};
template <> struct FieldFactory<uint8_t>: FieldFactoryBase<uint8_t> {};
template <> struct FieldFactory<uint16_t>: FieldFactoryBase<uint16_t> {};
template <> struct FieldFactory<uint32_t>: FieldFactoryBase<uint32_t> {};
template <> struct FieldFactory<uint64_t>: FieldFactoryBase<uint64_t> {};
template <> struct FieldFactory<UInt128T>: FieldFactoryBase<UInt128T> {};
template <> struct FieldFactory<Int128T>: FieldFactoryBase<Int128T> {};
template <> struct FieldFactory<UAcc64T>: FieldFactoryBase<UAcc64T> {};
template <> struct FieldFactory<UAcc128T>: FieldFactoryBase<UAcc128T> {};
template <> struct FieldFactory<UAcc192T>: FieldFactoryBase<UAcc192T> {};
template <> struct FieldFactory<UAcc256T>: FieldFactoryBase<UAcc256T> {};
template <> struct FieldFactory<float>: FieldFactoryBase<float> {};
template <> struct FieldFactory<double>: FieldFactoryBase<double> {};
template <> struct FieldFactory<bool>: FieldFactoryBase<bool> {};



namespace internal {

template <typename Tuple, int32_t Idx = std::tuple_size<Tuple>::value - 1>
struct TupleFactoryHelper {

    using CurrentType = typename std::tuple_element<Idx, Tuple>::type;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Tuple& field)
    {
        FieldFactory<CurrentType>::serialize(data, std::get<Idx>(field));

        TupleFactoryHelper<Tuple, Idx - 1>::serialize(data, field);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Tuple& field)
    {
        FieldFactory<CurrentType>::deserialize(data, std::get<Idx>(field));

        TupleFactoryHelper<Tuple, Idx - 1>::deserialize(data, field);
    }
};

template <typename Tuple>
struct TupleFactoryHelper<Tuple, -1> {
    template <typename SerializationData>
    static void serialize(SerializationData& data, const Tuple& field) {}

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Tuple& field) {}
};

}


template <typename... Types>
struct FieldFactory<std::tuple<Types...> > {

    using Type = std::tuple<Types...>;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        internal::TupleFactoryHelper<Type>::serialize(data, field);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            internal::TupleFactoryHelper<Type>::serialize(data, field[c]);
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        internal::TupleFactoryHelper<Type>::deserialize(data, field);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            internal::TupleFactoryHelper<Type>::deserialize(data, field[c]);
        }
    }
};



#undef MEMORIA_TYPED_FIELD


}
