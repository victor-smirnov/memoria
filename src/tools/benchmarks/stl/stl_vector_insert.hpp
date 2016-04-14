
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


class StlVectorWriteBenchmark: public BenchmarkTask {

    typedef BenchmarkTask Base;

    typedef std::vector<BigInt> VectorCtrType;

    VectorCtrType* ctr_;

    Int* rd_array_;

public:

    StlVectorWriteBenchmark(StringRef name):
        BenchmarkTask(name)
    {
    }

    virtual ~StlVectorWriteBenchmark() noexcept {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        ctr_ = new VectorCtrType();

        for (Int c = 0; c < size; c++)
        {
            BigInt value = getRandom(10000);
            ctr_->push_back(value);
        }

        rd_array_ = new Int[params.operations()];
        for (Int c = 0; c < params.operations(); c++)
        {
            rd_array_[c] = getRandom(size);
        }
    }

    virtual void release(ostream& out)
    {
        delete ctr_;
        delete[] rd_array_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        for (Int c = 0; c < params.operations(); c++)
        {
            ctr_->insert(ctr_->begin() + rd_array_[c], rd_array_[c]);
        }
    }
};


}}