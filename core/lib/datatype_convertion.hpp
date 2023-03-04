
// Copyright 2022-2023 Victor Smirnov
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
#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/strings/string_buffer.hpp>

#include <memoria/core/hermes/hermes.hpp>

#include <memoria/core/tools/uid_256.hpp>
#include <memoria/core/tools/uid_64.hpp>

#include <stdlib.h>
#include <string>

namespace memoria {

template <typename FromType, typename ToType>
struct DatatypeConverter;

template <typename Type>
struct FromPlainStringConverter;

template <typename DT>
struct ToPlainStringConverter {
    static U8String to_string(DTTViewType<DT> view) {
        SBuf buf;
        buf << view;
        return buf.str();
    }
};


template <>
struct ToPlainStringConverter<Varchar> {
    static U8String to_string(DTTViewType<Varchar> view) {
        return view;
    }
};

template <>
struct ToPlainStringConverter<TinyInt> {
    static U8String to_string(DTTViewType<TinyInt> view) {
        return std::to_string(view);
    }
};

template <>
struct ToPlainStringConverter<UTinyInt> {
    static U8String to_string(DTTViewType<UTinyInt> view) {
        return std::to_string(view);
    }
};

template <>
struct ToPlainStringConverter<Hermes> {
    static U8String to_string(const DTTViewType<Hermes>& view) {
        return view.to_string();
    }
};


template <>
struct ToPlainStringConverter<Uid256> {
    static U8String to_string(DTTViewType<Uid256> view) {
        return view.str();
    }
};

template <>
struct ToPlainStringConverter<Uid64> {
    static U8String to_string(DTTViewType<Uid64> view) {
        return view.str();
    }
};

template <>
struct FromPlainStringConverter<Varchar> {
    static hermes::Object from_string(U8StringView str) {
        return hermes::HermesCtrView::wrap_dataobject<Varchar>(str);
    }
};

template <typename DT, typename NumberT = DTTViewType<DT>>
struct NumericFromPlainStringConverter {
    static hermes::Object from_string(U8StringView str) {
        return hermes::HermesCtrView::wrap_dataobject<DT>(str);
    }
};

template <>
struct FromPlainStringConverter<BigInt> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoll(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<BigInt>(val);
    }
};


template <>
struct FromPlainStringConverter<UBigInt> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoull(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<UBigInt>(val);
    }
};


template <>
struct FromPlainStringConverter<Real> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stof(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<Real>(val);
    }
};


template <>
struct FromPlainStringConverter<Double> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stof(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<Double>(val);
    }
};

template <>
struct FromPlainStringConverter<Integer> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoi(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<Integer>(val);
    }
};


template <>
struct FromPlainStringConverter<UInteger> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoul(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<UInteger>(val);
    }
};


template <>
struct FromPlainStringConverter<SmallInt> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoi(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<SmallInt>(val);
    }
};


template <>
struct FromPlainStringConverter<USmallInt> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoi(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<USmallInt>(val);
    }
};

template <>
struct FromPlainStringConverter<TinyInt> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoi(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<TinyInt>(val);
    }
};

template <>
struct FromPlainStringConverter<UTinyInt> {
    static hermes::Object from_string(U8StringView str) {
        auto val = std::stoi(std::string(str));
        return hermes::HermesCtrView::wrap_dataobject<UTinyInt>(val);
    }
};

template <>
struct FromPlainStringConverter<Boolean> {
    static hermes::Object from_string(U8StringView str) {

        bool val = str == "true" || str == "1";
        if (!val) {
            val = str == "false" || str == "0";
        }

        if (val) {
            return hermes::HermesCtrView::wrap_dataobject<Boolean>(val);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Varchar value of {} is not convertuble to Boolean", str).do_throw();
        }
    }
};

template <>
struct FromPlainStringConverter<Hermes> {
    static hermes::Object from_string(U8StringView str) {
        return hermes::HermesCtrView::parse_document(str).root();
    }
};


template <>
struct FromPlainStringConverter<Uid256> {
    static hermes::Object from_string(U8StringView str) {
        auto val = UID256::parse(str);
        return hermes::HermesCtrView::wrap_dataobject<Uid256>(val);
    }
};

