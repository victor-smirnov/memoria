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


class MMapBufferConsumer: public bt::BufferConsumer<MMapIOBuffer> {
    using IOBuffer = MMapIOBuffer;

    IOBuffer io_buffer_;
public:
    MMapBufferConsumer(): io_buffer_(65536) {}

    virtual IOBuffer& buffer() {return io_buffer_;}
    virtual int32_t process(IOBuffer& buffer, int32_t entries)
    {
//      cout << "Consume " << entries << " entries" << endl;
        return entries;
    }
};



template <typename IOBufferT, typename Iterator>
class ChainedIOBufferProducer: public BufferProducer<IOBufferT> {

    using WalkerType = btfl::io::BTFLWalker<Iterator, IOBufferT>;

    Iterator* iter_;
    WalkerType walker_;
    IOBufferT io_buffer_;

public:
    ChainedIOBufferProducer(Iterator* iter, int32_t buffer_size = 65536):
        iter_(iter),
        walker_(*iter),
        io_buffer_(buffer_size)
    {
    }

    virtual IOBufferT& buffer() {
        return io_buffer_;
    }

    virtual int32_t populate(IOBufferT& buffer)
    {
        return iter_->bulkio_populate(walker_, &io_buffer_);
    }
};




int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    using KeyType   = int64_t;
    using ValueType = uint64_t;//FixedArray<32>;

    using CtrName = Map<KeyType, Vector<ValueType>>;

    DInit<CtrName>();

    try {
        auto alloc = PersistentInMemAllocator<>::load("mmap.memoria");
        auto snp   = alloc->master()->branch();

        auto map = find<CtrName>(snp, UUID(10000, 20000));


        auto map2 = create<CtrName>(snp);

        map2->setNewPageSize(32768);

        auto iter = map->begin();
        using CtrT = decltype(map)::element_type;

        ChainedIOBufferProducer<MMapIOBuffer, CtrT::Iterator> chained_producer(iter.get(), 65536);

        long ti0 = getTimeInMillis();
        auto totals2 = map2->begin()->bulkio_insert(chained_producer);
        long ti1 = getTimeInMillis();

        cout << "Totals: " << totals2 << ", time " << (ti1 - ti0) << endl;

        snp->commit();
    }
    catch (::memoria::v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }
    catch (::memoria::v1::PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
