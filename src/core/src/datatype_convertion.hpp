
// Copyright 2022 Victor Smirnov
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
struct FromPlainStringConverter<Varchar> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        return doc->set_dataobject<Varchar>(str)->as_object();
    }
};

template <typename DT, typename NumberT = DTTViewType<DT>>
struct NumericFromPlainStringConverter {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        return doc->set_dataobject<DT>(str)->as_object();
    }
};

template <>
struct FromPlainStringConverter<BigInt> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoll(std::string(str));
        return doc->set_dataobject<BigInt>(val)->as_object();
    }
};


template <>
struct FromPlainStringConverter<UBigInt> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoull(std::string(str));
        return doc->set_dataobject<UBigInt>(val)->as_object();
    }
};


template <>
struct FromPlainStringConverter<Real> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stof(std::string(str));
        return doc->set_dataobject<Real>(val)->as_object();
    }
};


template <>
struct FromPlainStringConverter<Double> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stof(std::string(str));
        return doc->set_dataobject<Double>(val)->as_object();
    }
};

template <>
struct FromPlainStringConverter<Integer> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoi(std::string(str));
        return doc->set_dataobject<Integer>(val)->as_object();
    }
};


template <>
struct FromPlainStringConverter<UInteger> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoul(std::string(str));
        return doc->set_dataobject<UInteger>(val)->as_object();
    }
};


template <>
struct FromPlainStringConverter<SmallInt> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoi(std::string(str));
        return doc->set_dataobject<SmallInt>(val)->as_object();
    }
};


template <>
struct FromPlainStringConverter<USmallInt> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoi(std::string(str));
        return doc->set_dataobject<USmallInt>(val)->as_object();
    }
};

template <>
struct FromPlainStringConverter<TinyInt> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoi(std::string(str));
        return doc->set_dataobject<TinyInt>(val)->as_object();
    }
};

template <>
struct FromPlainStringConverter<UTinyInt> {
    static hermes::ObjectPtr from_string(U8StringView str) {
        auto doc = hermes::HermesCtr::make_pooled();
        auto val = std::stoi(std::string(str));
        return doc->set_dataobject<UTinyInt>(val)->as_object();
    }
};

template <>
struct FromPlainStringConverter<Boolean> {
    static hermes::ObjectPtr from_string(U8StringView str) {

        bool val = str == "true" || str == "1";
        if (!val) {
            val = str == "false" || str == "0";
        }

        if (val) {
            auto doc = hermes::HermesCtr::make_pooled();
            return doc->set_dataobject<Boolean>(val)->as_object();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Varchar value of {} is not convertuble to Boolean", str).do_throw();
        }
    }
};


template <typename ToDT>
struct DatatypeConverter<Varchar, ToDT>: IDatatypeConverter {
    hermes::ObjectPtr convert(const void* view) const
    {
        const DTTViewType<Varchar>* str_view = reinterpret_cast<const DTTViewType<Varchar>*>(view);
        return FromPlainStringConverter<ToDT>::from_string(*str_view);
    }
};


template <typename FromDT>
struct DatatypeConverter<FromDT, Varchar>: IDatatypeConverter {
    hermes::ObjectPtr convert(const void* view) const
    {
        DTTViewType<FromDT> typed_view = *reinterpret_cast<const DTTViewType<FromDT>*>(view);
        auto str = ToPlainStringConverter<FromDT>::to_string(typed_view);
        auto doc = hermes::HermesCtr::make_pooled();
        return doc->set_dataobject<Varchar>(str)->as_object();
    }
};

template <typename FromDT, typename ToDT>
struct NumericConverter: IDatatypeConverter {
    hermes::ObjectPtr convert(const void* view) const
    {
        DTTViewType<FromDT> from_view = *reinterpret_cast<const DTTViewType<FromDT>*>(view);
        DTTViewType<ToDT> to_view     = from_view;

        return hermes::HermesCtr::wrap_dataobject<ToDT>(to_view)->as_object();
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



}
