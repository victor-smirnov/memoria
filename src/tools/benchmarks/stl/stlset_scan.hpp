
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_PMAP_STLSET_SCAN_HPP_
#define MEMORIA_BENCHMARKS_PMAP_STLSET_SCAN_HPP_

#include "../benchmarks_inc.hpp"

#include "custom_allocator.hpp"

#include <malloc.h>
#include <memory>
#include <set>



namespace memoria {

using namespace std;



class StlSetScanBenchmark: public BenchmarkTask {

    typedef BigInt       Key;
    typedef set<Key, less<Key>, CustomAllocator<Key> > Map;

    Map*            map_;
    BigInt          result_;

public:

    StlSetScanBenchmark(StringRef name): BenchmarkTask(name) {}

    virtual ~StlSetScanBenchmark() throw() {}



    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        map_ = new Map();

        BigInt sum = 0;

        for (Int c = 0; c < params.x(); c++)
        {
            map_->insert(sum++);
        }
    }

    virtual void release(ostream& out)
    {
        delete map_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        for (Int c = 0; c < params.operations();)
        {
            for (auto i = map_->begin(); i!= map_->end() && c < params.operations(); i++, c++)
            {
                result_ += *i;
            }
        }
    }
};


}


#endif
