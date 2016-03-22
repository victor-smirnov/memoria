
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../benchmarks_inc.hpp"

#include "custom_allocator.hpp"

#include <malloc.h>
#include <memory>
#include <set>



namespace memoria {

using namespace std;



class StlSetSizeBenchmark: public BenchmarkTask {
private:

    typedef BigInt       Key;
    typedef set<Key, less<Key>, CustomAllocator<Key> > Map;

    Map*            map_;
    Int             result_;
    Int*            rd_array_;

public:

    StlSetSizeBenchmark(StringRef name): BenchmarkTask(name)
    {
        average = 10;
    }

    virtual ~StlSetSizeBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        map_ = new Map();

        AllocatorBase<>::reset();

        for (Int c = 0; c < params.x(); c++)
        {
            map_->insert(c);
        }

        out<<"Allocated: "<<AllocatorBase<>::count()<<endl;

        rd_array_ = new Int[params.operations()];
        for (Int c = 0; c < params.operations(); c++)
        {
            rd_array_[c] = getRandom(map_->size());
        }
    }

    virtual void release(ostream& out)
    {
        delete map_;
        delete[] rd_array_;
    }



    virtual void Benchmark(BenchmarkParameters& data, ostream& out)
    {
        for (Int c = 0; c < data.operations(); c++)
        {
            result_ = (map_->find(rd_array_[c]) != map_->end());
        }
    }
};


}
