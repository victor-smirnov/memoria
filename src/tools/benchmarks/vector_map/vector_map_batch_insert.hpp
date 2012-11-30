
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_VECTOR_MAP_BATCH_INSERT_HPP_
#define MEMORIA_BENCHMARKS_VECTOR_MAP_BATCH_INSERT_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class VectorMapBatchinsertBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

//    typedef typename SmallCtrTypeFactory::Factory<Root>::Type       	RootCtr;
    typedef typename SmallCtrTypeFactory::Factory<VectorMapCtr>::Type   MapCtrType;
    typedef typename MapCtrType::Iterator                               Iterator;
    typedef typename MapCtrType::ID                                     ID;

    typedef typename MapCtrType::Idxset                                 SetCtrType;

    typedef typename SetCtrType::Accumulator                            Accumulator;

    typedef typename SetCtrType::ISubtreeProvider                       ISubtreeProvider;
    typedef typename SetCtrType::DefaultSubtreeProviderBase             DefaultSubtreeProviderBase;
    typedef typename SetCtrType::NonLeafNodeKeyValuePair                NonLeafNodeKeyValuePair;
    typedef typename SetCtrType::LeafNodeKeyValuePair                   LeafNodeKeyValuePair;


    class SubtreeProvider: public DefaultSubtreeProviderBase
    {
        typedef DefaultSubtreeProviderBase          Base;
        typedef typename ISubtreeProvider::Enum     Direction;

        BigInt data_size_;

    public:
        SubtreeProvider(SetCtrType* ctr, BigInt total, BigInt data_size): Base(*ctr, total), data_size_(data_size)
        {}

        virtual LeafNodeKeyValuePair getLeafKVPair(Direction direction, BigInt begin)
        {
            Accumulator acc;

            acc[0] = 1;
            acc[1] = data_size_;

            return LeafNodeKeyValuePair(acc, SetCtrType::Value());
        }
    };


    Allocator*  allocator_;
    MapCtrType*     map_;

    BigInt      memory_size;
    Int         data_size_;
public:

    VectorMapBatchinsertBenchmark(StringRef name, Int data_size):
        SPBenchmarkTask(name), memory_size(128*1024*1024), data_size_(data_size)
    {
//        RootCtr::initMetadata();
        MapCtrType::initMetadata();

        Add("memory_size", memory_size);
    }

    virtual ~VectorMapBatchinsertBenchmark() throw() {}

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();

        map_ = new MapCtrType(allocator_, 1, true);

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

        ArrayData data(size * data_size_, malloc(size * data_size_), true);

        SubtreeProvider provider(&map_->set(), size, data_size_);

        BigInt total = 0;

        BigInt key_count = 0;

        while (total < memory_size)
        {
            auto i = key_count == 0 ? map_->Begin() : map_->find(getRandom(key_count));

            map_->set().insertSubtree(i.is_iter(), provider);

            i.ba_iter().insert(data);

            total += data.size();
            key_count += size;
        }

        params.operations() = map_->count();
        params.memory()     = map_->size() + map_->count() * 16; //sizeof(BigInt) * 2

        allocator_->commit();
    }
};


}


#endif
