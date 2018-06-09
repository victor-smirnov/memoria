
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/integer/integer.hpp>

#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>

#include <iostream>
#include <type_traits>

using namespace memoria::v1;

namespace mp = boost::multiprecision;

int main()
{
    using UAcc = UnsignedAccumulatorT<uint64_t>;

    UAcc a{1}, b{2};

    mp::uint512_t d{12345};

    *d.backend().limbs() = 56789;

    std::cout << Type2Str<mp::limb_type> << " :: " << sizeof(mp::limb_type) << std::endl;

//    using FQTree = PkdFQTreeT<UAcc, 1, uint64_t>;

//    FQTree* tree{};

//    std::cout << TypeNameFactory<FQTree::IndexValue>::name() << std::endl;

//    (void)tree->reindex();

//    tree->findGEBackward(0, 0, UAcc{});
//    tree->findGEForward(0, 0, UAcc{});

//    tree->findGTBackward(0, 0, UAcc{});
//    tree->findGTForward(0, 0, UAcc{});

//    (void)tree->remove(0,0);
    
    


//    int a = 0;
//    int b = std::numeric_limits<int>::max() - 1;
//    int of = 0;

//    std::cout << __builtin_sadd_overflow(a, b, &of) << std::endl;
//    std::cout << of << std::endl;

    //std::cout << a << " :: " << b << " :: " << (a == b) << " :: " << d << std::endl;

    return 0;
}
