
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/time.hpp>

#include <type_traits>

using namespace memoria;

int main(int, char**)
{
    auto t0 = getTimeInMillis();

    uint64_t cnt = 0;
    uint64_t cnt_m = 1;

    double total = std::numeric_limits<uint32_t>::max();

    uint32_t num_b = 8;
    uint32_t mask0 = (1u << num_b) - 1;

    uint32_t max = std::numeric_limits<uint32_t>::max();
    for (uint32_t c = 0; c < max; c++)
    {
        bool tgt = PopCnt(c) >= num_b;
        if (tgt)
        {
            cnt++;
            for (size_t d = 0; d < 32 - num_b; d++)
            {
                if (((c >> d) & mask0) == mask0)
                {
                    cnt_m ++;
                    break;
                }
            }
        }
    }

    auto t1 = getTimeInMillis();

    std::cout << "time: " << (t1 - t0) << " :: " << cnt << " " << (cnt_m / (double)cnt) << std::endl;

    return 0;
}
