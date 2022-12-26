
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/datatypes/type_registry.hpp>
#include <memoria/core/datatypes/datum.hpp>

#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/fixed_array.hpp>
#include <memoria/core/strings/u8_string.hpp>

#include <memoria/api/map/map_api.hpp>
#include <memoria/api/multimap/multimap_api.hpp>
#include <memoria/api/vector/vector_api.hpp>
#include <memoria/api/set/set_api.hpp>


namespace memoria {
/*
AnyDatum DataTypeRegistry::from_sdn_string(U8StringView sdn_string) const
{
    auto sdn_doc = LDDocument::parse(sdn_string);

    auto value = sdn_doc->value();

    if (value->is_null()) {
        return AnyDatum();
    }
    else
    {
        U8String typedecl;
        if (value->is_varchar())
        {
            typedecl = make_datatype_signature_string<Varchar>();
        }
        else if (value->is_bigint()) {
            typedecl = make_datatype_signature_string<BigInt>();
        }
        else if (value->is_double()) {
            typedecl = make_datatype_signature_string<Double>();
        }
        else if (value->is_boolean()) {
            typedecl = make_datatype_signature_string<Boolean>();
        }
        else if (value->is_typed_value())
        {
            auto tv = value->as_typed_value();
            typedecl = tv->type()->to_standard_string();
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex("Unsupported data type requested");
        }


        auto ii = creators_.find(typedecl);
        if (ii != creators_.end())
        {
            auto& fn = std::get<1>(ii->second);

            if (fn) {
                return fn(*this, *sdn_doc);
            }
            else {
                MMA_THROW(RuntimeException()) << format_ex("Value deserializer for {} is not registered", typedecl);
            }
        }
        else {
            MMA_THROW(RuntimeException()) << format_ex("Type creator for {} is not registered", typedecl);
        }
    }
}


DataTypeRegistry::DataTypeRegistry() {
    refresh();
}

void DataTypeRegistry::refresh()
{
    creators_.clear();
    DataTypeRegistryStore::global().copy_to(*this);
}


Optional<std::shared_ptr<DataTypeOperations>> DataTypeRegistry::get_operations(uint64_t type_code)
{
    auto ii = operations_by_code_.find(type_code);
    if (ii != operations_by_code_.end())
    {
        return ii->second;
    }

    return Optional<std::shared_ptr<DataTypeOperations>>{};
}

Optional<std::shared_ptr<DataTypeOperations>> DataTypeRegistry::get_operations(U8StringView cxx_typedecl)
{
    auto ii = operations_.find(cxx_typedecl);
    if (ii != operations_.end())
    {
        return ii->second;
    }

    return Optional<std::shared_ptr<DataTypeOperations>>{};
}


DataTypeRegistryStore& DataTypeRegistryStore::global()
{
    static DataTypeRegistryStore type_registry_store_;
    return type_registry_store_;
}

DataTypeRegistry& DataTypeRegistry::local()
{
    static thread_local DataTypeRegistry type_registry_;
    return type_registry_;
}
*/

}
