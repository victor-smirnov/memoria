
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_INSERT_HPP_
#define MEMORIA_BENCHMARKS_SET_INSERT_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetInsertBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;


    typedef typename DCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::Types::Entry                           Entry;


    Allocator*  allocator_;
    SetCtrType* set_;



public:

    SetInsertBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {}

    virtual ~SetInsertBenchmark() throw() {}

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

        params.operations() = size;

        for (Int c = 0; c < size; c++)
        {
            auto i = set_->find(getRandom(size));

            Entry entry;

            entry.indexes()[0] = 1;

            i.insert(entry);
        }
    }
};


}


#endif
