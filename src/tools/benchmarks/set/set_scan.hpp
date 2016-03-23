
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



class SetScanBenchmark: public SPBenchmarkTask {

    typedef SPBenchmarkTask Base;

    typedef typename Base::Allocator    Allocator;
    typedef typename Base::Profile      Profile;

    typedef typename DCtrTF<Set1>::Type                                 SetCtrType;
    typedef typename SetCtrType::Iterator                               Iterator;
    typedef typename SetCtrType::ID                                     ID;
    typedef typename SetCtrType::BranchNodeEntry                            BranchNodeEntry;


    typedef typename SetCtrType::Key                                    Key;
    typedef typename SetCtrType::Value                                  Value;


    Allocator*  allocator_;
    SetCtrType* set_;

    Int         result_;

public:

    SetScanBenchmark(StringRef name):
        SPBenchmarkTask(name)
    {
        SetCtrType::initMetadata();
    }

    virtual ~SetScanBenchmark() throw() {}

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

        if (IsResourceExists(resource_name))
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

            StoreResource(*allocator_, resource_name);
        }
    }

    virtual void release(ostream& out)
    {
        delete set_;
        delete allocator_;
    }


    virtual void Benchmark(BenchmarkParameters& params, ostream& out)
    {
        BigInt total = 0;

        for (Int c = 0; c < params.operations(); )
        {
            for (auto i = set_->begin(); !i.isEnd() && c < params.operations(); i++, c++)
            {
                total += i.key();
            }
        }
    }
};


}}