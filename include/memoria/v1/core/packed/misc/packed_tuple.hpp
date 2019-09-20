
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

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/tools/accessors.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>
#include <memoria/v1/api/common/packed_api.hpp>
#include <memoria/v1/api/datatypes/datum.hpp>
#include <memoria/v1/api/datatypes/type_registry.hpp>
#include <memoria/v1/core/strings/string_codec.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <memoria/v1/core/packed/misc/packed_so_default.hpp>

#include <type_traits>
#include <tuple>

namespace memoria {
namespace v1 {

namespace _ {

    template <typename PkdTuple, typename T, bool HasFF = HasFieldFactory<T>::Value>
    struct PkdTupleValueHandler;


    template <typename PkdTuple>
    struct PkdTupleValueHandler<PkdTuple, EmptyType, false>
    {
        static void set_value(const EmptyType& value, psize_t idx, PkdTuple& pkd_tuple) {}

        static void get_value(EmptyType& value, psize_t idx, const PkdTuple& pkd_tuple) {}

        template <typename SerializationData>
        static void serialize(SerializationData& buf, psize_t idx, const PkdTuple& tuple) {}

        template <typename DeserializationData>
        static void deserialize(DeserializationData& buf, psize_t idx, PkdTuple& tuple) {}

        static void generateDataEvents(const char* name, psize_t idx, IBlockDataEventHandler* handler, const PkdTuple& pkd_tuple)
        {
            U8String str("EMPTY_TYPE");
            BlockValueProviderT<U8String> pp(str);
            handler->value(name, pp);
        }
    };



    template <typename PkdTuple, typename T>
    struct PkdTupleValueHandler<PkdTuple, T, true> {
        static void set_value(const T& value, psize_t idx, PkdTuple& pkd_tuple)
        {
            T* obj;
            if (MMA1_UNLIKELY(pkd_tuple.is_empty(idx))) {
                obj = allocate<T>(pkd_tuple, idx);
            }
            else {
                obj = get<T>(pkd_tuple, idx);
            }

            new (*obj) T(value);
        }

        static void get_value(T& value, psize_t idx, const PkdTuple& pkd_tuple)
        {
            value = *get<T>(pkd_tuple, idx);
        }

        template <typename SerializationData>
        static void serialize(SerializationData& buf, psize_t idx, const PkdTuple& tuple)
        {
            const T* data = get<T>(tuple, idx);
            FieldFactory<T>::serialize(buf, *data);
        }

        template <typename DeserializationData>
        static void deserialize(DeserializationData& buf, psize_t idx, PkdTuple& tuple)
        {
            T* data = get<T>(tuple, idx);
            FieldFactory<T>::deserialize(buf, *data);
        }

        static void generateDataEvents(const char* name, psize_t idx, IBlockDataEventHandler* handler, const PkdTuple& pkd_tuple)
        {
            U8String str_value = fmt::format8(u"{}", *get<T>(pkd_tuple, idx));
            BlockValueProviderT<U8String> pp(str_value);
            handler->value(name, pp);
        }
    };

    template <typename PkdTuple, typename T>
    struct PkdTupleValueHandler<PkdTuple, T, false>
    {
        static void set_value(const T& value, psize_t idx, PkdTuple& pkd_tuple)
        {
            Datum<T> datum(value);
            U8String sdn_str = datum.to_sdn_string();

            ValueCodec<U8String> codec;

            psize_t str_data_length = codec.length(sdn_str);

            pkd_tuple.resizeBlock(idx, str_data_length);

            auto* addr = pkd_tuple.template get<typename ValueCodec<U8String>::BufferType>(idx);
            codec.encode(addr, sdn_str, 0);
        }

        static void get_value(T& value, psize_t idx, const PkdTuple& pkd_tuple)
        {
            ValueCodec<U8StringView> codec;
            const auto* addr = pkd_tuple.template get<typename ValueCodec<U8String>::BufferType>(idx);

            U8StringView sdn_str;
            codec.decode(addr, sdn_str, 0);

            AnyDatum any_datum = DataTypeRegistry::local().from_sdn_string(sdn_str);

            Datum<T> datum = datum_cast<T>(any_datum);

            value = datum.view();
        }

