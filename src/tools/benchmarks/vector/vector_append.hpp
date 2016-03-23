
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

#include <memoria/v1/containers/vector/vctr_factory.hpp>

namespace memoria {
namespace v1 {

using namespace std;



class VectorAppendBenchmark: public SPBenchmarkTask {
public:



    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;


    typedef typename DCtrTF<Vector<UByte>>::Type                                Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;


    Allocator*  allocator_;
    Ctr*        ctr_;

    Int         memory_size;


public:

    VectorAppendBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);
    }

    virtual ~VectorAppendBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_  = new Allocator();
        ctr_        = new Ctr(allocator_);

        allocator_->commit();
    }

    virtual void release(ostream& out)
    {
        delete ctr_;
        delete allocator_;
    }


    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        auto i = ctr_->End();

        BigInt total = 0;

        params.operations() = 0;

        vector<UByte> data(size);

        while (total < memory_size)
        {
            i.insert(data);

            total += data.size();

            params.operations()++;
        }

        params.memory() = total;

        allocator_->rollback();
    }
};


}}