
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {
namespace v1 {

using namespace std;



class setCreateBatchBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

    typedef typename DCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::Types::Entry                           Entry;

    Allocator*  allocator_;
    SetCtrType*     set_;

public:

    setCreateBatchBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {}

    virtual ~setCreateBatchBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_  = new Allocator();
        set_        = new SetCtrType(allocator_);

        allocator_->commit();
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

        FnDataSource<Entry> source(size, [&](BigInt idx){
            return entry;
        });

        params.operations() = size;

        Iterator i = set_->End();

        set_->insert(i, source);

        allocator_->commit();
    }
};


}}