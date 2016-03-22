
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



class StlVectorLinearReadBenchmark: public BenchmarkTask {

    typedef BenchmarkTask Base;

    typedef std::vector<BigInt> VectorCtrType;

    VectorCtrType* ctr_;

    Int result_;


public:

    StlVectorLinearReadBenchmark(StringRef name):
        BenchmarkTask(name)
    {
        average = 10;
    }

    virtual ~StlVectorLinearReadBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x() / sizeof(BigInt);

        ctr_ = new VectorCtrType();

        for (Int c = 0; c < size; c++)
        {
            BigInt value = getRandom(10000);
            ctr_->push_back(value);
        }
    }

    virtual void release(ostream& out)
    {
        delete ctr_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        for (Int c = 0; c < params.operations();)
        {
            for (auto i = ctr_->begin(); i != ctr_->end(); i++)
            {
                result_ += *i;
            }

            c += ctr_->size();
        }

        params.memory() = params.operations() * sizeof(BigInt);
    }
};


}
