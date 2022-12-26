
// Copyright 2011-2022 Victor Smirnov
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



#include <memoria/core/strings/string.hpp>
#include <memoria/profiles/common/container_operations.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/tools/result.hpp>

#include <memoria/core/hermes/hermes.hpp>

#include "reflection_internal.hpp"

#include <memory>

namespace memoria {

using ShortCodeMap = ska::flat_hash_map<uint64_t, std::shared_ptr<TypeReflection>>;
using TypeCodeMap  = ska::flat_hash_map<UID256, std::shared_ptr<TypeReflection>>;


IDatatypeConverter::~IDatatypeConverter() noexcept {}

namespace {

ShortCodeMap& short_code_map() {
    static ShortCodeMap map;
    return map;
}

TypeCodeMap& type_code_map() {
    static TypeCodeMap map;
    return map;
}

template <typename T>
class HermesTypeReflectionImpl: public TypeReflection {};

}

TypeReflection& get_type_reflection(ShortTypeCode short_type_hash)
{
    auto ii = short_code_map().find(short_type_hash.u64());
    if (ii != short_code_map().end()) {
        return *ii->second;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Type with typecode {} is not regitered", short_type_hash.u64()).do_throw();
    }
}

TypeReflection& get_type_reflection(const UID256& type_hash)
{
    const auto& map = type_code_map();

    auto ii = map.find(type_hash);
    if (ii != map.end()) {
        return *ii->second;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Type with typecode {} is not regitered", type_hash).do_throw();
    }
}


bool has_type_reflection(const UID256& type_hash)
{
    auto ii = type_code_map().find(type_hash);
    return ii != type_code_map().end();
}


bool has_type_reflection(ShortTypeCode short_type_hash)
{
    auto ii = short_code_map().find(short_type_hash.u64());
    return ii != short_code_map().end();
}

void register_type_reflection(TypeReflection& type_reflection)
{
    auto short_type_hash = type_reflection.shot_type_hash();

    //println("Register reflection {} for type {}, code {}", short_type_hash.u64(), short_type_hash.code(), type_reflection.str());

    auto ii = short_code_map().find(short_type_hash.u64());
    if (ii == short_code_map().end()) {
        short_code_map()[short_type_hash.u64()] = type_reflection.self();
    }
    else {
        //MEMORIA_MAKE_GENERIC_ERROR("Type with typcode {} has been already regitered", short_type_hash.u64()).do_throw();
    }
}

void register_type_reflection(const UID256& type_code, TypeReflection& type_reflection) {
    type_code_map()[type_code] = type_reflection.self();
}

void register_type_reflection_256(TypeReflection& type_reflection) {

    auto datatype = hermes::HermesCtr::parse_datatype(type_reflection.str())->root().as_datatype();

    auto hash = datatype.cxx_type_hash();
    register_type_reflection(hash, *type_reflection.self());
}



void for_each_type_reflection(std::function<void (const UID256&, TypeReflection&)> fn)
{
    for (auto& pair: short_code_map()) {
        fn(pair.first, *pair.second.get());
    }
}


void for_each_type_reflection(std::function<void (const ShortTypeCode&, TypeReflection&)> fn)
{
    for (auto& pair: short_code_map()) {
        fn(ShortTypeCode::of_raw(pair.first), *pair.second.get());
    }
}


std::ostream& operator<<(std::ostream& os, const IDValue& id) {
    os << id.str();
    return os;
}


hermes::Object TypeReflection::datatype_convert_to(
        LWMemHolder*,
        ShortTypeCode target_tag,
        hermes::ValueStorageTag vs_tag,
        hermes::ValueStorage& ptr
) const {
    MEMORIA_MAKE_GENERIC_ERROR("Type {} is not convertible to {}",
        str(), get_type_reflection(target_tag).str()
    ).do_throw();
}

hermes::Object TypeReflection::datatype_convert_from_plain_string(U8StringView) const {
    MEMORIA_MAKE_GENERIC_ERROR("Type {} is not convertible from plain string", str()).do_throw();
}

U8String TypeReflection::convert_to_plain_string(
        LWMemHolder*,
        hermes::ValueStorageTag vs_stag,
        hermes::ValueStorage& storage
) const
{
    MEMORIA_MAKE_GENERIC_ERROR("Type {} is not convertible to plain string", str()).do_throw();
}

hermes::Object TypeReflection::import_value(
        LWMemHolder*,
        hermes::ValueStorageTag, hermes::ValueStorage&
) const {
    MEMORIA_MAKE_GENERIC_ERROR("Importing value in not supported for type {}", str()).do_throw();
}


U8StringView get_datatype_name(U8StringView name) {
    size_t pos = name.find_last_of(':');
    if (pos == name.npos) {
        return name;
    }
    else {
        return name.substr(pos + 1);
    }
}

Any get_cxx_instance(const hermes::Datatype& typedecl) {
    UID256 hash = typedecl.cxx_type_hash();
    return get_type_reflection(hash).create_cxx_instance(typedecl);
}

}
