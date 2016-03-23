
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

#include "custom_allocator.hpp"

#include <malloc.h>
#include <memory>
#include <set>



namespace memoria {
namespace v1 {

using namespace std;



class StlSetScanBenchmark: public BenchmarkTask {

    typedef BigInt       Key;
    typedef set<Key, less<Key>, CustomAllocator<Key> > Map;

    Map*            map_;
    BigInt          result_;

public:

    StlSetScanBenchmark(StringRef name): BenchmarkTask(name) {}

    virtual ~StlSetScanBenchmark() throw() {}



    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        map_ = new Map();

        BigInt sum = 0;

        for (Int c = 0; c < params.x(); c++)
        {
            map_->insert(sum++);
        }
    }

    virtual void release(ostream& out)
    {
        delete map_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        for (Int c = 0; c < params.operations();)
        {
            for (auto i = map_->begin(); i!= map_->end() && c < params.operations(); i++, c++)
            {
                result_ += *i;
            }
        }
    }
};


}}