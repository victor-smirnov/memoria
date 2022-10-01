
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

#include <memory>

namespace memoria {

using ShortCodeMap = ska::flat_hash_map<uint64_t, std::shared_ptr<TypeReflection>>;

namespace {

ShortCodeMap& short_code_map() {
    static ShortCodeMap map;
    return map;
}

template <typename T>
class HermesTypeReflectionImpl: public TypeReflection {};

}

TypeReflection& get_type_reflection(uint64_t short_type_hash)
{
    auto ii = short_code_map().find(short_type_hash);
    if (ii != short_code_map().end()) {
        return *ii->second;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Type with typcode {} is not regitered", short_type_hash).do_throw();
    }
}

bool has_type_reflection(uint64_t short_type_hash)
{
    auto ii = short_code_map().find(short_type_hash);
    return ii != short_code_map().end();
}

void register_type_reflection(std::unique_ptr<TypeReflection> type_reflection)
{
    uint64_t short_type_hash = type_reflection->shot_type_hash();

    //println("Register reflection {} for type {}", short_type_hash, type_reflection->str());

    auto ii = short_code_map().find(short_type_hash);
    if (ii == short_code_map().end()) {
        short_code_map()[short_type_hash] = std::move(type_reflection);
    }
    else {
        //MEMORIA_MAKE_GENERIC_ERROR("Type with typcode {} has been already regitered", short_type_hash).do_throw();
    }
}

std::ostream& operator<<(std::ostream& os, const IDValue& id) {
    os << id.str();
    return os;
}

}
