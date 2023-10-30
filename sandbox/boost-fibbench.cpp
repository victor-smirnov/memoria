
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

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/operations.hpp>

#include <iostream>

class TreeWalker {
    uint64_t counter_{};
    const uint64_t counter_max_;

    uint64_t fib_counter_{};

public:
    TreeWalker(uint64_t max): counter_max_(max) {}

    void dig_into_fibr(size_t depth = 0)
    {
        if (counter_ < counter_max_)
        {
            if (depth < 64) {
                counter_++;
                if (counter_ < counter_max_) {
                    dig_into_fibr(depth + 1);
                    dig_into_fibr(depth + 1);
                }
            }
        }

        if (++fib_counter_ > 0) {
            boost::this_fiber::yield();
            fib_counter_ = {};
        }
    }



    void reset() {
        counter_ = {};
        fib_counter_ = {};
    }

    uint64_t counter() const {return counter_;}
};


int main(int argc, char** argv)
{
    TreeWalker walker(1ull << 26);
    long t2 = memoria::getTimeInMillis();

    walker.reset();

    boost::fibers::fiber f0([&](){
        walker.dig_into_fibr();
    });

    f0.join();

    long t3 = memoria::getTimeInMillis();

    std::cout << "Counter fibr: " << walker.counter() << " :: " << (t3 - t2) << std::endl;
}
