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


template <typename Iter>
class SetBufferProducer: public BufferProducer<IOBuffer> {
    using Base = BufferProducer<IOBuffer>;

    Iter iter_;
    Iter end_;

    using Key   = std::decay_t<decltype(*std::declval<Iter>())>;

    IOBuffer buffer_;

public:
    SetBufferProducer(const Iter& begin, const Iter& end, size_t buffer_size):
        iter_(begin), end_(end), buffer_(buffer_size)
    {}

    virtual IOBuffer& buffer() {return buffer_;}

    virtual Int populate(IOBuffer& buffer)
    {
        Int entries = 0;

        while (iter_ != end_)
        {
            if (!IOBufferAdapter<Key>::put(buffer, *iter_))
            {
                return entries;
            }

            entries++;
            iter_++;
        }

        return -entries;
    }
};





int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using Key   = FixedArray<16>;

    DInit<Set<Key>>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();

        auto snp = alloc->master()->branch();

        auto map = create<Set<Key>>(snp);
        map->setNewPageSize(65536);

        int size = 30000000;

        using KeyVector = vector<Key>;

        KeyVector pairs;

        BigInt ts0 = getTimeInMillis();

        for (int c = 0; c < size; c++)
        {
            FixedArray<16> array;

            for (int c = 0; c < array.length(); c++)
            {
                array[c] = getRandomG(256);
            }

            pairs.emplace_back(array);
        }

        BigInt ts1 = getTimeInMillis();

        std::sort(pairs.begin(), pairs.end());

        BigInt ts2 = getTimeInMillis();

        cout << "Vector creation: " << FormatTime(ts1 - ts0) << ", sorting: " << FormatTime(ts2 - ts1) << endl;

        SetBufferProducer<KeyVector::const_iterator> producer(pairs.begin(), pairs.end(), 65536);

        BigInt t0 = getTimeInMillis();
        map->begin()->insert_iobuffer(&producer);

        cout << "Insertion time: " << FormatTime(getTimeInMillis() - t0) <<" size: " << map->size() << endl;


        // Finish snapshot so no other updates are possible.
        snp->commit();

//        FSDumpAllocator(snp, "setl_full.dir");

        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("setl_data.dump");
        alloc->store(out.get());
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
