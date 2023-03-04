
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

#include <memoria/core/tools/time.hpp>
#include <memoria/core/strings/format.hpp>


#include <boost/fiber/all.hpp>

namespace bf = boost::fibers;
using namespace memoria;

int main()
{
    long t0 = getTimeInMillis();
    for (size_t c = 0; c < 100; c++) {
        bf::fiber([]{
            for (size_t d = 0; d < 1000000; d++) {
                boost::this_fiber::yield();
            }
        }).detach();
    }
    long t1 = getTimeInMillis();

    println("Time: {}", FormatTime(t1 - t0));

    return 0;
}
