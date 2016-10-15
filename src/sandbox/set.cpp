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

#include <memoria/v1/core/tools/bytes/bytes.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <memory>

using namespace memoria::v1;
using namespace std;




int main() {
    MEMORIA_INIT(DefaultProfile<>);

    using Key = Bytes;

    DInit<Set<Key>>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp = alloc->master()->branch();

        try {
            auto ctr = create<Set<Key>>(snp);

            Bytes bb(4);

            for (size_t cc = 0; cc < bb.size(); cc++)
            {
                bb[cc] = 0;
            }

            cout << bb << endl;

            cout<<setbase(16);
            for (size_t c = 0; c < 4; c++)
            {
                cout<<setw(2)<<setfill('0');
                cout << 1;
            }

            cout << endl;


            int from = 0;
            int to   = 10000;

            for (int c = from; c < to; c++)
            {
                auto size = getRandomG(10);
                Bytes data(size);

                for (size_t d = 0; d < data.size(); d++)
                {
                    data[d] = getRandomG(256);
                }

                ctr->insert_key(data);
            }

            FSDumpAllocator(snp, "set_orig.dir");

//            int cnt = 0;
//
//            for (int c = from; c < to; c++) {
//              cnt += ctr->contains(c);
//            }
//
//            cout << "Contains: " << cnt << endl;
        }
        catch (...) {
            FSDumpAllocator(snp, "setx.dir");
            throw;
        }
    }
    catch (Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
