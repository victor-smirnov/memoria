
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

class HermesCtrBuilder;
class HermesCtr;
class HermesCtrImpl;


struct IParameterResolver;

template <typename>
class Array;

template <typename Key, typename Object>
class Map;

class Object;

class Parameter;


template <typename>
class DataObject;

using StringValue    = DataObject<Varchar>;
using StringValuePtr = ViewPtr<StringValue, true>;

class Datatype;
using DatatypePtr   = ViewPtr<Datatype, true>;

using ObjectMap     = Map<Varchar, Object>;
using ObjectMapPtr  = ViewPtr<ObjectMap, true>;

using ObjectArray    = Array<Object>;
using ObjectArrayPtr = ViewPtr<ObjectArray, true>;

template <typename DT>
using ArrayPtr = ViewPtr<Array<DT>, true>;
using ObjectPtr = ViewPtr<Object, true>;

template <typename DT>
using DataObjectPtr = ViewPtr<DataObject<DT>, true>;

class TypedValue;
using TypedValuePtr = ViewPtr<TypedValue, true>;

using ParameterPtr  = ViewPtr<Parameter, true>;

enum ObjectTypes {
    HERMES_OBJECT_DATA = 0, HERMES_OBJECT_ARRAY = 1, HERMES_OBJECT_MAP = 2
};


}

template <typename DT>
struct TypeHash<hermes::DataObject<DT>>: HasU64Value<TypeHashV<DT>> {};


template <>
struct TypeHash<hermes::Array<hermes::Object>>: HasU64Value<100> {};

template <>
struct TypeHash<hermes::Map<Varchar, hermes::Object>>: HasU64Value<101> {};

template <>
struct TypeHash<hermes::Datatype>: HasU64Value<102> {};

template <>
struct TypeHash<hermes::TypedValue>: HasU64Value<103> {};

template <>
struct TypeHash<hermes::Parameter>: HasU64Value<104> {};


template <>
struct TypeHash<hermes::Array<Integer>>: HasU64Value<105> {};


template <typename T>
struct TypeDescriptor<hermes::Array<T>>:HasU64Value<hermes::HERMES_OBJECT_ARRAY> {};

template <typename K, typename V>
struct TypeDescriptor<hermes::Map<K, V>>:HasU64Value<hermes::HERMES_OBJECT_MAP> {};

}
