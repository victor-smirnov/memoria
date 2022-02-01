
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

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/tools/accessors.hpp>
#include <memoria/core/types/algo/for_each.hpp>
#include <memoria/api/common/packed_api.hpp>
#include <memoria/core/datatypes/datum.hpp>
#include <memoria/core/datatypes/type_registry.hpp>
#include <memoria/core/strings/string_codec.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/packed/misc/packed_so_default.hpp>

#include <type_traits>
#include <tuple>

namespace memoria {

namespace detail {

    template <typename PkdTuple, typename T, bool HasFF = HasFieldFactory<T>::Value>
    struct PkdTupleValueHandler;


    template <typename PkdTuple>
    struct PkdTupleValueHandler<PkdTuple, std::tuple<>, true>
    {
        static void set_value(const std::tuple<>& value, psize_t idx, PkdTuple& pkd_tuple) {}

        static void get_value(std::tuple<>& value, psize_t idx, const PkdTuple& pkd_tuple) {}

        template <typename SerializationData>
        static VoidResult serialize(SerializationData& buf, psize_t idx, const PkdTuple& tuple) noexcept {
            return VoidResult::of();
        }

        template <typename DeserializationData>
        static VoidResult deserialize(DeserializationData& buf, psize_t idx, PkdTuple& tuple) noexcept {
            return VoidResult::of();
        }

        static VoidResult generateDataEvents(const char* name, psize_t idx, IBlockDataEventHandler* handler, const PkdTuple& pkd_tuple) noexcept
        {
            U8String str("EMPTY_TYPE");
            BlockValueProviderT<U8String> pp(str);
            handler->value(name, pp);            
            return VoidResult::of();
        }
    };



    template <typename PkdTuple, typename T>
    struct PkdTupleValueHandler<PkdTuple, T, true> {
        static void set_value(const T& value, psize_t idx, PkdTuple& pkd_tuple)
        {
            T* obj;
            if (MMA_UNLIKELY(pkd_tuple.is_empty(idx))) {
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
        static VoidResult serialize(SerializationData& buf, psize_t idx, const PkdTuple& tuple) noexcept
        {
            const T* data = get<T>(tuple, idx);
            FieldFactory<T>::serialize(buf, *data);

            return VoidResult::of();
        }

        template <typename DeserializationData>
        static VoidResult deserialize(DeserializationData& buf, psize_t idx, PkdTuple& tuple) noexcept
        {
            T* data = get<T>(tuple, idx);
            FieldFactory<T>::deserialize(buf, *data);
            return VoidResult::of();
        }

        static VoidResult generateDataEvents(const char* name, psize_t idx, IBlockDataEventHandler* handler, const PkdTuple& pkd_tuple) noexcept
        {
            U8String str_value = format_u8("{}", *get<T>(pkd_tuple, idx));
            BlockValueProviderT<U8String> pp(str_value);
            handler->value(name, pp);

            return VoidResult::of();
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
        static VoidResult serialize(SerializationData& buf, psize_t idx, const PkdTuple& tuple) noexcept
        {
            using BufferType = typename ValueCodec<U8StringView>::BufferType;
            const BufferType* data = get<BufferType>(tuple, idx);

            ValueCodec<U8StringView> codec;
            psize_t str_data_length = codec.length(data, 0);

            FieldFactory<T>::serialize(buf, data, str_data_length);
            return VoidResult::of();
        }

        template <typename DeserializationData>
        static VoidResult deserialize(DeserializationData& buf, psize_t idx, PkdTuple& tuple) noexcept
        {
            T* data = get<T>(tuple, idx);
            FieldFactory<T>::deserialize(buf, *data);

            return VoidResult::of();
        }

        static VoidResult generateDataEvents(const char* name, psize_t idx, IBlockDataEventHandler* handler, const PkdTuple& pkd_tuple) noexcept
        {
            using BufferType = typename ValueCodec<U8StringView>::BufferType;
            const BufferType* data = get<BufferType>(pkd_tuple, idx);

            ValueCodec<U8String> codec;
            U8String str_value;
            codec.decode(data, str_value, 0);

            BlockValueProviderT<U8String> pp(str_value);
            handler->value(name, pp);

            return VoidResult::of();
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

    VoidResult pack() noexcept {
        return VoidResult::of();
    }

    struct SetValueFn {
        template <psize_t Idx>
        static bool process(Tuple& tuple, MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            detail::PkdTupleValueHandler<MyType, T>::set_value(std::get<Idx>(tuple), Idx, pkd_tuple);
            return true;
        }
    };

    void set_value(Tuple& tuple) {
        ForEach<0, TupleSize>::process(SetValueFn(), tuple, *this);
    }

    struct GetValueFn {
        template <psize_t Idx>
        static bool process(Tuple& tuple, const MyType& pkd_tuple)
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            detail::PkdTupleValueHandler<MyType, T>::get_value(std::get<Idx>(tuple), Idx, pkd_tuple);
            return true;
        }
    };

    void get_value(Tuple& tuple) const noexcept {
        ForEach<0, TupleSize>::process(GetValueFn(), tuple, *this);
    }

    static psize_t empty_size() noexcept
    {
        return PackedAllocator::block_size(0, TupleSize);
    }

    VoidResult init() noexcept
    {
        MEMORIA_TRY_VOID(init(empty_size(), TupleSize));
        return VoidResult::of();
    }


    struct GenerateEventsFn {
        template <int32_t Idx>
        BoolResult process(IBlockDataEventHandler* handler, const MyType& pkd_tuple) noexcept
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            auto res = detail::PkdTupleValueHandler<MyType, T>::generateDataEvents("ENTRY", Idx, handler, pkd_tuple);
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(true);
        }
    };

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startStruct();
        handler->startGroup("PKD_TUPLE");

        constexpr psize_t tuple_size = std::tuple_size<Tuple>::value;
        handler->value("SIZE", &tuple_size);

        handler->startGroup("ENTRIES");

        auto res = ForEach<0, tuple_size>::process_res(GenerateEventsFn(), handler, *this);
        MEMORIA_RETURN_IF_ERROR(res);

        handler->endGroup();

        handler->endGroup();
        handler->endStruct();

        return VoidResult::of();
    }

    struct SerializeFn {
        template <int32_t Idx, typename SerializationData>
        BoolResult process(SerializationData& buffer, const MyType& pkd_tuple) noexcept
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            auto res = detail::PkdTupleValueHandler<MyType, T>::serialize(buffer, Idx, pkd_tuple);
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(true);
        }
    };

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        constexpr psize_t tuple_size = std::tuple_size<Tuple>::value;
        return ForEach<0, tuple_size>::process_res(SerializeFn(), buf, *this);
    }

    struct DeserializeFn {
        template <int32_t Idx, typename DeserializationData>
        BoolResult process(DeserializationData& buffer, MyType& pkd_tuple) noexcept
        {
            using T = std::tuple_element_t<Idx, Tuple>;
            auto res = detail::PkdTupleValueHandler<MyType, T>::deserialize(buffer, Idx, pkd_tuple);
            MEMORIA_RETURN_IF_ERROR(res);

            return BoolResult::of(true);
        }
    };

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        constexpr psize_t tuple_size = std::tuple_size<Tuple>::value;
        return ForEach<0, tuple_size>::process_res(DeserializeFn(), buf, *this);
    }
};


template <typename Types>
struct PackedStructTraits<PackedTuple<Types>> {
    using SearchKeyDataType = typename Types::Value;
    static constexpr PackedDataTypeSize DataTypeSize = PackedDataTypeSize::VARIABLE;
};



}
