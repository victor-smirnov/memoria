
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/memoria.hpp>

#include <memoria/v1/tools/profile_tests.hpp>
#include <memoria/v1/tools/tools.hpp>

#include <functional>
#include <memory>

namespace memoria {
namespace v1 {

using namespace std;

template <
    typename ContainerTypeName,
    typename AllocatorType,
    typename Profile
>
class BTTestBase: public TestTask {

    using MyType = BTTestBase<
                Profile,
                AllocatorType,
                ContainerTypeName
    >;

    using Base = TestTask;

protected:
    using CtrName           = ContainerTypeName;
    using Ctr               = typename CtrTF<Profile, ContainerTypeName>::Type;

    using IteratorPtr       = typename Ctr::IteratorPtr;
    using Iterator       	= typename Ctr::Iterator;


    using Allocator     = AllocatorType;
    using AllocatorPtr  = typename Allocator::AllocatorPtr;
    using SnapshotPtr   = typename Allocator::SnapshotPtr;

    AllocatorPtr allocator_;
    SnapshotPtr  snapshot_;

    String dump_name_;

public:

    BTTestBase(StringRef name):
        TestTask(name)
    {
        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
    }

    virtual ~BTTestBase() noexcept {}

    auto& allocator() {
        return allocator_;
    }

    const auto& allocator() const {
        return allocator_;
    }

    auto& snapshot() {
        return snapshot_;
    }

    const auto& snapshot() const {
        return snapshot_;
    }

    auto& branch()
    {
        if (snapshot_) {
            snapshot_ = snapshot_->branch();
        }
        else {
            snapshot_ = allocator_->master()->branch();
        }

        return snapshot_;
    }

    void commit()
    {
        snapshot_->commit();
        snapshot_->set_as_master();

        if (snapshot_->has_parent())
        {
            auto parent = snapshot_->parent();

            if (parent->has_parent())
            {
                parent->drop();
                allocator_->pack();
            }
        }
    }

    void drop()
    {
        auto parent = snapshot_->parent();

        snapshot_->drop();
        snapshot_.reset();

        parent->set_as_master();

        snapshot_ = parent;

        allocator_->pack();
    }


    void check(const SnapshotPtr& snapshot, const char* source)
    {
        v1::check(snapshot, "Snapshot check failed", source);
    }

    void check(const char* source)
    {
        v1::check(snapshot_, "Snapshot check failed", source);
    }

    void check(const SnapshotPtr& snapshot, const char* msg, const char* source)
    {
        v1::check(snapshot, msg, source);
    }

    void check(const char* msg, const char* source)
    {
        v1::check(snapshot_, msg, source);
    }

    // FIXME: remove it
    virtual void createAllocator(AllocatorPtr& allocator)
    {
        allocator = Allocator::create();
    }

    virtual void setUp()
    {
        Base::setUp();

        if (!isReplayMode())
        {
            createAllocator(allocator_);
            MEMORIA_V1_ASSERT_NOT_NULL(allocator_.get());
        }
        else {
            loadAllocator(dump_name_);
            snapshot_ = allocator_->master();
        }
    }

    virtual void tearDown()
    {
        if (snapshot_) {
            snapshot_.reset();
        }

        allocator_.reset();
    }

    virtual void onException() noexcept
    {
        try {

            if (snapshot_->is_active())
            {
                commit();

                auto file_name_invalid = getAllocatorFileName(".invalid");
                storeAllocator(file_name_invalid);

                drop();

                dump_name_ = getAllocatorFileName(".valid");
                storeAllocator(dump_name_);
            }
            else if (snapshot_->is_committed())
            {
                dump_name_ = getAllocatorFileName(".valid");
                storeAllocator(dump_name_);
            }
        }
        catch (...) {
            out() << "Exception is thrown in BTTestBase::onException()";
        }
    }

    virtual void storeAllocator(String file_name) const
    {
        auto out = FileOutputStreamHandler::create(file_name.c_str());
        allocator_->store(out.get());
    }


    virtual void loadAllocator(StringRef file_name)
    {
        auto in = FileInputStreamHandler::create(file_name.c_str());
        allocator_ = Allocator::load(in.get());
    }

    virtual void dumpAllocator()
    {
        String file_name = getAllocatorFileName("-allocator.dump");
        FSDumpAllocator(allocator_, file_name);
    }

    virtual void dumpSnapshot()
    {
        if (snapshot_)
        {
            String file_name = getAllocatorFileName("-snapshot.dump");
            FSDumpAllocator(snapshot_, file_name);
        }
    }



    virtual void checkAllocator(const char* msg, const char* source)
    {
        v1::check<Allocator>(this->allocator(), msg, source);
    }

    virtual String getAllocatorFileName(StringRef infix = "") const
    {
        return getResourcePath("Allocator"+infix+".dump");
    }

    bool checkSoftMemLimit()
    {
//      size_t allocated = allocator()->allocated();
//
//      return allocated <= this->soft_memlimit_;

        return true;
    }
};

}}
