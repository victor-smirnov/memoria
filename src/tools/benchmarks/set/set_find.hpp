
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BENCHMARKS_SET_FIND_HPP_
#define MEMORIA_BENCHMARKS_SET_FIND_HPP_

#include "../benchmarks_inc.hpp"

#include <malloc.h>
#include <memory>

namespace memoria {

using namespace std;



class SetFindRandomBenchmark: public SPBenchmarkTask {


    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;


    typedef typename SCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::ID                                     ID;
    typedef typename SetCtrType::Accumulator                            Accumulator;


    typedef typename SetCtrType::Key                                    Key;
    typedef typename SetCtrType::Value                                  Value;


    Allocator* allocator_;
    SetCtrType* set_;

    Int result_;

    Int* rd_array_;

public:

    SetFindRandomBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {
        average = 10;
    }

    virtual ~SetFindRandomBenchmark() throw() {}

    Key key(Int c) const
    {
        return c * 2 + 1;
    }

    virtual void Prepare(BenchmarkParameters& params, ostream& out)
    {
        allocator_ = new Allocator();
        allocator_->commit();

        Int size = params.x();

        String resource_name = "allocator."+toString(size)+".dump";

        if (false && IsResourceExists(resource_name))
        {
            LoadResource(*allocator_, resource_name);

            set_ = new SetCtrType(allocator_, CTR_FIND, 1);
        }
        else {
            set_ = new SetCtrType(allocator_, CTR_CREATE, 1);

            Iterator i = set_->End();

            for (Int c = 0; c < size; c++)
            {
            	i.insert(key(c), EmptyValue());
            }

            allocator_->commit();
//            StoreResource(*allocator_, resource_name);
        }

        rd_array_ = new Int[params.operations()];
        for (Int c = 0; c < params.operations(); c++)
        {
            rd_array_[c] = key(getRandom(size));
        }
    }

    virtual void release(ostream& out)
    {
        delete set_;
        delete allocator_;
        delete[] rd_array_;
    }

    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        for (Int c = 0; c < params.operations(); c++)
        {
        	auto key = rd_array_[c];

            if (!set_->find(key).is_found_eq(key))
            {
                cout<<"MISS!!!"<<endl; // this should't happen
            }
        }
    }
};


}


#endif
