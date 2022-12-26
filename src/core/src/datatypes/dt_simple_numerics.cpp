
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

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>
#include <memoria/core/datatypes/type_registry.hpp>
#include <memoria/core/datatypes/datum.hpp>

#include <memoria/core/linked/document/ld_common.hpp>

#include <memoria/core/datatypes/default_datatype_ops.hpp>
#include <memoria/core/tools/bitmap.hpp>

#include <string>
#include <cstdlib>
#include <type_traits>
#include <limits>


namespace memoria {

ArenaBuffer<U8StringView::value_type>& local_buffer() {
    static thread_local ArenaBuffer<U8StringView::value_type> buffer;
    return buffer;
}

const U8StringView::value_type* make_cstring(U8StringView str)
{
    auto& buf = local_buffer();

    buf.clear();
    buf.ensure(str.length());

    buf.add_size(str.length());
    MemCpyBuffer(str.data(), buf.data(), str.length());
    buf.append_value(0);

    return buf.data();
}



template <typename T, bool IsUnigned = std::is_unsigned<T>::value>
struct NumberCvt;

template <typename T>
struct NumberCvt<T, false> {
    static T convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            const auto* cstr = make_cstring(*value.as_varchar()->view());

            int64_t value = std::strtoll(cstr, nullptr, 0);
            if (errno)
            {
                MMA_THROW(RuntimeException())
                        << format_ex("Error converting string {} to number: {}", cstr, std::strerror(errno));
            }

            if (value < (int64_t)std::numeric_limits<T>::min()) {
                MMA_THROW(BoundsException())
                        << format_ex("Provided value {} is less than {}", value, (int64_t)std::numeric_limits<T>::min());
            }

            if (value > (int64_t)std::numeric_limits<T>::max()) {
                MMA_THROW(BoundsException())
                        << format_ex("Provided value {} is greather than {}", value, (int64_t)std::numeric_limits<T>::max());
            }

            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};



template <typename T>
struct NumberCvt<T, true> {
    static T convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            auto view = value.as_varchar()->view();
            const auto* cstr = make_cstring(*view);

            uint64_t value = std::strtoull(cstr, nullptr, 0);
            if (errno)
            {
                MMA_THROW(RuntimeException())
                        << format_ex("Error converting string {} to number: {}", *view, std::strerror(errno));
            }

            if (value > (uint64_t)std::numeric_limits<T>::max()) {
                MMA_THROW(BoundsException())
                        << format_ex("Provided value {} is greather than {}", value, (uint64_t)std::numeric_limits<T>::max());
            }

            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};


template <>
struct NumberCvt<float, false> {
    static float convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            const auto* cstr = make_cstring(*value.as_varchar()->view());

            float value = std::strtof(cstr, nullptr);
            if (errno)
            {
                MMA_THROW(RuntimeException())
                        << format_ex("Error converting string {} to number: {}", cstr, std::strerror(errno));
            }

            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};


template <>
struct NumberCvt<double, false> {
    static double convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            const auto* cstr = make_cstring(*value.as_varchar()->view());

            double value = std::strtod(cstr, nullptr);
            if (errno)
            {
                MMA_THROW(RuntimeException())
                        << format_ex("Error converting string {} to number: {}", cstr, std::strerror(errno));
            }

            return value;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};



template <typename T>
struct NumericDataTypeOperationsImpl: DataTypeOperations {

    virtual LDDValueTag type_hash() {
        return TypeHash<T>::Value & 0xFFFFFFFFFFFF;
    }

    virtual U8String full_type_name() {
        SBuf buf;
        DataTypeTraits<T>::create_signature(buf);
        return buf.str();
    }

    virtual boost::any create_cxx_instance(const LDTypeDeclarationView& typedecl) {
        return boost::any(T{});
    }

    virtual boost::any create_cxx_instance(const hermes::Datatype& typedecl) {
        return boost::any(T{});
    }

    virtual AnyDatum from_ld_document(const LDDValueView& value) {
        MMA_THROW(UnsupportedOperationException());
    }

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        std::ios_base::fmtflags f(out.flags());

