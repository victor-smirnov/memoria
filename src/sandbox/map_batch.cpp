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

#include <memoria/v1/containers/map/map_factory.hpp>
#include <memoria/v1/containers/seq_dense/seqd_factory.hpp>
#include <memoria/v1/containers/vector/vctr_factory.hpp>

#include <memoria/v1/core/container/metadata_repository.hpp>

#include <memoria/v1/core/tools/time.hpp>

#include <memory>
#include <vector>

using namespace memoria;
using namespace std;

template <typename T>
using SVector = shared_ptr<vector<T>>;





int main() {
    MEMORIA_INIT(DefaultProfile<>);

    cout<<"Field Factory: "<<HasFieldFactory<double>::Value<<endl;
    cout<<"ValueCodec: "<<HasValueCodec<double>::Value<<endl;



    using KeyType = String;
    using ValueType = String;

    DInit<Map<KeyType, ValueType>>();

    using Entry = pair<KeyType, ValueType>;


    ListPrinter<TL<int8_t, char, uint8_t>>::print(cout)<<endl;

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp   = alloc->master()->branch();

        auto map = create<Map<KeyType, ValueType>>(snp);


        auto data = make_shared<vector<Entry>>(100000);

        for (size_t c = 0; c < data->size(); c++)
        {
//          data->operator[](c) = Entry("str_"+toString(c), c);
            data->operator[](c) = Entry("key_"+toString(c), "val_"+toString(c));

            //data->operator[](c) = Entry(c, c);
        }



        BigInt t0 = getTimeInMillis();

        cout << "Inserted: " << map->end()->bulk_insert(data->begin(), data->end()) << endl;
        BigInt t1 = getTimeInMillis();
        cout << "Insertion time: " << FormatTime(t1 - t0) << " s" << endl;

        FSDumpAllocator(snp, "map_batch_data.dir");


        vector<Entry> data2;
        auto inserter = back_inserter(data2);

        map->begin()->read(inserter, map->size());

        cout << "Entries data: " << endl;

        for (auto& entry: data2) {
            cout << entry.first << " +++ " << entry.second << endl;
        }


        cout << "Removed: " << map->seek(map->size() / 2)->remove(map->size()) << endl;

        cout << "Ater remove: " << map->size() << endl;

        snp->commit();




        BigInt t2 = getTimeInMillis();
        check_snapshot(snp);
        BigInt t3 = getTimeInMillis();
        cout << "Check time: " << FormatTime(t3 - t2) << " s" << endl;



        BigInt t4 = getTimeInMillis();
        unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create("map_batch_data.dump"));
        alloc->store(out.get());
        BigInt t5 = getTimeInMillis();
        cout << "Dump time: " << FormatTime(t5 - t4) << " s" << endl;
    }
    catch (v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    catch (v1::PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
