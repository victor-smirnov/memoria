
// Copyright 2023 Victor Smirnov
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

#include "queue_bench.hpp"

#include <atomic_queue/atomic_queue.h>

#include <thread>
#include <vector>
#include <functional>

using namespace memoria;

int main(void) {

    using T = uint64_t;
    using QueueT = atomic_queue::AtomicQueue<T, 1024>;

    QueueT queue;

    std::vector<std::thread> consumers;
    constexpr size_t N = 19;
    size_t num_msg = 100000000;

    auto consumer_fn = [&](){
        T val{};
        do {
            //while(!queue.try_pop(val)) {}
            val = queue.pop();
        }
        while (val > 1);
    };


    using namespace std::placeholders;
    for (size_t c = 0; c < N; c++) {
        consumers.emplace_back(consumer_fn);
    }

    uint64_t t0 = getTimeInMillis();
    for (size_t c = 2; c <= num_msg + 1; c++) {
        queue.push(c);
    }

    for (size_t c = 0; c < N; c++) {
        queue.push(1);
    }

    uint64_t t1 = getTimeInMillis();

    println("Done...");

    for (size_t c = 0; c < N; c++) {
        consumers[c].join();
    }

    println("Time: {}", FormatTime(t1 - t0));

    return 0;
}
