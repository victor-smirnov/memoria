
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

#include <chrono>
#include <iostream>
#include <vector>
#include <random>

static inline uint64_t getTimeInNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int main(void)
{
    using RngEngine64 = std::mt19937_64;

    std::mt19937_64 engine_;
    std::uniform_int_distribution<uint64_t> distribution_;

    auto random = [&](){
        return distribution_(engine_);
    };

    size_t size_ln2 = 30;
    size_t buf_size = 1ull << size_ln2; // 8GB
    size_t idx_size = buf_size/16;

    uint64_t mask = buf_size - 1;

    int64_t t0, t1;
    uint64_t sum{};

    auto tx = getTimeInNanos();
    {
        std::vector<uint64_t> buffer_(buf_size);
        std::vector<uint64_t> indices_(idx_size);

        for (auto& v: buffer_) {
            v = random();
        }

        for (auto& v: indices_) {
            v = random() % buf_size;
        }

        t0 = getTimeInNanos();

        for (size_t c = 0; c < idx_size; c++) {
            sum += buffer_[sum & mask];
        }

//        for (auto ii: indices_) {
//            sum += buffer_[ii];
//        }

        t1 = getTimeInNanos();
    }
    auto tz = getTimeInNanos();

    double ns_in_ms = 1000000;
    double time = t1 - t0;

    std::cout << "Done. " << idx_size << " ops in " << (time/ns_in_ms) << " ms, " << time/idx_size << " ns/op" << std::endl;
    std::cout << "Setup time: " << ((t0 - tx)/ns_in_ms) << " ms, total " << ((tz - tx)/ns_in_ms) << " ms, sum = " << sum << std::endl;

    return 0;
}
