
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



class VectorSequentialReadBenchmark: public SPBenchmarkTask {


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<Vector<UByte>>::Type                                VectorCtrType;
    typedef typename VectorCtrType::Iterator                                    Iterator;

    Allocator*      allocator_;
    VectorCtrType*  ctr_;

    BigInt      memory_size;

public:

    VectorSequentialReadBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);

        average = 10;
    }

    virtual ~VectorSequentialReadBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        ctr_ = new VectorCtrType(allocator_);

        BigInt size = 1024*1024;

        vector<UByte> data(size);

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

        vector<UByte> data(size);

        for (Iterator i = ctr_->seek(0); !i.isEof();)
        {
            i.read(data);
        }

        params.operations() = memory_size / size;
        params.memory()     = memory_size;
    }
};


}
