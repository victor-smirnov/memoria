
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




DataTypeRegistryStore::Initializer<Decimal, TL<int64_t, int64_t>, TL<int64_t>, TL<>> decimal;
DataTypeRegistryStore::Initializer<Dynamic<Decimal>, TL<>> dynamic_decimal;
DataTypeRegistryStore::Initializer<BigDecimal, TL<int64_t, int64_t>, TL<int64_t>, TL<>> big_decimal;
DataTypeRegistryStore::Initializer<Dynamic<BigDecimal>, TL<>> dynamic_big_decimal;




}}
