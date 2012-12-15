
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_INSERT_BATCH_HPP_
#define MEMORIA_BENCHMARKS_SET_INSERT_BATCH_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetInsertBatchBenchmark: public SPBenchmarkTask {
public:

    Int max_size;

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

    typedef typename SCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::ID                                     ID;
    typedef typename SetCtrType::Accumulator                            Accumulator;


    typedef typename SetCtrType::Key                                    Key;
    typedef typename SetCtrType::Value                                  Value;

    typedef typename SetCtrType::ISubtreeProvider                       ISubtreeProvider;
    typedef typename SetCtrType::DefaultSubtreeProviderBase             DefaultSubtreeProviderBase;
    typedef typename SetCtrType::NonLeafNodeKeyValuePair                NonLeafNodeKeyValuePair;
    typedef typename SetCtrType::LeafNodeKeyValuePair                   LeafNodeKeyValuePair;


    class SubtreeProvider: public DefaultSubtreeProviderBase
    {
        typedef DefaultSubtreeProviderBase          Base;
        typedef typename ISubtreeProvider::Enum     Direction;
    public:
        SubtreeProvider(SetCtrType* ctr, BigInt total): Base(*ctr, total) {}

        virtual LeafNodeKeyValuePair getLeafKVPair(Direction direction, BigInt begin)
        {
            Accumulator acc;
            acc[0] = 1;
            return LeafNodeKeyValuePair(acc, Value());
        }
    };


    Allocator*  allocator_;
    SetCtrType*     set_;

public:

    SetInsertBatchBenchmark(StringRef name):
        SPBenchmarkTask(name), max_size(16*1024*1024)
    {
        Add("max_size", max_size);
    }

    virtual ~SetInsertBatchBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        set_ = new SetCtrType(allocator_, 1, true);
    }

    virtual void release(ostream& out)
    {
        delete set_;
        delete allocator_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        Int size = params.x();

        SubtreeProvider provider(set_, size);

        Int map_size = 0;

        params.operations() = this->max_size;

        for (Int c = 0; c < params.operations() / size; c++)
        {
            Int pos = getRandom(map_size - 1) + 1;
            auto i = map_size == 0? set_->End() : set_->find(pos);

            set_->insertSubtree(i, provider);

            map_size += size;
        }

        allocator_->rollback();
    }
};


}


#endif