        template <typename SerializationData>
        static void serialize(SerializationData& buf, psize_t idx, const PkdTuple& tuple)
        {
            using BufferType = typename ValueCodec<U8StringView>::BufferType;
            const BufferType* data = get<BufferType>(tuple, idx);

            ValueCodec<U8StringView> codec;
            psize_t str_data_length = codec.length(data, 0);

            FieldFactory<T>::serialize(buf, data, str_data_length);
        }

        template <typename DeserializationData>
        static void deserialize(DeserializationData& buf, psize_t idx, PkdTuple& tuple)
        {
            T* data = get<T>(tuple, idx);
            FieldFactory<T>::deserialize(buf, *data);
        }

        static void generateDataEvents(const char* name, psize_t idx, IBlockDataEventHandler* handler, const PkdTuple& pkd_tuple)
        {
            using BufferType = typename ValueCodec<U8StringView>::BufferType;
            const BufferType* data = get<BufferType>(pkd_tuple, idx);

            ValueCodec<U8String> codec;
            U8String str_value;
            codec.decode(data, str_value, 0);

            BlockValueProviderT<U8String> pp(str_value);
            handler->value(name, pp);
        }
    };

}

template <typename Tuple_>
class PackedTuple: public PackedAllocator {
    using Base = PackedAllocator;
public:
    static constexpr uint32_t VERSION = 1;

    using MyType = PackedTuple;

    using Tuple = Tuple_;

    using SparseObject = PackedDefaultSO<PackedTuple>;

    static constexpr psize_t TupleSize = std::tuple_size<Tuple>::value;

    using Base::get;

public:

    using PackedAllocator::block_size;
    using PackedAllocator::init;
    using PackedAllocator::allocate;
    using PackedAllocator::allocateArrayBySize;

    PackedTuple() = default;

    OpStatus pack() {
        return OpStatus::OK;
    }

    struct SetValueFn {
        template <psize_t Idx>
        static bool process(const Tuple& tuple, MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            _::PkdTupleValueHandler<MyType, T>::set_value(std::get<Idx>(tuple), Idx, pkd_tuple);
            return true;
        }
    };

    void set_value(const Tuple& tuple) {
        ForEach<0, TupleSize>::process(SetValueFn(), tuple, *this);
    }

    struct GetValueFn {
        template <psize_t Idx>
        static bool process(Tuple& tuple, const MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            _::PkdTupleValueHandler<MyType, T>::get_value(std::get<Idx>(tuple), Idx, pkd_tuple);
            return true;
        }
    };

    void get_value(Tuple& tuple) const {
        ForEach<0, TupleSize>::process(GetValueFn(), tuple, *this);
    }

    static psize_t empty_size()
    {
        return PackedAllocator::block_size(0, TupleSize);
    }

    OpStatus init()
    {
        if(isFail(init(empty_size(), TupleSize))) {
            return OpStatus::FAIL;
        }

        return OpStatus::OK;
    }


    struct GenerateEventsFn {
        template <int32_t Idx>
        bool process(IBlockDataEventHandler* handler, const MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            _::PkdTupleValueHandler<MyType, T>::generateDataEvents("ENTRY", Idx, handler, pkd_tuple);

            return true;
        }
    };

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("PKD_TUPLE");

        constexpr psize_t tuple_size = std::tuple_size<Tuple>::value;
        handler->value("SIZE", &tuple_size);

        handler->startGroup("ENTRIES");

        ForEach<0, tuple_size>::process(GenerateEventsFn(), handler, *this);

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();
    }

    struct SerializeFn {
        template <int32_t Idx, typename SerializationData>
        bool process(SerializationData& buffer, const MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            _::PkdTupleValueHandler<MyType, T>::serialize(buffer, Idx, pkd_tuple);
            return true;
        }
    };

    template <typename SerializationData>
    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        constexpr psize_t tuple_size = std::tuple_size<Tuple>::value;
        ForEach<0, tuple_size>::process(SerializeFn(), buf, *this);
    }

    struct DeserializeFn {
        template <int32_t Idx, typename DeserializationData>
        bool process(DeserializationData& buffer, MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            _::PkdTupleValueHandler<MyType, T>::deserialize(buffer, Idx, pkd_tuple);
            return true;
        }
    };

    template <typename DeserializationData>
    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        constexpr psize_t tuple_size = std::tuple_size<Tuple>::value;
        ForEach<0, tuple_size>::process(DeserializeFn(), buf, *this);
    }
};


template <typename Types>
struct PackedStructTraits<PackedTuple<Types>> {
    using SearchKeyDataType = typename Types::Value;
    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;
};



}}
