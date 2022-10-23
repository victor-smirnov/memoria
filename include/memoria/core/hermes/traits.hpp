
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

#include <memoria/core/reflection/typehash.hpp>
#include <memoria/core/datatypes/traits.hpp>

namespace memoria {

struct Varchar;

namespace hermes {
namespace path {
namespace interpreter {
class Interpreter;
}
}
}

namespace hermes {
class DocumentBuilder;

class DocView;

template <typename>
class Array;

template <typename Key, typename Value>
class Map;

class Value;


template <typename>
class DataObject;

using StringValue    = DataObject<Varchar>;
using StringValuePtr = ViewPtr<StringValue, true>;

class Datatype;
using DatatypePtr   = ViewPtr<Datatype, true>;


using GenericMap    = Map<Varchar, Value>;
using GenericMapPtr = ViewPtr<GenericMap, true>;

using GenericArray    = Array<Value>;
using GenericArrayPtr = ViewPtr<GenericArray, true>;

using ValuePtr = ViewPtr<Value, true>;

template <typename DT>
using DataObjectPtr = ViewPtr<DataObject<DT>, true>;

class TypedValue;
using TypedValuePtr = ViewPtr<TypedValue, true>;



namespace detail {
template <typename> struct ValueCastHelper;
}

}

template <typename DT>
struct TypeHash<hermes::DataObject<DT>>: HasU64Value<TypeHashV<DT>> {};


template <>
struct TypeHash<hermes::Array<hermes::Value>>: HasU64Value<100> {};

template <>
struct TypeHash<hermes::Map<Varchar, hermes::Value>>: HasU64Value<101> {};

template <>
struct TypeHash<hermes::Datatype>: HasU64Value<102> {};

template <>
struct TypeHash<hermes::TypedValue>: HasU64Value<103> {};



}
