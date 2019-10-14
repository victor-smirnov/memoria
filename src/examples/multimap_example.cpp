
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


#include <memoria/v1/api/datatypes/type_signature.hpp>
#include <memoria/v1/api/datatypes/sdn.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/type_registry.hpp>

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/core/tools/time.hpp>

#include <memoria/v1/memoria.hpp>


#include <iostream>

using namespace memoria::v1;

int main()
{
    StaticLibraryCtrs<>::init();

    try {

        using MultimapType = Multimap<Varchar, Varchar>;
        using Entry   = std::pair<U8String, std::vector<U8String>>;

        auto alloc = IMemoryStore<>::create();

        auto snp = alloc->master()->branch();

        auto ctr0 = create(snp, MultimapType());

        ctr0->set_new_block_size(64*1024);


        int64_t t0 = getTimeInMillis();

        std::vector<Entry> entries_;

        for (int c = 0; c < 100000; c++) {

            std::vector<U8String> values;

            for (int d = 0; d < 100; d++) {
                values.push_back("BBBBBBBBBBBBBBBBBBBBB___BBBBBBBB__" + std::to_string(d) + "__" + std::to_string(c));
            }

            entries_.emplace_back(Entry(
                "AAAAAAAAAAAAAAAA__" + std::to_string(c),
                std::move(values)
            ));
        }

        int64_t t1 = getTimeInMillis();

        std::cout << "Populated entries in " << (t1 - t0) << " ms" << std::endl;

        int64_t t0_i = getTimeInMillis();

        ctr0->append_entries([&](auto& seq, auto& keys, auto& values, auto& sizes) {

            size_t batch_start = sizes.entries_;

            size_t batch_size = 8192;
            size_t limit = (batch_start + batch_size <= entries_.size()) ?
                        batch_size : entries_.size() - batch_start;

            for (size_t c = 0; c < limit; c++)
            {
                seq.append(0, 1);
                keys.append(std::get<0>(entries_[batch_start + c]));

                auto& data = std::get<1>(entries_[batch_start + c]);

                seq.append(1, data.size());
                values.append(data);
            }

            sizes.entries_ += limit;

            return limit != batch_size;
        });

        int64_t t1_i = getTimeInMillis();

        std::cout << "Inserted entries in " << (t1_i - t0_i) << " ms" << std::endl;
        std::cout << "Size = " << ctr0->size() << std::endl;


        snp->commit();
        snp->set_as_master();
        alloc->store("store_multimap_ex.mma1");


        int64_t t2 = getTimeInMillis();

        auto ii = ctr0->entries_scanner(ctr0->seek(0));

        size_t sum0 = 0;

        //ii->dump_iterator();

        ii->for_each([&](auto key, auto values){
            sum0 += values.size();

//            std::cout << key << " -> " << values.size() << std::endl;

//            for (auto& vv: values) {
//                std::cout << "\t" << vv << std::endl;
//            }
        });


        int64_t t3 = getTimeInMillis();

        std::cout << "Iterated over entries in " << (t3 - t2) << " ms " << sum0 << std::endl;


        auto keys = ctr0->keys();

        while (!keys->is_end())
        {
            auto kk = keys->keys();
            for (size_t c = 0; c < kk.size(); c++)
            {
                //std::cout << kk[c] << std::endl;

                auto vals = keys->values(c);

                size_t dd = 0;
                while (!vals->is_end())
                {
                    for (auto& vv: vals->values())
                    {
//                        std::cout << "\t" << dd << ") " << vv << std::endl;
                        dd++;
                    }

                    if (!vals->is_run_finished())
                    {
                        vals->next_block();
                    }
                    else {
                        break;
                    }
                }
            }

            keys->next();
        }

    }
    catch (MemoriaThrowable& th) {
        th.dump(std::cout);
    }


    return 0;
}
