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
#include <memoria/v1/core/tools/time.hpp>

#include <memory>
#include <memoria/v1/core/tools/strings/string_wrapper_codec.hpp>

using namespace memoria::v1;
using namespace std;

int main() {
    MEMORIA_INIT(DefaultProfile<>);

    using Key = StringWrapper;

    DInit<Set<Key>>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp = alloc->master()->branch();

        try {
            auto ctr = create<Set<Key>>(snp);

            ctr->setNewPageSize(65536);

            int from = 0;
            int to   = 100 * 1024 * 1024;

            char chars[256];

            BigInt t0 = getTimeInMillis();
            BigInt tt = t0;

            int threshold_value = 100000;
            int threshold = threshold_value;


            for (int c = from; c < to; c++)
            {
            	auto size = getRandomG(10) + 25;
            	StringWrapper data(chars, size, false);

            	for (size_t d = 0; d < data.size(); d++)
            	{
            		chars[d] = getRandomG(10) + 48;
            	}

                ctr->insert_key(data);

                if (c == threshold)
                {
                	long tx = getTimeInMillis();

                	cout << c << " in " << FormatTime(tx - tt) << endl;

                	tt = tx;
                	threshold += threshold_value;
                }
            }

            BigInt t1 = getTimeInMillis();

            cout << "Time " << FormatTime(t1 - t0)<< endl;

            snp->commit();

            auto out = FileOutputStreamHandler::create("sets_data.dump");
            alloc->store(out.get());

            //FSDumpAllocator(snp, "sets_orig.dir");

        }
        catch (...) {
//            FSDumpAllocator(snp, "setx.dir");
            throw;
        }
    }
    catch (Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
