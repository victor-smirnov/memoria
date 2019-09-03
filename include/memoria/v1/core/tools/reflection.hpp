
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

#include <memoria/v1/core/tools/md5.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#include <string.h>
#include <tuple>

namespace memoria {
namespace v1 {

inline int64_t PtrToLong(const void *ptr) {
    return T2T<int64_t>(ptr);
}

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
class BitField{};

template <typename Type>
struct FieldFactory<BitField<Type> > {

    typedef BitField<Type> BFType;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        field.template serialize<FieldFactory>(data);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        field.template deserialize<FieldFactory>(data);
    }
};

template <>
struct FieldFactory<EmptyValue> {

    template <typename SerializationData>
    static void serialize(SerializationData& data, const EmptyValue& field, int32_t count = 1) {}

    template <typename SerializationData>
    static void serialize(SerializationData& data, const EmptyValue* field, int32_t count = 1) {}

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, EmptyValue& field, int32_t count = 1) {}

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, EmptyValue* field, int32_t count = 1) {}
};



#define MEMORIA_TYPED_FIELD(Type)                                               \
template <> struct FieldFactory<Type> {                                         \
    template <typename SerializationData>                                       \
    static void serialize(SerializationData& data, const Type& field) {         \
        memcpy(data.buf, &field, sizeof(Type));                                 \
        data.buf += sizeof(Type);                                               \
        data.total += sizeof(Type);                                             \
    }                                                                           \
                                                                                \
    template <typename DeserializationData>                                     \
    static void deserialize(DeserializationData& data, Type& field) {           \
        memcpy(&field, data.buf, sizeof(Type));                                 \
        data.buf += sizeof(Type);                                               \
    }                                                                           \
                                                                                \
    template <typename SerializationData>                                       \
    static void serialize(SerializationData& data, const Type& field, int32_t count) {\
        memcpy(data.buf, &field, count*sizeof(Type));                           \
        data.buf += count * sizeof(Type);                                       \
        data.total += count * sizeof(Type);                                     \
    }                                                                           \
                                                                                \
    template <typename DeserializationData>                                     \
    static void deserialize(DeserializationData& data, Type& field, int32_t count) {\
        memcpy(&field, data.buf, count*sizeof(Type));                           \
        data.buf += count*sizeof(Type);                                         \
    }                                                                           \
                                                                                \
    template <typename SerializationData>                                       \
    static void serialize(SerializationData& data, const Type* field, int32_t count) {\
        memcpy(data.buf, field, count*sizeof(Type));                            \
        data.buf += count * sizeof(Type);                                       \
        data.total += count * sizeof(Type);                                     \
    }                                                                           \
                                                                                \
    template <typename DeserializationData>                                     \
    static void deserialize(DeserializationData& data, Type* field, int32_t count) {\
        memcpy(field, data.buf, count*sizeof(Type));                            \
        data.buf += count*sizeof(Type);                                         \
    }                                                                           \
}


template <> struct FieldFactory<uint64_t> {

    using Type = uint64_t;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field) {
        memcpy(data.buf, &field, sizeof(Type));
        data.buf += sizeof(Type);
        data.total += sizeof(Type);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field) {
        memcpy(&field, data.buf, sizeof(Type));
        data.buf += sizeof(Type);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field, int32_t count) {
        memcpy(data.buf, &field, count*sizeof(Type));
        data.buf += count * sizeof(Type);
        data.total += count * sizeof(Type);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field, int32_t count) {
        memcpy(&field, data.buf, count*sizeof(Type));
        data.buf += count*sizeof(Type);
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t count) {
        memcpy(data.buf, field, count*sizeof(Type));
        data.buf += count * sizeof(Type);
        data.total += count * sizeof(Type);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t count) {
        memcpy(field, data.buf, count*sizeof(Type));
        data.buf += count*sizeof(Type);
    }
};


MEMORIA_TYPED_FIELD(char);
MEMORIA_TYPED_FIELD(int8_t);
MEMORIA_TYPED_FIELD(int16_t);
MEMORIA_TYPED_FIELD(int32_t);
MEMORIA_TYPED_FIELD(int64_t);
MEMORIA_TYPED_FIELD(uint8_t);
MEMORIA_TYPED_FIELD(uint16_t);
MEMORIA_TYPED_FIELD(uint32_t);
MEMORIA_TYPED_FIELD(float);
MEMORIA_TYPED_FIELD(double);
MEMORIA_TYPED_FIELD(bool);


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


}}
