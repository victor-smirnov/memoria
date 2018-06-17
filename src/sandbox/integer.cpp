
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

        auto ctr = create<EdgeMap>(snp);

        UUID k1 = UUID::make_random();

        std::vector<UAcc> values;

        auto t1 = getTimeInMillis();

        for (uint64_t c = 0; c < 1000; c++)
        {
            UAcc128T vv = UUID::make_random();
            values.push_back(vv);
            ctr.upsert(k1, vv);
        }

        auto t2 = getTimeInMillis();

        engine().coutln(u"Creation time: {}", FormatTime(t2 - t1));

        std::sort(values.begin(), values.end());
        auto values_sorted = values;

        size_t cnt{};
        for (auto ii = ctr.iterate(k1); ii.has_next();)
        {
            auto nn = ii.next();
            engine().coutln(u"values: {} {}", nn, values_sorted[cnt++]);
        }

        snp.commit();

        alloc.store("allocator.mma1");

        std::ofstream ofs( "store.dat" );
        boost::archive::text_oarchive ar(ofs);

        ar << values;

        return 0;
    });
}
