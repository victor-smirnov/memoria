
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
#include <memoria/v1/core/tools/uuid.hpp>

#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>
#include <memoria/v1/api/db/update_log/update_log_api.hpp>

#include <memoria/v1/api/allocator/allocator_inmem_api.hpp>

#include <memoria/v1/core/tools/time.hpp>

#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/core/tools/boost_serialization.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <iostream>
#include <type_traits>
#include <vector>
#include <algorithm>
#include <random>

namespace mp = boost::multiprecision;

using namespace memoria::v1;
using namespace memoria::v1::reactor;

using UAcc = memoria::v1::UnsignedAccumulator<128>;

int main(int argc, char** argv, char** envp)
{
    return Application::run_e(argc, argv, envp, [](){

        ShutdownOnScopeExit sh;

        InMemAllocator<> alloc = InMemAllocator<>::create();

        auto snp = alloc.master().branch();

        auto ctr = create<UpdateLog>(snp);

//        UUID id = UUID::make_random();
//        ctr.create_snapshot(id);

        std::vector<UUID> ids_v;
//        ids_v.push_back(id);

        UUID id;

        for (size_t c = 0; c < 10000; c++)
        {
            UUID id0 = UUID::make_random();

            if (c == 3) {
                id = id0;
            }

            ctr.create_snapshot(id0);
            ids_v.push_back(id0);
        }

        auto ii = ctr.find_snapshot(id);

        size_t cnt = 3;
        while (ii.has_next())
        {
            engine().coutln(u"{} {} {}", cnt, ii.next(), ids_v[cnt]);
            cnt++;
        }

        snp.commit();

        alloc.store("allocator.mma1");

        return 0;
    });
}
