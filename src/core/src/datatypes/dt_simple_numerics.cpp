
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

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/datatypes/type_registry.hpp>
#include <memoria/v1/core/datatypes/datum.hpp>

#include <memoria/v1/core/linked/document/ld_common.hpp>

#include <memoria/v1/core/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

#include <string>
#include <cstdlib>
#include <type_traits>
#include <limits>


namespace memoria {
namespace v1 {

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
            const auto* cstr = make_cstring(value.as_varchar().view());

            int64_t value = std::strtoll(cstr, nullptr, 0);
            if (errno)
            {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(u"Error converting string {} to number: {}", cstr, std::strerror(errno));
            }

            if (value < (int64_t)std::numeric_limits<T>::min()) {
                MMA1_THROW(BoundsException())
                        << fmt::format_ex(u"Provided value {} is less than {}", value, (int64_t)std::numeric_limits<T>::min());
            }

            if (value > (int64_t)std::numeric_limits<T>::max()) {
                MMA1_THROW(BoundsException())
                        << fmt::format_ex(u"Provided value {} is greather than {}", value, (int64_t)std::numeric_limits<T>::max());
            }

            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};



template <typename T>
struct NumberCvt<T, true> {
    static T convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            U8StringView view = value.as_varchar().view();
            const auto* cstr = make_cstring(view);

            uint64_t value = std::strtoull(cstr, nullptr, 0);
            if (errno)
            {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(u"Error converting string {} to number: {}", view, std::strerror(errno));
            }

            if (value > (uint64_t)std::numeric_limits<T>::max()) {
                MMA1_THROW(BoundsException())
                        << fmt::format_ex(u"Provided value {} is greather than {}", value, (uint64_t)std::numeric_limits<T>::max());
            }

            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};


template <>
struct NumberCvt<float, false> {
    static float convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            const auto* cstr = make_cstring(value.as_varchar().view());

            float value = std::strtof(cstr, nullptr);
            if (errno)
            {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(u"Error converting string {} to number: {}", cstr, std::strerror(errno));
            }

            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
        }
    }
};


template <>
struct NumberCvt<double, false> {
    static double convert(const LDDValueView& value) {
        if (value.is_varchar())
        {
            const auto* cstr = make_cstring(value.as_varchar().view());

            double value = std::strtod(cstr, nullptr);
            if (errno)
            {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(u"Error converting string {} to number: {}", cstr, std::strerror(errno));
            }

            return value;
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Unsupported linked data value type");
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

    virtual AnyDatum from_ld_document(const LDDValueView& value) {
        MMA1_THROW(UnsupportedOperationException());
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
        LDPtrHolder new_ptr = tgt->template new_value_raw<T>(val);
        return new_ptr;
    }

    virtual LDPtrHolder construct_from(
            LDDocumentView* doc,
            const LDDValueView& value
    ) {
        using NumberT = DTTLDStorageType<T>;

        NumberT val = NumberCvt<NumberT>::convert(value);
        LDPtrHolder new_ptr = doc->template new_value_raw<T>(val);
        return new_ptr;
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
    DataTypeRegistryStore::NoTCtrOpsInitializer<UTinyInt> ldd_u_tiny_int_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<TinyInt> ldd_tiny_int_;

    DataTypeRegistryStore::NoTCtrOpsInitializer<USmallInt> ldd_u_small_int_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<SmallInt> ldd_small_int_;

    DataTypeRegistryStore::NoTCtrOpsInitializer<UInteger> ldd_u_int_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<Integer> ldd_int_;

    DataTypeRegistryStore::NoTCtrOpsInitializer<UBigInt> ldd_u_big_int_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<BigInt> ldd_big_int_;

    DataTypeRegistryStore::NoTCtrOpsInitializer<Real> ldd_real_;
    DataTypeRegistryStore::NoTCtrOpsInitializer<Double> ldd_double_;
}

}}
