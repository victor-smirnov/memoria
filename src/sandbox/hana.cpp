
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


#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/datatypes/datatypes.hpp>

#include <memoria/v1/core/types/hana.hpp>
#include <memoria/v1/core/types/mp11.hpp>

#include <memoria/v1/core/types/algo/select.hpp>

#include <memoria/v1/core/tools/arena_buffer.hpp>

#include <iostream>

using namespace memoria::v1;

namespace hana = boost::hana;
using namespace hana::literals;

namespace mp11 = boost::mp11;

struct TT {

    template <typename T>
    struct add_pointer_mf: StdMetaFn<T*> {};

    using T = decltype(from_hana(
        hana::transform(to_hana<TL<int, long(), double>>(), hana::metafunction<add_pointer_mf>)
    ));

    template <typename T>
    using AddPointerT = typename std::add_pointer<T>::type;

    using T1 =
        mp11::mp_transform<AddPointerT, std::tuple<int, long(), double>>;
};

int main()
{
//    auto tt = to_hana<TL<int, long, double>>();

//    auto jj = hana::tuple_t<int, long, double>;

//    auto ii = hana::make_tuple(5_c, 3_c, 8_c, 2_c, 4_c, 7_c, 0_c);


//    BOOST_HANA_CONSTANT_CHECK(tt == jj);

//    std::cout << TypeNameFactory<decltype(tt)>::name() << std::endl;
//    std::cout << TypeNameFactory<decltype(jj)>::name() << std::endl;
//    std::cout << TypeNameFactory<decltype(hana::contains(jj, hana::type_c<double>))>::name() << std::endl;

//    std::cout << TypeNameFactory<decltype(hana::sort(ii))>::name() << std::endl;
    std::cout << TypeNameFactory<TT::T>::name() << std::endl;
    std::cout << TypeNameFactory<TT::T1>::name() << std::endl;


    std::cout << SelectVOrDefault<5, IntList<1,2,3,4,5>> << std::endl;


    ArenaBuffer<int32_t> buffer([](int32_t id, ArenaBufferCmd cmd, size_t size, uint8_t* buf) -> uint8_t* {
        if (cmd == ArenaBufferCmd::ALLOCATE) {
            return allocate_system<uint8_t>(size).release();
        }
        else {
            free_system(buf);
            return nullptr;
        }
    });

    buffer.set_buffer_id(0);

    std::vector<int32_t> vals = {1,2,3,4,5};

    buffer.append_value(12345);
    buffer.append_values(vals);

    for (auto ii: buffer.span()) {
        std::cout << ii << std::endl;
    }

    return 0;
}
