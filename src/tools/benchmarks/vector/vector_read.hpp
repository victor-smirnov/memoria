
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_VECTOR_READ_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_VECTOR_READ_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorReadBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

    typedef typename SmallCtrTypeFactory::Factory<Vector<BigInt>>::Type         VectorCtrType;
    typedef typename VectorCtrType::Iterator                                    Iterator;
    typedef typename VectorCtrType::ID                                          ID;
    typedef typename VectorCtrType::Accumulator                                 Accumulator;


    typedef typename VectorCtrType::Key                                         Key;



    Allocator*      allocator_;
    VectorCtrType*  ctr_;

    Int result_;

    Int* rd_array_;

public:

    VectorReadBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {
        VectorCtrType::initMetadata();

        average = 5;
    }

    virtual ~VectorReadBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        Int size = params.x();

        ctr_ = new VectorCtrType(allocator_);

        Iterator i = ctr_->seek(0);

        for (Int c = 0; c < size/128; c++)
        {
            BigInt array[128];
            for (Int d = 0; d < 128; d++)
            {
                array[d] = getRandom(10000);
            }

            i.insert(ArrayData<BigInt>(sizeof(array)/sizeof(BigInt), array));
        }

        rd_array_ = new Int[params.operations()];
        for (Int c = 0; c < params.operations(); c++)
        {
            rd_array_[c] = getRandom(size);
        }
    }

    virtual void release(ostream& out)
    {
        delete ctr_;
        delete allocator_;
        delete[] rd_array_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        BigInt buffer;
        ArrayData<BigInt> data(sizeof(buffer), &buffer);

        BigInt total = 0;

        for (Int c = 0; c < params.operations(); c++)
        {
            total += ctr_->seek(rd_array_[c]).read(data);
        }

        params.memory() = total;
    }
};


}


#endif
