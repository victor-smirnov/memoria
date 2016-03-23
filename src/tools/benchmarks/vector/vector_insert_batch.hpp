
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

class VectorInsertBatchBenchmark: public SPBenchmarkTask {
public:

    Int max_size;


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<Vector<UByte>>::Type                                Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;


    Allocator*  allocator_;
    Ctr* map_;



public:

    VectorInsertBatchBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(128*1024*1024)
    {
        Add("max_size", max_size);
    }

    virtual ~VectorInsertBatchBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_  = new Allocator();
        map_        = new Ctr(allocator_);

        allocator_->commit();
    }

    virtual void release(ostream& out)
    {
        delete map_;
        delete allocator_;
    }


    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        vector<UByte> data(size);

        Int max = this->max_size / size;

        Int total = 0;

        params.operations() = max;

        for (Int c = 0; c < max; c++)
        {
            auto i = map_->seek(getRandom(total));

            i.insert(data);

            total += size;
        }

        allocator_->rollback();
    }
};


}}