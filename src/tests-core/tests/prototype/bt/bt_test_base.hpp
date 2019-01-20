
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>
#include <memoria/v1/tests/yaml.hpp>
#include <memoria/v1/tests/tools.hpp>

#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/allocator/allocator_inmem_api.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/reactor/reactor.hpp>

#include <functional>
#include <memory>



namespace memoria {
namespace v1 {
namespace tests {

template <typename T>
struct AlwaysCommitHandler {
    T& test_;

    ~AlwaysCommitHandler() {
        test_.commit();
    }
};


template <
    typename ContainerTypeName,
    typename AllocatorType,
    typename Profile
>
class BTTestBase: public TestState {

    using MyType = BTTestBase<
                Profile,
                AllocatorType,
                ContainerTypeName
    >;

    using Base = TestState;

protected:
    using CtrName           = ContainerTypeName;
    using Ctr               = CtrApi<ContainerTypeName, Profile>;

    using Iterator          = IterApi<CtrName, Profile>;

    using Allocator     = AllocatorType;
    using AllocatorPtr  = Allocator;
    using SnapshotPtr   = typename Allocator::SnapshotPtr;

    AllocatorPtr allocator_;
    SnapshotPtr  snapshot_;

    int64_t size_{};
    UUID snapshot_id_{};

    RngInt int_generator_{};
    RngInt64 bigint_generator_{};

public:
    MMA1_STATE_FILEDS(size_, snapshot_id_);
    MMA1_INDIRECT_STATE_FILEDS(allocator_);

    BTTestBase()
    {
        allocator_ = Allocator::create();
        snapshot_  = allocator_.master();
    }

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
            snapshot_ = snapshot_.branch();
        }
        else {
            snapshot_ = allocator_.master().branch();
        }

        snapshot_id_ = snapshot_.uuid();

        return snapshot_;
    }

    void commit()
    {
        snapshot_.commit();
        snapshot_.set_as_master();

        if (snapshot_.has_parent())
        {
            auto parent = snapshot_.parent();

            if (parent.has_parent())
            {
                parent.drop();
                allocator_.pack();
            }
        }
    }

    void drop()
    {
        auto parent = snapshot_.parent();

        snapshot_.drop();
        snapshot_.reset();

        parent.set_as_master();

        snapshot_ = parent;

        allocator_.pack();
    }


    void check(SnapshotPtr& snapshot, const char* source)
    {
        tests::check(snapshot, "Snapshot check failed", source);
    }

    void check(const char* source)
    {
        tests::check(snapshot_, "Snapshot check failed", source);
    }

    void check(SnapshotPtr& snapshot, const char* msg, const char* source)
    {
        tests::check(snapshot, msg, source);
    }

    void check(const char* msg, const char* source)
    {
        tests::check(snapshot_, msg, source);
    }

    virtual void set_up() noexcept
    {
        if (is_replay())
        {
            snapshot_ = allocator_.find(snapshot_id_);
        }
    }

    virtual void tear_down() noexcept
    {}

    virtual void on_test_failure() noexcept
    {
        try {
            if (snapshot_.is_active())
            {
                snapshot_id_ = snapshot_.parent().uuid();
                snapshot_.commit();

                allocator_.pack();
            }
        }
        catch (...) {
            out() << "Exception is thrown in BTTestBase::on_test_failure()";
        }
    }

    virtual void storeAllocator(U16String file_name)
    {   
        allocator_.store(file_name.to_u8().data());
    }


    virtual void loadAllocator(U16String file_name)
    {
        allocator_ = Allocator::load(file_name.to_u8().data());
    }


    virtual void checkAllocator(const char* msg, const char* source)
    {
        tests::check<Allocator>(allocator_, msg, source);
    }

    bool checkSoftMemLimit()
    {
        return true;
    }

    bool isReplayMode() const noexcept {
        return this->is_replay();
    }

    U16String getResourcePath(const U16String& resource)
    {
        filesystem::path path = this->working_directory_;
        path.append(resource.to_u8().to_std_string());

        return path.to_u16();
    }

    template <typename... Args>
    void coutln(const char16_t* format, Args&&... args) {
        reactor::engine().coutln(format, std::forward<Args>(args)...);
    }
};

}}}
