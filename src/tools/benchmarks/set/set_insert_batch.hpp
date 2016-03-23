
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



class SetInsertBatchBenchmark: public SPBenchmarkTask {
public:


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<Set1>::Type                                         SetCtrType;
    typedef typename SetCtrType::Iterator                                       Iterator;
    typedef typename SetCtrType::Types::Entry                                   Entry;

    Int             max_size;
    Allocator*      allocator_;
    SetCtrType*     set_;

public:

    SetInsertBatchBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(16*1024*1024)
    {
        Add("max_size", max_size);
    }

    virtual ~SetInsertBatchBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        set_ = new SetCtrType(allocator_);
    }

    virtual void release(ostream& out)
    {
        delete set_;
        delete allocator_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        Entry entry;
        entry.indexes()[0] = 1;

        Int map_size = 0;

        params.operations() = this->max_size;

        for (Int c = 0; c < params.operations() / size; c++)
        {
            Int pos = getRandom(map_size - 1) + 1;
            auto i = map_size == 0? set_->End() : set_->find(pos);

            FnDataSource<Entry> source(size, [&](BigInt idx){
                return entry;
            });

            set_->insert(i, source);

            map_size += size;
        }

        allocator_->rollback();
    }
};


}}