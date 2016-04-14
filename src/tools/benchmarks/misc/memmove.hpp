
// Copyright 2012 Victor Smirnov
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


#pragma once

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {
namespace v1 {

using namespace std;



class MemmoveBenchmark: public BenchmarkTask {

    Byte*   array_;
    Int*    rd_array_;

public:

    MemmoveBenchmark(StringRef name):
        BenchmarkTask(name)
    {
        average = 5;
    }

    virtual ~MemmoveBenchmark() noexcept {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        array_ = T2T<Byte*>(malloc(size));

        rd_array_ = new Int[params.operations()];
        for (Int c = 0; c < params.operations(); c++)
        {
            Int addr = getRandom(size - 16);

            if ((addr & 4095) > 4080)
            {
                addr -= 16;
            }

            rd_array_[c] = addr;
        }
    }

    virtual void release(ostream& out)
    {
        free (array_);
        delete[] rd_array_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        BigInt  total   = 0;

        for (Int c = 0; c < params.operations(); c++)
        {
            Int addr = rd_array_[c];
            Int size = 4096 - (addr & 0xFFF) - 8;

            CopyBuffer(array_ + addr, array_ + addr + 8, size);

            total += size;
        }

        params.memory() = total;
    }
};


}}