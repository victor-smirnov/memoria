
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

#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>

#include <memoria/v1/api/map/map_api.hpp>
#include <memoria/v1/api/multimap/multimap_api.hpp>
#include <memoria/v1/api/vector/vector_api.hpp>
#include <memoria/v1/api/db/update_log/update_log_api.hpp>
#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>
#include <memoria/v1/api/set/set_api.hpp>

namespace memoria {
namespace v1 {

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

DataTypeRegistryStore::Initializer<Decimal, TL<int64_t, int64_t>, TL<int64_t>, TL<>> decimal;
DataTypeRegistryStore::Initializer<Dynamic<Decimal>, TL<>> dynamic_decimal;
DataTypeRegistryStore::Initializer<BigDecimal, TL<int64_t, int64_t>, TL<int64_t>, TL<>> big_decimal;
DataTypeRegistryStore::Initializer<Dynamic<BigDecimal>, TL<>> dynamic_big_decimal;


DataTypeRegistryStore::Initializer<FixedArray<4>, TL<>> fixed_array_4;
DataTypeRegistryStore::Initializer<FixedArray<8>, TL<>> fixed_array_8;
DataTypeRegistryStore::Initializer<FixedArray<16>, TL<>> fixed_array_16;
DataTypeRegistryStore::Initializer<FixedArray<32>, TL<>> fixed_array_32;
DataTypeRegistryStore::Initializer<FixedArray<64>, TL<>> fixed_array_64;
DataTypeRegistryStore::Initializer<FixedArray<128>, TL<>> fixed_array_128;
DataTypeRegistryStore::Initializer<FixedArray<256>, TL<>> fixed_array_256;
DataTypeRegistryStore::Initializer<FixedArray<512>, TL<>> fixed_array_512;
DataTypeRegistryStore::Initializer<FixedArray<1024>, TL<>> fixed_array_1024;

DataTypeRegistryStore::Initializer<U8String, TL<>> u8_string;
DataTypeRegistryStore::Initializer<uint8_t, TL<>> uint8_;
DataTypeRegistryStore::Initializer<int64_t, TL<>> int64_;
DataTypeRegistryStore::Initializer<uint64_t, TL<>> uint64_;


DataTypeRegistryStore::Initializer<UpdateLog, TL<>> update_log_;
DataTypeRegistryStore::Initializer<EdgeMap, TL<>> edge_map;

DataTypeRegistryStore::Initializer<Map<UUID, UUID>, TL<>> root_map;

DataTypeRegistryStore::Initializer<Map<U8String, U8String>, TL<>> string_string_map;

DataTypeRegistryStore::Initializer<Multimap<int64_t, uint8_t>, TL<>> llb_multimap;

DataTypeRegistryStore::Initializer<Map<UUID, uint8_t>, TL<>> uub_multimap;

DataTypeRegistryStore::Initializer<Set<FixedArray<16>>, TL<>> fixed_16_set;

}}
