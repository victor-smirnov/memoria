
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



class setCommitappendBenchmark: public SPBenchmarkTask {

    public:
    Int max_size;


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

//    typedef typename SmallCtrTypeFactory::Factory<Root>::Type       RootCtr;
    typedef typename SmallCtrTypeFactory::Factory<Set1>::Type       SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::ID                                     ID;
    typedef typename SetCtrType::Accumulator                            Accumulator;


    typedef typename SetCtrType::Key                                    Key;
    typedef typename SetCtrType::Value                                  Value;

    Allocator*  allocator_;
    SetCtrType*     set_;

public:

    setCommitappendBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(1*1024*1024)
    {
        Add("max_size", max_size);

//        RootCtr::initMetadata();
        SetCtrType::initMetadata();
    }

    virtual ~setCommitappendBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        set_ = new SetCtrType(allocator_, 1, true);
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
            Accumulator keys;
            keys[0] = 1;

            set_->insertRaw(i, keys);

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
