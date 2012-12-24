
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_APPEND_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_APPEND_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorMapAppendBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;


    typedef typename SCtrTF<VectorMap<BigInt, Byte>>::Type                      Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;


    Allocator*  allocator_;
    Ctr*        map_;

    BigInt      memory_size;

public:

    VectorMapAppendBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        Add("memory_size", memory_size);
    }

    virtual ~VectorMapAppendBenchmark() throw() {}


    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        map_ = new Ctr(allocator_);

        allocator_->commit();
    }

    virtual void release(ostream& out)
    {
        delete map_;
        delete allocator_;
    }


    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        MemBuffer<Byte> data(size);

        BigInt total = 0;

        auto i = map_->End();

        while (total < memory_size)
        {
            map_->createNew(i);
            i.insert(data);
            i++;

            total += data.size();
        }

        params.operations() = map_->count();
        params.memory()     = map_->size() + map_->count() * 16; //sizeof(BigInt) * 2

        allocator_->commit();

    }
};


}


#endif
