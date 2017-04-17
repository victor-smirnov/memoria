
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/reactor/mpsc_queue.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <thread>
#include <iostream>
#include <atomic>

using namespace memoria::v1::reactor;
using namespace memoria::v1;

size_t counter{};

int main(int argc, char** argv)
{
    MPSCQueue<int> queue1;
    MPSCQueue<int> queue2;
    
    size_t size = 10000000;
    
    auto t0 = getTimeInMillis();
    
    std::thread th1([&]{
        for (size_t c = 0; c < size; c++)
        {
            queue1.send(c);
            while (queue2.get(1, [](auto v){}) == 0);
        }
        queue1.send(-1);
    });
    
    std::thread th2([&]{
        bool done = false;
        while (!done){
            queue1.get(100, [&](auto v){
                if (v < 0) {
                    done = true;
                }
                else {
                    counter++;
                    queue2.send(v);
                }
            });
        }
    });
    
    th1.join();
    th2.join();
    
    auto t1 = getTimeInMillis();
    
    std::cout << "Time: " << FormatTime(t1 - t0) << " -- " << counter << std::endl;
    
    return 0;
}