template <>
struct FromPlainStringConverter<Uid64> {
    static hermes::Object from_string(U8StringView str) {
        auto val = UID64::parse(str);
        return hermes::HermesCtrView::wrap_dataobject<Uid64>(val);
    }
};

template <typename ToDT>
struct DatatypeConverter<Varchar, ToDT>: IDatatypeConverter {
    hermes::Object convert(const void* view) const
    {
        const DTTViewType<Varchar>* str_view = reinterpret_cast<const DTTViewType<Varchar>*>(view);
        return FromPlainStringConverter<ToDT>::from_string(*str_view);
    }
};


template <typename FromDT>
struct DatatypeConverter<FromDT, Varchar>: IDatatypeConverter {
    hermes::Object convert(const void* view) const
    {
        DTTViewType<FromDT> typed_view = *reinterpret_cast<const DTTViewType<FromDT>*>(view);
        auto str = ToPlainStringConverter<FromDT>::to_string(typed_view);
        return hermes::HermesCtrView::wrap_dataobject<Varchar>(str);
    }
};


template <>
struct DatatypeConverter<Varchar, Varchar>: IDatatypeConverter {
    hermes::Object convert(const void* view) const
    {
        DTTViewType<Varchar> typed_view = *reinterpret_cast<const DTTViewType<Varchar>*>(view);
        return hermes::HermesCtrView::wrap_dataobject<Varchar>(typed_view);
    }
};


template <typename FromDT, typename ToDT>
struct NumericConverter: IDatatypeConverter {
    hermes::Object convert(const void* view) const
    {
        DTTViewType<FromDT> from_view = *reinterpret_cast<const DTTViewType<FromDT>*>(view);
        DTTViewType<ToDT> to_view     = from_view;

        return hermes::HermesCtrView::wrap_dataobject<ToDT>(to_view);
    }
};


template <>
struct DatatypeConverter<BigInt, Double>: NumericConverter<BigInt, Double> {};

template <>
struct DatatypeConverter<Double, BigInt>: NumericConverter<Double, BigInt> {};

template <>
struct DatatypeConverter<BigInt, Integer>: NumericConverter<BigInt, Integer> {};

template <>
struct DatatypeConverter<Integer, BigInt>: NumericConverter<Integer, BigInt> {};

template <>
struct DatatypeConverter<Integer, UTinyInt>: NumericConverter<Integer, UTinyInt> {};

template <>
struct DatatypeConverter<Integer, USmallInt>: NumericConverter<Integer, USmallInt> {};

template <>
struct DatatypeConverter<UInteger, UTinyInt>: NumericConverter<UInteger, UTinyInt> {};

template <>
struct DatatypeConverter<UInteger, BigInt>: NumericConverter<UInteger, BigInt> {};

template <>
struct DatatypeConverter<Integer, Double>: NumericConverter<Integer, Double> {};

template <>
struct DatatypeConverter<UInteger, Double>: NumericConverter<UInteger, Double> {};

template <>
struct DatatypeConverter<Integer, Real>: NumericConverter<Integer, Real> {};

template <>
struct DatatypeConverter<UInteger, Real>: NumericConverter<UInteger, Real> {};


template <>
struct DatatypeConverter<UBigInt, UInteger>: NumericConverter<UBigInt, UInteger> {};

template <>
struct DatatypeConverter<UInteger, UBigInt>: NumericConverter<UInteger, UBigInt> {};


template <>
struct DatatypeConverter<BigInt, Real>: NumericConverter<BigInt, Real> {};

template <>
struct DatatypeConverter<Real, BigInt>: NumericConverter<Real, BigInt> {};

template <>
struct DatatypeConverter<Double, Real>: NumericConverter<Double, Real> {};

template <>
struct DatatypeConverter<Real, Double>: NumericConverter<Real, Double> {};

template <>
struct DatatypeConverter<UBigInt, Double>: NumericConverter<UBigInt, Double> {};

template <>
struct DatatypeConverter<Double, UBigInt>: NumericConverter<Double, UBigInt> {};

template <>
struct DatatypeConverter<UBigInt, Real>: NumericConverter<UBigInt, Real> {};

template <>
struct DatatypeConverter<Real, UBigInt>: NumericConverter<Real, UBigInt> {};

template <>
struct DatatypeConverter<USmallInt, Integer>: NumericConverter<USmallInt, Integer> {};



}
