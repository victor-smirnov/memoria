
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




class VectorMapRandomReadBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<VectorMap<BigInt, Byte>>::Type                      Ctr;
    typedef typename Ctr::Iterator                                              Iterator;

    static const Int MAX_DATA_SIZE                                              = 256;

    Allocator*  allocator_;
    Ctr*        ctr_;

    BigInt      memory_size;

public:

    VectorMapRandomReadBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);
    }

    virtual ~VectorMapRandomReadBenchmark() noexcept {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        Int size = params.x();
        MemBuffer<Byte> data(size);

        BigInt total = 0;

        ctr_ = new Ctr(allocator_);

        while (total < memory_size)
        {
            auto i = ctr_->create();
            i.insert(data);
            total += data.size();
        }

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
        MemBuffer<Byte> data(size);
        Int total = memory_size/size;

        params.operations() = total;

        for (Int c = 0; c < total; c++)
        {
            ctr_->find(getRandom(total)).read(data);
        }

        params.operations() = ctr_->count();
        params.memory()     = ctr_->size() + ctr_->count() * 16; //sizeof(BigInt) * 2
    }
};


}}