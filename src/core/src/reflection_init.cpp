
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

#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/hermes/hermes.hpp>

#include <memoria/core/datatypes/reflection.hpp>
#include <memoria/core/datatypes/core.hpp>

#include "reflection_internal.hpp"

namespace memoria {

using namespace hermes;

namespace {

template <typename DTList> struct DataObjectReflectionListBuilder;

template <typename DT, typename... Tail>
struct DataObjectReflectionListBuilder<TL<DT, Tail...>> {
    static void build() {
        register_type_reflection(std::make_unique<HermesTypeReflectionDatatypeImpl<DataObject<DT>, DT>>());
        DataObjectReflectionListBuilder<TL<Tail...>>::build();
    }
};

template <>
struct DataObjectReflectionListBuilder<TL<>> {
    static void build() {}
};

}


void InitTypeReflections()
{
    HermesCtr::init_hermes_doc_parser();

    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<Object>, TypedGenericArray<Object>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<Integer>, TypedGenericArray<Integer>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<UInteger>, TypedGenericArray<UInteger>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<Real>, TypedGenericArray<Real>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<Double>, TypedGenericArray<Double>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<UBigInt>, TypedGenericArray<UBigInt>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<BigInt>, TypedGenericArray<BigInt>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<SmallInt>, TypedGenericArray<SmallInt>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<USmallInt>, TypedGenericArray<USmallInt>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<TinyInt>, TypedGenericArray<TinyInt>>>());
    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Array<UTinyInt>, TypedGenericArray<UTinyInt>>>());

    register_type_reflection(std::make_unique<HermesContainerTypeReflectionImpl<Map<Varchar, Object>, GenericObjectMap>>());
    register_type_reflection(std::make_unique<HermesTypeReflectionImpl<Datatype>>());
    register_type_reflection(std::make_unique<HermesTypeReflectionImpl<TypedValue>>());
    register_type_reflection(std::make_unique<HermesTypeReflectionImpl<Parameter>>());

    DataObjectReflectionListBuilder<AllHermesDatatypes>::build();
}

}
