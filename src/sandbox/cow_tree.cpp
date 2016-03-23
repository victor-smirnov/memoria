// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


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
    CoWTree<BigInt, BigInt> tree;

    Int threads_num;

    if (argc > 1)
    {
        threads_num = strToL(String(argv[1]));
    }
    else {
        threads_num = 1;
    }

    cout<<"Using threads: "<<threads_num<<endl;

    vector<BigInt> operations;
    vector<BigInt> founds;

    vector<std::thread> threads;

    std::mutex mutex;

    bool run = true;

    try {
        long tc0 = getTimeInMillis();

        auto tx0 = tree.transaction();

        RNG<Int, RngEngine32> rng0;

        rng0.seed(getTimeInMillis());

        for (Int c = 0; c < 5000000; c++) {
            tree.assign(tx0, rng0(), c);
        }

        tx0.commit();

        cout<<"Tree created in "<<FormatTime(getTimeInMillis() - tc0)<<endl;

        tree.dump_log();

        const Int epochs = 20;

        for (Int c = 0; c < threads_num; c++)
        {
            operations.emplace_back(0);
            founds.emplace_back(0);
            threads.emplace_back(std::thread([&, c]() {
                try {
                    RNG<Int, RngEngine32> rng;
                    rng.seed(c);

                    for (BigInt epoch = 0; epoch < epochs && run; epoch++)
                    {
                        Int lfounds = 0;

                        BigInt t0 = getTimeInMillis();

                        for (int j = 0; j < 1000; j++)
                        {
                            auto sn = tree.snapshot();
                            for (Int i = 0; i < 1000; i++)
                            {
                                lfounds += tree.find(sn, rng());
                            }
                        }

                        BigInt t1 = getTimeInMillis();

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

        for (Int s = 0; s < 500; s++)
        {
            auto tx1 = tree.transaction();

            for (Int c = 0; c < 10000; c++) {
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

        BigInt total = 0;
        BigInt total_founds = 0;

        for (auto c = 0; c < (Int)operations.size(); c++)
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
