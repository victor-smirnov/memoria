// Copyright 2015 Victor Smirnov
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



#include <memoria/v1/core/tools/cow_tree/cow_tree.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <vector>
#include <thread>

using namespace memoria;
using namespace v1::cow::tree;
using namespace std;

int main(int argc, const char** argv, const char** envp)
{
    CoWTree<int64_t, int64_t> tree;

    int32_t threads_num;

    if (argc > 1)
    {
        threads_num = strToL(String(argv[1]));
    }
    else {
        threads_num = 1;
    }

    cout<<"Using threads: "<<threads_num<<endl;

    vector<int64_t> operations;
    vector<int64_t> founds;

    vector<std::thread> threads;

    std::mutex mutex;

    bool run = true;

    try {
        long tc0 = getTimeInMillis();

        auto tx0 = tree.transaction();

        RNG<int32_t, RngEngine32> rng0;

        rng0.seed(getTimeInMillis());

        for (int32_t c = 0; c < 5000000; c++) {
            tree.assign(tx0, rng0(), c);
        }

        tx0.commit();

        cout<<"Tree created in "<<FormatTime(getTimeInMillis() - tc0)<<endl;

        tree.dump_log();

        const int32_t epochs = 20;

        for (int32_t c = 0; c < threads_num; c++)
        {
            operations.emplace_back(0);
            founds.emplace_back(0);
            threads.emplace_back(std::thread([&, c]() {
                try {
                    RNG<int32_t, RngEngine32> rng;
                    rng.seed(c);

                    for (int64_t epoch = 0; epoch < epochs && run; epoch++)
                    {
                        int32_t lfounds = 0;

                        int64_t t0 = getTimeInMillis();

                        for (int j = 0; j < 1000; j++)
                        {
                            auto sn = tree.snapshot();
                            for (int32_t i = 0; i < 1000; i++)
                            {
                                lfounds += tree.find(sn, rng());
                            }
                        }

                        int64_t t1 = getTimeInMillis();

                        operations[c] += t1 - t0;
                        founds[c]     += lfounds;
                    }

                    cout<<"Thread "<<c<<" finished"<<endl;
                }
                catch (std::exception& ex) {
                    cout<<"Exception: "<<ex.what()<<endl;
                }
            }));
        }

        for (int32_t s = 0; s < 500; s++)
        {
            auto tx1 = tree.transaction();

            for (int32_t c = 0; c < 10000; c++) {
                tree.assign(tx1, rng0(), c);
            }

            tx1.commit();
        }

        cout<<"Ingestion is done"<<endl;

        for (auto& t : threads) {
            t.join();
        }

        tree.cleanup_snapshots();

        tree.dump_log();

        int64_t total = 0;
        int64_t total_founds = 0;

        for (auto c = 0; c < (int32_t)operations.size(); c++)
        {
            total += operations[c] / epochs;
            total_founds += founds[c];
        }

        cout<<"Total average thread time: "<<FormatTime(total/threads_num)<<endl;
        cout<<"Total founds: "<<total_founds<<endl;

        cout<<"Done..."<<endl;
    }
    catch (std::exception& ex) {
        cout<<ex.what()<<endl;
    }
}
