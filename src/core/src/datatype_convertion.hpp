
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
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        doc->set_dataobject<Varchar>(str);
        return doc;
    }
};

template <typename DT, typename NumberT = DTTViewType<DT>>
struct NumericFromPlainStringConverter {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        doc->set_dataobject<DT>(str);
        return doc;
    }
};

template <>
struct FromPlainStringConverter<BigInt> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoll(std::string(str));
        doc->set_dataobject<BigInt>(val);
        return doc;
    }
};


template <>
struct FromPlainStringConverter<UBigInt> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoull(std::string(str));
        doc->set_dataobject<UBigInt>(val);
        return doc;
    }
};


template <>
struct FromPlainStringConverter<Real> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stof(std::string(str));
        doc->set_dataobject<Real>(val);
        return doc;
    }
};


template <>
struct FromPlainStringConverter<Double> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stof(std::string(str));
        doc->set_dataobject<Double>(val);
        return doc;
    }
};

template <>
struct FromPlainStringConverter<Integer> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoi(std::string(str));
        doc->set_dataobject<Integer>(val);
        return doc;
    }
};


template <>
struct FromPlainStringConverter<UInteger> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoul(std::string(str));
        doc->set_dataobject<UInteger>(val);
        return doc;
    }
};


template <>
struct FromPlainStringConverter<SmallInt> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoi(std::string(str));
        doc->set_dataobject<SmallInt>(val);
        return doc;
    }
};


template <>
struct FromPlainStringConverter<USmallInt> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoi(std::string(str));
        doc->set_dataobject<USmallInt>(val);
        return doc;
    }
};

template <>
struct FromPlainStringConverter<TinyInt> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoi(std::string(str));
        doc->set_dataobject<TinyInt>(val);
        return doc;
    }
};

template <>
struct FromPlainStringConverter<UTinyInt> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoi(std::string(str));
        doc->set_dataobject<UTinyInt>(val);
        return doc;
    }
};

template <>
struct FromPlainStringConverter<Boolean> {
    static PoolSharedPtr<hermes::DocView> from_string(U8StringView str) {
        auto doc = hermes::DocView::make_pooled();
        auto val = std::stoi(std::string(str));
        doc->set_dataobject<Boolean>(val);
        return doc;
    }
};


template <typename ToDT>
struct DatatypeConverter<Varchar, ToDT>: IDatatypeConverter {
    PoolSharedPtr<hermes::DocView> convert(const void* view) const
    {
        const DTTViewType<Varchar>* str_view = reinterpret_cast<const DTTViewType<Varchar>*>(view);
        return FromPlainStringConverter<ToDT>::from_string(*str_view);
    }
};


template <typename FromDT>
struct DatatypeConverter<FromDT, Varchar>: IDatatypeConverter {
    PoolSharedPtr<hermes::DocView> convert(const void* view) const
    {
        DTTViewType<FromDT> typed_view = *reinterpret_cast<const DTTViewType<FromDT>*>(view);
        auto str = ToPlainStringConverter<FromDT>::to_string(typed_view);
        auto doc = hermes::DocView::make_pooled();
        doc->set_dataobject<Varchar>(str);
        return doc;
    }
};


}
