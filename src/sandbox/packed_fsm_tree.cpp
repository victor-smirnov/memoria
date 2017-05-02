
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



#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/uuid.hpp>


#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

#include <iostream>

#include <cstddef>

using namespace memoria;
using namespace std;





int main() {

    try {
        constexpr int32_t Block = 0;
        constexpr int32_t Blocks = Block + 1;


        using Tree = PkdFMTreeT<UUID, Blocks>;

//      using Values = Tree::Values;

        int32_t block_size = 4096*100000;

        auto tree = MakeSharedPackedStructByBlock<Tree>(block_size);

        int32_t size = 2048;



        vector<UUID> data(size);

        for (auto& v: data) v = UUID::make_random();

        auto data_s = data;

        std::sort(data_s.begin(), data_s.end());

//      long t0 = getTimeInMillis();
        tree->_insert(0, size, [&](int32_t block, int32_t idx){return data_s[idx];});

//      long t1 = getTimeInMillis();

//      tree->dump();

        cout<<"Find test: "<<endl;

//      for (auto& v: data)
//      {
//          auto result = tree->find_ge(Block, v);
//
////            cout<<v<<" -- "<<result.idx()<<" -- "<<data_s[result.idx()]<<endl;
//
//          MEMORIA_V1_ASSERT(v, ==, data_s[result.idx()]);
//      }

        auto nv = UUID::make_random();

        auto result = tree->find_ge(Block, nv);

        if (result.idx() >= size)
        {
            cout<<nv<<" -- "<<result.idx()<<endl;
        }
        else {
            cout<<nv<<" -- "<<result.idx()<<" "<<data_s[result.idx()]<<endl;
            cout<<(nv < data_s[result.idx()])<<endl;
            cout<<(nv == data_s[result.idx()])<<endl;
            cout<<(nv > data_s[result.idx() - 1])<<endl;
        }

    }
    catch (PackedOOMException& ex) {
        cout<<ex.source()<<endl;
        cout<<ex<<endl;
    }
    catch (Exception& ex) {
        cout<<ex.source()<<endl;
        cout<<ex<<endl;
    }
}