        using StorageType = DTTLDStorageType<T>;
        out.precision(std::numeric_limits<StorageType>::max_digits10);

        StorageType val = *(doc->arena_.template get<StorageType>(ptr));
        out << '\'' << val << "'@" << full_type_name();

        out.flags(f);
    }

    virtual LDPtrHolder deep_copy_to(
            const LDDocumentView* src,
            LDPtrHolder ptr,
            LDDocumentView* tgt,
            ld_::LDArenaAddressMapping& mapping
    ) {
        DTTLDStorageType<T> val = *(src->arena_.template get<DTTLDStorageType<T>>(ptr));
        LDPtrHolder new_ptr = tgt->template new_value<T>(val);
        return new_ptr;
    }

    virtual LDDValueView construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        using NumberT = DTTLDStorageType<T>;

        NumberT val = NumberCvt<NumberT>::convert(value);
        LDPtrHolder new_ptr = doc->template new_value<T>(val);
        return LDDValueView(doc, new_ptr, ld_tag_value<T>());
    }
};

template <>
struct DataTypeOperationsImpl<UTinyInt>: NumericDataTypeOperationsImpl<UTinyInt> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        std::ios_base::fmtflags f(out.flags());
        out << std::dec;

        DTTLDStorageType<UTinyInt> val = *(doc->arena_.template get<DTTLDStorageType<UTinyInt>>(ptr));
        out << '\'' << (uint32_t)val << "'@" << full_type_name();

        out.flags(f);
    }
};

template <>
struct DataTypeOperationsImpl<TinyInt>: NumericDataTypeOperationsImpl<TinyInt> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        std::ios_base::fmtflags f(out.flags());
        out << std::dec;

        DTTLDStorageType<TinyInt> val = *(doc->arena_.template get<DTTLDStorageType<TinyInt>>(ptr));
        out << '\'' << (int32_t)val << "'@" << full_type_name();
        out.flags(f);
    }
};

template <>
struct DataTypeOperationsImpl<BigInt>: NumericDataTypeOperationsImpl<BigInt> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        std::ios_base::fmtflags f(out.flags());
        out << std::dec;

        DTTLDStorageType<BigInt> val = *(doc->arena_.template get<DTTLDStorageType<BigInt>>(ptr));
        out << val;
        out.flags(f);
    }
};

template <>
struct DataTypeOperationsImpl<Double>: NumericDataTypeOperationsImpl<Double> {

    virtual void dump(
            const LDDocumentView* doc,
            LDPtrHolder ptr,
            std::ostream& out,
            LDDumpFormatState& state,
            LDDumpState& dump_state
    ){
        std::ios_base::fmtflags f(out.flags());
        out << std::dec;

        using StorageType = DTTLDStorageType<Double>;

        out.precision(std::numeric_limits<StorageType>::max_digits10);

        StorageType val = *(doc->arena_.template get<StorageType>(ptr));
        out << val;
        out.flags(f);
    }
};

template <> struct DataTypeOperationsImpl<SmallInt>: NumericDataTypeOperationsImpl<SmallInt> {};
template <> struct DataTypeOperationsImpl<USmallInt>: NumericDataTypeOperationsImpl<USmallInt> {};

template <> struct DataTypeOperationsImpl<Integer>: NumericDataTypeOperationsImpl<Integer> {};
template <> struct DataTypeOperationsImpl<UInteger>: NumericDataTypeOperationsImpl<UInteger> {};

template <> struct DataTypeOperationsImpl<UBigInt>: NumericDataTypeOperationsImpl<UBigInt> {};

template <> struct DataTypeOperationsImpl<Real>: NumericDataTypeOperationsImpl<Real> {};

void InitSimpleNumericDatatypes()
{
    register_notctr_operations<UTinyInt>();
    register_notctr_operations<TinyInt>();

    register_notctr_operations<USmallInt>();
    register_notctr_operations<SmallInt>();

    register_notctr_operations<UInteger>();
    register_notctr_operations<Integer>();

    register_notctr_operations<UBigInt>();
    register_notctr_operations<BigInt>();

    register_notctr_operations<Real>();
    register_notctr_operations<Double>();
}

}
