
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_vctr_vctr_READ_HPP_
#define MEMORIA_BENCHMARKS_vctr_vctr_READ_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorReadBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::Profile                                              Profile;

    typedef typename SCtrTF<Vector<BigInt>>::Type                               VectorCtrType;
    typedef typename VectorCtrType::Iterator                                    Iterator;



    Allocator*      allocator_;
    VectorCtrType*  ctr_;

    Int result_;

    Int* rd_array_;

public:

    VectorReadBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {
        average = 5;
    }

    virtual ~VectorReadBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ 	= new Allocator();
        Int size 	= params.x();

        ctr_ = new VectorCtrType(allocator_);

        Iterator i = ctr_->seek(0);

        for (Int c = 0; c < size/128; c++)
        {
        	vector<BigInt> buffer(128);

            for (auto& d: buffer)
            {
                d = getRandom(10000);
            }

            i.insert(buffer);
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
        volatile BigInt buffer = 0;

        BigInt total = 0;

        for (Int c = 0; c < params.operations(); c++, total++)
        {
            buffer += ctr_->seek(rd_array_[c]).value();
        }

        params.memory() = total;
    }
};


}


#endif
