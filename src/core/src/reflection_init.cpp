
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
        register_type_reflection(*std::make_shared<HermesTypeReflectionDatatypeImpl<DataObject<DT>, DT>>());
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

    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<Object>, TypedGenericArray<Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<Integer>, TypedGenericArray<Integer>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<UInteger>, TypedGenericArray<UInteger>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<Real>, TypedGenericArray<Real>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<Double>, TypedGenericArray<Double>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<UBigInt>, TypedGenericArray<UBigInt>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<BigInt>, TypedGenericArray<BigInt>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<SmallInt>, TypedGenericArray<SmallInt>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<USmallInt>, TypedGenericArray<USmallInt>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<TinyInt>, TypedGenericArray<TinyInt>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Array<UTinyInt>, TypedGenericArray<UTinyInt>>>());

    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<UTinyInt, Object>, TypedGenericMap<UTinyInt, Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<Varchar, Object>, TypedGenericMap<Varchar, Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<Integer, Object>, TypedGenericMap<Integer, Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<BigInt, Object>, TypedGenericMap<BigInt, Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<Double, Object>, TypedGenericMap<Double, Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<Real, Object>, TypedGenericMap<Real, Object>>>());
    register_type_reflection(*std::make_shared<HermesContainerTypeReflectionImpl<Map<UBigInt, Object>, TypedGenericMap<UBigInt, Object>>>());


    register_type_reflection(*std::make_shared<HermesTypeReflectionImpl<Datatype>>());
    register_type_reflection(*std::make_shared<HermesTypeReflectionImpl<TypedValue>>());
    register_type_reflection(*std::make_shared<HermesTypeReflectionImpl<Parameter>>());

    DataObjectReflectionListBuilder<AllHermesDatatypes>::build();

    for_each_type_reflection([](const ShortTypeCode& type_code, TypeReflection& reflection) {
        auto datatype = HermesCtr::parse_datatype(reflection.str())->root()->as_datatype();

//        println("DE: {}", datatype.to_cxx_string());

        auto hash = datatype->cxx_type_hash();
        register_type_reflection(hash, *reflection.self());

        // FIXME strip_namespace is apparently broken for numeric type param
//        auto alias_dt = strip_namespaces(datatype);
//        if (*alias_dt->type_name()->view() == "DataObject") {
//            alias_dt = alias_dt->type_parameters()->get(0)->as_datatype();
//        }

        auto alias_dt = datatype;

        register_type_reflection(alias_dt->cxx_type_hash(), *reflection.self());
    });

}

}
