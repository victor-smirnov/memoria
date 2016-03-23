
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {
namespace v1 {

using namespace std;




class VectorMapSequentialReadBenchmark: public SPBenchmarkTask {


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename DCtrTF<VectorMap<BigInt, Byte>>::Type                      Ctr;
    typedef typename Ctr::Iterator                                              Iterator;

    Allocator*  allocator_;
    Ctr*        ctr_;

    BigInt      memory_size;

public:

    VectorMapSequentialReadBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);

        average = 10;
    }

    virtual ~VectorMapSequentialReadBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        Int size = params.x();
        MemBuffer<Byte> data(size);

        BigInt total = 0;

        ctr_ = new Ctr(allocator_);

        while (total < memory_size)
        {
            auto i = ctr_->create();
            i.insert(data);
            total += data.size();
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
        Int size = params.x();
        MemBuffer<Byte> data(size);

        for (auto i = ctr_->Begin(); !i.isEnd(); i++)
        {
            i.read(data);
        }

        params.operations() = ctr_->count();
        params.memory()     = ctr_->size() + ctr_->count() * 16; //sizeof(BigInt) * 2
    }
};


}}