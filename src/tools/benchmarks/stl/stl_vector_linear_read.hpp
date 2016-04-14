
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



class StlVectorLinearReadBenchmark: public BenchmarkTask {

    typedef BenchmarkTask Base;

    typedef std::vector<BigInt> VectorCtrType;

    VectorCtrType* ctr_;

    Int result_;


public:

    StlVectorLinearReadBenchmark(StringRef name):
        BenchmarkTask(name)
    {
        average = 10;
    }

    virtual ~StlVectorLinearReadBenchmark() noexcept {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x() / sizeof(BigInt);

        ctr_ = new VectorCtrType();

        for (Int c = 0; c < size; c++)
        {
            BigInt value = getRandom(10000);
            ctr_->push_back(value);
        }
    }

    virtual void release(ostream& out)
    {
        delete ctr_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        for (Int c = 0; c < params.operations();)
        {
            for (auto i = ctr_->begin(); i != ctr_->end(); i++)
            {
                result_ += *i;
            }

            c += ctr_->size();
        }

        params.memory() = params.operations() * sizeof(BigInt);
    }
};


}}