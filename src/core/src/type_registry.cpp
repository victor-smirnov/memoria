
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


#include <memoria/v1/api/datatypes/type_registry.hpp>
#include <memoria/v1/api/datatypes/datum.hpp>

#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>

#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/api/multimap/multimap_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>
//#include <memoria/v1/api/db/update_log/update_log_api.hpp>
//#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>
#include <memoria/v1/api/set/set_api.hpp>

namespace memoria {
namespace v1 {

MMA1_DEFINE_EXPLICIT_CU_LINKING(Datatypes)

AnyDatum DataTypeRegistry::from_sdn_string(U8StringView sdn_string) const
{
    SDNDocument sdn_doc = SDNDocument::parse(sdn_string);

    U8String typedecl;

    switch (sdn_doc.value().type())
    {
        case SDNValueType::DOUBLE : typedecl = make_datatype_signature_string<Double>(); break;
        case SDNValueType::LONG :   typedecl = make_datatype_signature_string<BigInt>(); break;
        case SDNValueType::TYPED_STRING_VALUE :
        {
            const TypedStringValue& str_val = boost::get<TypedStringValue>(sdn_doc.value().value());

            if (str_val.has_type())
            {
                typedecl = str_val.type().to_typedecl_string();
            }
            else {
                typedecl = make_datatype_signature_string<Varchar>();
            }

            break;
        }
        case SDNValueType::NAME_TOKEN :
        {
            const NameToken& token = boost::get<NameToken>(sdn_doc.value().value());

            if (token.is_null_token()) {
                return AnyDatum();
            }
            else {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(
                               u"Unsupported name token requested: {}",
                               token.text()
                           );
            }
        }

        default: MMA1_THROW(RuntimeException())
                << fmt::format_ex(
                       u"Unsupported data type requested: {}",
                       static_cast<int>(sdn_doc.value().type())
                   );
    }

    auto ii = creators_.find(typedecl);
    if (ii != creators_.end())
    {
        auto& fn = std::get<1>(ii->second);

        if (fn) {
            return fn(*this, sdn_doc);
        }
        else {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Value deserializer for {} is not registered", typedecl);
        }
    }
    else {
        MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Type creator for {} is not registered", typedecl);
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

DataTypeRegistryStore::Initializer<TinyInt, TL<>> tiny_int;
DataTypeRegistryStore::Initializer<SmallInt, TL<>> small_int;
DataTypeRegistryStore::Initializer<Integer, TL<>> integer_;
DataTypeRegistryStore::Initializer<BigInt, TL<>> bigint_;
DataTypeRegistryStore::Initializer<Real, TL<>> real_;
DataTypeRegistryStore::Initializer<Double, TL<>> double_;
DataTypeRegistryStore::Initializer<Varchar, TL<>> varchar;
DataTypeRegistryStore::Initializer<Timestamp, TL<>> timestamp;
DataTypeRegistryStore::Initializer<TSWithTimeZone, TL<>> ts_with_tz;
DataTypeRegistryStore::Initializer<Date, TL<>> date;
DataTypeRegistryStore::Initializer<Time, TL<>> time;
DataTypeRegistryStore::Initializer<TimeWithTimeZone, TL<>> time_with_tz;
DataTypeRegistryStore::Initializer<UUID, TL<>> uuid;


DataTypeRegistryStore::Initializer<Map<Varchar, Varchar>, TL<>> map_varchar_varchar;
DataTypeRegistryStore::Initializer<Map<UUID, UUID>, TL<>> map_uuid_uuid;

DataTypeRegistryStore::Initializer<Multimap<Varchar, Varchar>, TL<>> multimap_varchar_varchar;

//DataTypeRegistryStore::Initializer<Decimal, TL<int64_t, int64_t>, TL<int64_t>, TL<>> decimal;
//DataTypeRegistryStore::Initializer<Dynamic<Decimal>, TL<>> dynamic_decimal;
//DataTypeRegistryStore::Initializer<BigDecimal, TL<int64_t, int64_t>, TL<int64_t>, TL<>> big_decimal;
//DataTypeRegistryStore::Initializer<Dynamic<BigDecimal>, TL<>> dynamic_big_decimal;


DataTypeRegistryStore::Initializer<FixedArray<4>, TL<>> fixed_array_4;
DataTypeRegistryStore::Initializer<FixedArray<8>, TL<>> fixed_array_8;
DataTypeRegistryStore::Initializer<FixedArray<16>, TL<>> fixed_array_16;
DataTypeRegistryStore::Initializer<FixedArray<32>, TL<>> fixed_array_32;
DataTypeRegistryStore::Initializer<FixedArray<64>, TL<>> fixed_array_64;
DataTypeRegistryStore::Initializer<FixedArray<128>, TL<>> fixed_array_128;
DataTypeRegistryStore::Initializer<FixedArray<256>, TL<>> fixed_array_256;
DataTypeRegistryStore::Initializer<FixedArray<512>, TL<>> fixed_array_512;
DataTypeRegistryStore::Initializer<FixedArray<1024>, TL<>> fixed_array_1024;


}}
