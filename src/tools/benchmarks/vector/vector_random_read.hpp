
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_RANDOM_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_RANDOM_READ_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorRandomReadBenchmark: public SPBenchmarkTask {


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

//    typedef typename SmallCtrTypeFactory::Factory<Root>::Type           RootCtr;
    typedef typename SmallCtrTypeFactory::Factory<VectorCtr>::Type      VectorCtrType;
    typedef typename VectorCtrType::Iterator                            Iterator;
    typedef typename VectorCtrType::ID                                  ID;
    typedef typename VectorCtrType::Accumulator                         Accumulator;


    typedef typename VectorCtrType::Key                                 Key;



    Allocator*  allocator_;
    VectorCtrType*  ctr_;

    BigInt      memory_size;

public:

    VectorRandomReadBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
//        RootCtr::initMetadata();
        VectorCtrType::initMetadata();

        Add("memory_size", memory_size);

        average = 10;
    }

    virtual ~VectorRandomReadBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        ctr_ = new VectorCtrType(allocator_);

        BigInt size = 1024*1024;

        ArrayData data(size, malloc(size), true);

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

        ArrayData data(size, malloc(size), true);

        BigInt total = 0;
        BigInt operations = 0;

        while (total < memory_size)
        {
            auto i = ctr_->seek(getRandom(memory_size - size));
            total += i.read(data);
            operations++;
        }

        params.operations() = operations;
        params.memory()     = total;
    }
};


}


#endif
