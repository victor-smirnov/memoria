
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

#include <memoria/reactor/mpsc_queue.hpp>
#include <memoria/core/tools/time.hpp>

#include <boost/lockfree/spsc_queue.hpp>

#include <thread>
#include <iostream>
#include <atomic>

using namespace memoria::reactor;
using namespace memoria::v1;

namespace lf = boost::lockfree;

size_t counter{};



int main(int argc, char** argv)
{
    lf::spsc_queue<int, boost::lockfree::capacity<1024>> queue1;
    lf::spsc_queue<int, boost::lockfree::capacity<1024>> queue2;
    
    size_t size = 10000000;
    
    auto t0 = getTimeInMillis();

    int vv = 0;
    
    std::thread th1([&]{
        int vvv = 0;
        for (size_t c = 0; c < size; c++)
        {
            while(!queue1.push(c)) {}            
            while (!queue2.pop(vvv)) {}
        }
        while(!queue1.push(-1)) {}
        
        vv = vvv;
    });
    
    std::thread th2([&]{
        while (true)
        {
            int vv = 0;
            
            while (!queue1.pop(vv)) {}
            
            if (vv < 0) {
                break;
            }
            else {
                counter++;
                while (!queue2.push(vv)) {}
            }
        }
    });
    
    th1.join();
    th2.join();
    
    auto t1 = getTimeInMillis();
    
    std::cout << "Time: " << FormatTime(t1 - t0) << " -- " << counter << " -- " << vv << std::endl;
    
    return 0;
}
