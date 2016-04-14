
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



class VectorSequentialReadBenchmark: public SPBenchmarkTask {


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<Vector<UByte>>::Type                                VectorCtrType;
    typedef typename VectorCtrType::Iterator                                    Iterator;

    Allocator*      allocator_;
    VectorCtrType*  ctr_;

    BigInt      memory_size;

public:

    VectorSequentialReadBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);

        average = 10;
    }

    virtual ~VectorSequentialReadBenchmark() noexcept {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        ctr_ = new VectorCtrType(allocator_);

        BigInt size = 1024*1024;

        vector<UByte> data(size);

        Iterator i = ctr_->seek(0);
        for (Int c = 0; c < memory_size / size; c++)
        {
            i.insert(data);
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
        Int     size    = params.x();

        vector<UByte> data(size);

        for (Iterator i = ctr_->seek(0); !i.isEof();)
        {
            i.read(data);
        }

        params.operations() = memory_size / size;
        params.memory()     = memory_size;
    }
};


}}