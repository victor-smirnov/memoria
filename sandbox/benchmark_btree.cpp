
// Copyright 2022 Victor Smirnov
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


#include "benchmark_common.hpp"

#include <absl/container/btree_set.h>
#include <iostream>

int main(void)
{
    size_t max_memblock_size = 1024 * 1024 * 128; // 1GB

    for (size_t memblock_size = 1; memblock_size <= max_memblock_size; memblock_size *= 2)
    {
        size_t tree_size = memblock_size ; // empirically obtained

        RngUInt64 rng;

        absl::btree_set<uint64_t> set;

        for (size_t c = 1; c < tree_size; c++) {
            set.insert(c);
        }

        size_t query_size = 1024 * 1024;
        std::vector<size_t> indices(query_size);

        for (auto& ii: indices) {
            ii = rng() % tree_size;
        }

        auto t0 = get_time_in_millis();
        volatile uint64_t sum{};

        for (auto ii: indices) {
            sum += set.contains(ii);
        }

        auto t1 = get_time_in_millis();

        std::cout << memblock_size << " " << (query_size * 1000 / (t1 - t0)) << std::endl;
    }
}
