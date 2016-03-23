
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

#include <memoria/v1/containers/vector/vctr_factory.hpp>

namespace memoria {
namespace v1 {

using namespace std;



class VectorAppendBenchmark: public SPBenchmarkTask {
public:



    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;


    typedef typename DCtrTF<Vector<UByte>>::Type                                Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;


    Allocator*  allocator_;
    Ctr*        ctr_;

    Int         memory_size;


public:

    VectorAppendBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);
    }

    virtual ~VectorAppendBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_  = new Allocator();
        ctr_        = new Ctr(allocator_);

        allocator_->commit();
    }

    virtual void release(ostream& out)
    {
        delete ctr_;
        delete allocator_;
    }


    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        auto i = ctr_->End();

        BigInt total = 0;

        params.operations() = 0;

        vector<UByte> data(size);

        while (total < memory_size)
        {
            i.insert(data);

            total += data.size();

            params.operations()++;
        }

        params.memory() = total;

        allocator_->rollback();
    }
};


}}