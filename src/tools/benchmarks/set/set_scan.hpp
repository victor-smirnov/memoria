
// Copyright 2012 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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