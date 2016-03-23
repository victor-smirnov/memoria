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
using namespace memoria::cow::tree;
using namespace std;

int main(int argc, const char** argv, const char** envp)
{
    CoWTree<BigInt, BigInt> tree;

    try {
        long tc0 = getTimeInMillis();

        auto tx0 = tree.transaction();

        RNG<Int, RngEngine32> rng0;

        rng0.seed(getTimeInMillis());

        for (Int c = 0; c < 100000; c++) {
            tree.assign(tx0, rng0(), c);
        }

        tx0.commit();

        cout<<"Tree created in "<<FormatTime(getTimeInMillis() - tc0)<<endl;

        tree.dump_log();

        for (Int s = 0; s < 10; s++)
        {
            auto tx1 = tree.transaction();

            for (Int c = 0; c < 1000; c++) {
                tree.assign(tx1, rng0() , c);
            }

            tx1.commit();
        }

        tree.dump_log();
        tree.check_log();


        cout<<"Done..."<<endl;
    }
    catch (std::exception& ex) {
        cout<<ex.what()<<endl;
    }
}
