
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



#include <memoria/v1/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>

#include <memoria/v1/core/tools/time.hpp>

#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>
#include <memoria/v1/core/tools/bignum/bigint.hpp>
#include <memoria/v1/core/tools/bignum/cppint_codec.hpp>
#include <memoria/v1/core/tools/bignum/int64_codec.hpp>
#include <memoria/v1/core/tools/alloc.hpp>


#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <memoria/v1/metadata/page.hpp>

#include <iostream>
#include <vector>
#include <cstddef>

using namespace memoria;
using namespace std;

using namespace boost::multiprecision;



int main()
{
    try {
        String str = "abc";

        cout<<sizeof(str)<<endl;


//      using Tree = PkdVBMTreeT<BigInteger>;
//      auto block = AllocateAllocator(1024*10240);
//
//      auto tree = block->template allocateEmpty<Tree>(0);
//
//      BigInteger num("12345670000000000000000000000000000000000000000000000");
//
//      tree->_insert(0, 100000, [&](int32_t, int32_t c) -> BigInteger {
//          return (num++);
//      });
//
//      DumpStruct(tree);
//
//
//      auto tgt = num - 60;
//      cout<<"Looking for: "<<tgt<<endl;
//      cout<<"Find: "<<tree->find_ge(tgt).idx()<<endl;
//
//      tree->check();
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
