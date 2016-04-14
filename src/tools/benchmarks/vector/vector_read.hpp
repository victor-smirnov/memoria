
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



class VectorReadBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<Vector<BigInt>>::Type                               VectorCtrType;
    typedef typename VectorCtrType::Iterator                                    Iterator;



    Allocator*      allocator_;
    VectorCtrType*  ctr_;

    Int result_;

    Int* rd_array_;

public:

    VectorReadBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {
        average = 5;
    }

    virtual ~VectorReadBenchmark() noexcept {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_  = new Allocator();
        Int size    = params.x();

        ctr_ = new VectorCtrType(allocator_);

        Iterator i = ctr_->seek(0);

        for (Int c = 0; c < size/128; c++)
        {
            vector<BigInt> buffer(128);

            for (auto& d: buffer)
            {
                d = getRandom(10000);
            }

            i.insert(buffer);
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
        delete allocator_;
        delete[] rd_array_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        volatile BigInt buffer = 0;

        BigInt total = 0;

        for (Int c = 0; c < params.operations(); c++, total++)
        {
            buffer += ctr_->seek(rd_array_[c]).value();
        }

        params.memory() = total;
    }
};


}}