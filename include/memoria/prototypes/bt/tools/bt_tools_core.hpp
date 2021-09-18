
// Copyright 2012 Victor Smirnov
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

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_vector_tuple.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/uid_64.hpp>
#include <memoria/core/tools/uid_256.hpp>
#include <memoria/core/container/cow.hpp>

#include <ostream>
#include <tuple>

namespace memoria {

template <typename T> struct FieldFactory;

template<>
struct FieldFactory<UUID> {

    using Type = UUID;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        FieldFactory<uint64_t>::serialize(data, field.lo());
        FieldFactory<uint64_t>::serialize(data, field.hi());
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            FieldFactory<uint64_t>::serialize(data, field[c].lo());
            FieldFactory<uint64_t>::serialize(data, field[c].hi());
        }
    }


    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        FieldFactory<uint64_t>::deserialize(data, field.lo());
        FieldFactory<uint64_t>::deserialize(data, field.hi());
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            FieldFactory<uint64_t>::deserialize(data, field[c].lo());
            FieldFactory<uint64_t>::deserialize(data, field[c].hi());
        }
    }
};






template<>
struct FieldFactory<UID64> {

    using Type = UID64;
    using AtomT = typename UID64::AtomT;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        FieldFactory<AtomT>::serialize(data, field.atom());
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            FieldFactory<AtomT>::serialize(data, field[c].atom());
        }
    }


    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        AtomT atom;
        FieldFactory<AtomT>::deserialize(data, atom);
        field.set_atom(atom);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++){
            deserialize(data, field[c]);
        }
    }
};


template<>
struct FieldFactory<UID256> {

    using Type = UID256;
    using AtomT = typename UID256::AtomT;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        for (size_t c = 0; c < Type::NUM_ATOMS; c++) {
            FieldFactory<AtomT>::serialize(data, field.atom(c));
        }
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            serialize(data, field[c]);
        }
    }


    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        for (size_t c = 0; c < Type::NUM_ATOMS; c++) {
            AtomT atom;
            FieldFactory<AtomT>::deserialize(data, atom);
            field.set_atom(c, atom);
        }
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++){
            deserialize(data, field[c]);
        }
    }
};



template<>
struct FieldFactory<CowBlockID<UID64>> {

    using Type = CowBlockID<UID64>;
    using UidT = UID64;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        FieldFactory<UidT>::serialize(data, field.value());
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            FieldFactory<UidT>::serialize(data, field[c].value());
        }
    }


    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        UidT atom;
        FieldFactory<UidT>::deserialize(data, atom);
        field.set_value(atom);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++){
            deserialize(data, field[c]);
        }
    }
};


template<>
struct FieldFactory<CowBlockID<UID256>> {

    using Type = CowBlockID<UID256>;
    using UidT = UID256;

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type& field)
    {
        FieldFactory<UidT>::serialize(data, field.value());
    }

    template <typename SerializationData>
    static void serialize(SerializationData& data, const Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++)
        {
            FieldFactory<UidT>::serialize(data, field[c].value());
        }
    }


    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type& field)
    {
        UidT uid;
        FieldFactory<UidT>::deserialize(data, uid);
        field.set_value(uid);
    }

    template <typename DeserializationData>
    static void deserialize(DeserializationData& data, Type* field, int32_t size)
    {
        for (int32_t c = 0; c < size; c++){
            deserialize(data, field[c]);
        }
    }
};



namespace bt {

template <typename List> struct TypeListToTupleH;

template <typename List>
using TypeListToTuple = typename TypeListToTupleH<List>::Type;

template <typename... List>
struct TypeListToTupleH<TL<List...>> {
    using Type = std::tuple<List...>;
};

}}
