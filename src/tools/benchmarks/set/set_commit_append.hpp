
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_COMMIT_APPEND_HPP_
#define MEMORIA_BENCHMARKS_SET_COMMIT_APPEND_HPP_

#include "../benchmarks_inc.hpp"


#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetCommitAppendBenchmark: public SPBenchmarkTask {

    public:
    Int max_size;


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;


    typedef typename DCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::Types::Entry                           Entry;


    Allocator*      allocator_;
    SetCtrType*     set_;

public:

    SetCommitAppendBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(1*1024*1024)
    {
        Add("max_size", max_size);
    }

    virtual ~SetCommitAppendBenchmark() throw() {}

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

        auto i = set_->End();

        params.operations() = this->max_size;

        for (Int c = 0; c < this->max_size; c++)
        {
            Entry entry;

            entry.indexes()[0] = 1;

            i.insert(entry);

            if (c % size == 0)
            {
                allocator_->commit();
            }
        }

        allocator_->commit();
    }
};


}


#endif
