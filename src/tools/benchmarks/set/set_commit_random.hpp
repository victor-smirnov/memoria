
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetCommitRandomBenchmark: public SPBenchmarkTask {
public:
    Int max_size;

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;


    typedef typename DCtrTF<Set1>::Type                                         SetCtrType;
    typedef typename SetCtrType::Iterator                                       Iterator;
    typedef typename SetCtrType::Types::Entry                                   Entry;

    Allocator*      allocator_;
    SetCtrType*     set_;

public:

    SetCommitRandomBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(1*1024*1024)
    {
        Add("max_size", max_size);
    }

    virtual ~SetCommitRandomBenchmark() throw() {}

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

        params.operations() = this->max_size;

        for (Int c = 0; c < this->max_size; c++)
        {
            auto i = c == 0? set_->End() : set_->find(getRandom(c));

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
