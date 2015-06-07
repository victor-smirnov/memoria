
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_APPEND_HPP_
#define MEMORIA_BENCHMARKS_SET_APPEND_HPP_

#include "../benchmarks_inc.hpp"


#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetAppendBatchBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

    typedef typename SCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::Types::Entry                           Entry;



    Allocator*  allocator_;
    SetCtrType* set_;

    Int         max_size;

public:

    SetAppendBatchBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(16*1024*1024)
    {
        Add("max_size", max_size);
    }

    virtual ~SetAppendBatchBenchmark() throw() {}


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

        Int map_size = 0;

        Iterator i = set_->End();

        params.operations() = this->max_size;

        Entry entry;
        entry.indexes()[0] = 1;

        for (Int c = 0; c < params.operations() / size; c++)
        {
            FnDataSource<Entry> source(size, [&](BigInt idx){
                return entry;
            });

            set_->insert(i, source);

            map_size += size;
        }

        allocator_->rollback();
    }
};


}


#endif
