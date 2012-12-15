
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_APPEND_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_APPEND_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorappendBenchmark: public SPBenchmarkTask {
public:



    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;


    typedef typename SmallCtrTypeFactory::Factory<Vector<UByte> >::Type         MapCtrType;
    typedef typename MapCtrType::Iterator                                       Iterator;
    typedef typename MapCtrType::ID                                             ID;


    Allocator*  allocator_;
    MapCtrType* map_;

    Int         memory_size;


public:

    VectorappendBenchmark(StringRef name):
        SPBenchmarkTask(name), memory_size(128*1024*1024)
    {
        MapCtrType::initMetadata();

        Add("memory_size", memory_size);
    }

    virtual ~VectorappendBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_  = new Allocator();
        map_        = new MapCtrType(allocator_, 1, true);

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

        auto i = map_->End();

        BigInt total = 0;

        params.operations() = 0;

        ArrayData<UByte> data(size, malloc(size), true);

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


}


#endif
