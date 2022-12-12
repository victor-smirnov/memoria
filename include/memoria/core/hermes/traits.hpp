
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
#include <memoria/core/datatypes/varchars/varchars.hpp>

namespace memoria {

struct Varchar;

template <typename>
class HermesTypeReflectionDatatypeImpl;

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
class ArrayView;

template <typename Key, typename Value>
class MapView;


template <typename Key, typename Value>
using Map = Own<MapView<Key, Value>, OwningKind::HOLDING>;

class ObjectView;

class ParameterView;

using Object = Own<ObjectView, OwningKind::HOLDING>;

using StringOView = DTView<Varchar>;

class DatatypeView;
using Datatype = Own<DatatypeView, OwningKind::HOLDING>;

using ObjectMapView = MapView<Varchar, Object>;
using ObjectMap     = Own<ObjectMapView, OwningKind::HOLDING>;

using TinyObjectMapView = MapView<UTinyInt, Object>;
using TinyObjectMap     = Own<TinyObjectMapView, OwningKind::HOLDING>;

using ObjectArrayView   = ArrayView<Object>;
using ObjectArray       = Own<ObjectArrayView, OwningKind::HOLDING>;

template <typename DT>
using Array = Own<ArrayView<DT>, OwningKind::HOLDING>;

class TypedValueView;
using TypedValue = Own<TypedValueView, OwningKind::HOLDING>;

using Parameter  = Own<ParameterView, OwningKind::HOLDING>;

enum ObjectTypes {
    HERMES_OBJECT_DATA = 0, HERMES_OBJECT_ARRAY = 1, HERMES_OBJECT_MAP = 2
};


}

template <>
struct TypeHash<hermes::Object>: HasU64Value<99> {};


template <>
struct TypeHash<hermes::Array<hermes::Object>>: HasU64Value<100> {};

template <>
struct TypeHash<hermes::Map<Varchar, hermes::Object>>: HasU64Value<101> {};

template <>
struct TypeHash<hermes::Map<UTinyInt, hermes::Object>>: HasU64Value<98> {};


template <>
struct TypeHash<hermes::Datatype>: HasU64Value<102> {};

template <>
struct TypeHash<hermes::TypedValue>: HasU64Value<103> {};

template <>
struct TypeHash<hermes::Parameter>: HasU64Value<104> {};


template <>
struct TypeHash<hermes::Array<Integer>>: HasU64Value<105> {};

template <typename T>
struct TypeHash<hermes::Array<T>>: HasU64Value<
        HashHelper<
            105,
            TypeHashV<T>
        >
> {};

template <typename Key>
struct TypeHash<hermes::Map<Key, hermes::Object>>: HasU64Value<
        HashHelper<
            106,
            TypeHashV<Key>,
            TypeHashV<hermes::Object>
        >
> {};


template <typename T>
struct TypeDescriptor<hermes::Array<T>>:HasU64Value<hermes::HERMES_OBJECT_ARRAY> {};


template <typename K, typename V>
struct TypeDescriptor<hermes::Map<K, V>>:HasU64Value<hermes::HERMES_OBJECT_MAP> {};

}
