// Copyright 2016 Victor Smirnov
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




#include <memoria/v1/memoria.hpp>
#include <memoria/v1/containers/multimap/mmap_factory.hpp>
#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/containers/set/set_factory.hpp>

#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <algorithm>
#include <vector>
#include <type_traits>


using namespace memoria::v1;
using namespace memoria::v1::btss;
using namespace std;



int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using Key = BigInt;
    using Value = UByte;
    using CtrT = Map<Key, Vector<Value>>;

    DInit<CtrT>();

    DInit<memoria::v1::Set<Bytes>>();
    DInit<memoria::v1::Map<BigInt, Bytes>>();
    DInit<memoria::v1::Set<FixedArray<20>>>();
//    DInit<memoria::v1::Map<U8String, U8String>>();
    DInit<memoria::v1::Map<BigInt, Vector<UByte>>>();



    try {
    	auto alloc = PersistentInMemAllocator<>::load("data.memoria");

    	alloc->dump("alloc.dir");

    	auto snp   = alloc->find_branch("DICTIONARY-0")->branch();

    	auto map = find<CtrT>(snp, UUID::parse("d4a7f47c-d057-44c2-bf0c-fcf88a974de5"));

    	std::vector<Value> values(709);

    	map->find_or_create(-1560192589, values.begin(), values.end());

    	snp->dump("after.dir");

//    	map->begin()->remove(1);
//    	map->begin()->dump();
//    	map->find_or_create(1111, values.begin(), values.end());

//    	map->begin()->dump();
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
