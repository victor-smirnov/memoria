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

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memory>
#include <vector>

using namespace memoria::v1;
using namespace std;

using MMapIOBuffer = DefaultIOBuffer;

struct KeyConsumer: BufferConsumer<MMapIOBuffer> {

	int total_ = 0;

	virtual int32_t process(MMapIOBuffer& buffer, int32_t entries)
	{
		for (int32_t e = 0; e < entries; e++) {
			buffer.getBigInt();
			total_++;
		}

		buffer.done();

		return entries;
	}
};

int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using KeyType   = int64_t;
    using ValueType = uint8_t;

    using CtrName = Map<KeyType, Vector<ValueType>>;

    DInit<CtrName>();

    try {
        auto alloc = PersistentInMemAllocator<>::load("mmap.memoria");
        auto snp   = alloc->master()->branch();

        auto map = find<CtrName>(snp, UUID::parse("a962d32d-254f-4180-9c73-f5666c37fd64"));

        auto iter = map->begin();

        KeyConsumer consumer;

        int64_t t0 = getTimeInMillis();

        iter->read_keys(&consumer);

        int64_t t1 = getTimeInMillis();

        cout << "total keys: " << consumer.total_ << ", time = " << (t1 - t0) << endl;
    }
    catch (::memoria::v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }
    catch (::memoria::v1::PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
