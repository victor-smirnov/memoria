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

    try {
        long tc0 = getTimeInMillis();

        auto tx0 = tree.transaction();

        RNG<int32_t, RngEngine32> rng0;

        rng0.seed(getTimeInMillis());

        for (int32_t c = 0; c < 100000; c++) {
            tree.assign(tx0, rng0(), c);
        }

        tx0.commit();

        cout<<"Tree created in "<<FormatTime(getTimeInMillis() - tc0)<<endl;

        tree.dump_log();

        for (int32_t s = 0; s < 10; s++)
        {
            auto tx1 = tree.transaction();

            for (int32_t c = 0; c < 1000; c++) {
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
