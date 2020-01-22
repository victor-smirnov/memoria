
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

#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>
#include <memoria/tests/yaml.hpp>
#include <memoria/tests/tools.hpp>

#include <memoria/profiles/default/default.hpp>
#include <memoria/api/store/memory_store_api.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/reactor/reactor.hpp>

#include <functional>
#include <memory>



namespace memoria {
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
    typename StoreType,
    typename Profile
>
class BTTestBase: public TestState {

    using MyType = BTTestBase;

    using Base = TestState;

protected:
    using CtrName = ContainerTypeName;
    using CtrApi  = ICtrApi<ContainerTypeName, Profile>;


    using Store = typename StoreType::element_type;
    using StorePtr = StoreType;
    using SnapshotPtr = typename Store::SnapshotPtr;

    StorePtr store_;
    SnapshotPtr snapshot_;

    int64_t size_{};
    UUID snapshot_id_{};

    RngInt int_generator_{};
    RngInt64 bigint_generator_{};

public:
    MMA_STATE_FILEDS(size_, snapshot_id_);
    MMA_INDIRECT_STATE_FILEDS(store_);

    BTTestBase()
    {
        store_ = Store::create();
        snapshot_  = store_->master();
    }

    auto& store() {
        return store_;
    }

    const auto& store() const {
        return store_;
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
            snapshot_ = store_->master()->branch();
        }

        snapshot_id_ = snapshot_->uuid();

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
                store_->pack();
            }
        }
    }

    void drop()
    {
        auto parent = snapshot_->parent();

        snapshot_->drop();
        snapshot_->reset();

        parent->set_as_master();

        snapshot_ = parent;

        store_->pack();
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
            snapshot_ = store_->find(snapshot_id_);
        }
    }

    virtual void tear_down() noexcept
    {}

    virtual void on_test_failure() noexcept
    {
        try {
            if (snapshot_->is_active())
            {
                snapshot_id_ = snapshot_->parent()->uuid();
                snapshot_->commit();

                store_->pack();
            }
        }
        catch (...) {
            out() << "Exception is thrown in BTTestBase::on_test_failure()";
        }
    }

    virtual void storeAllocator(U8String file_name)
    {   
        store_->store(file_name.to_u8());
    }


    virtual void loadAllocator(U8String file_name)
    {
        store_ = Store::load(file_name.to_u8());
    }


    virtual void checkAllocator(const char* msg, const char* source)
    {
        tests::check(store_, msg, source);
    }

    bool checkSoftMemLimit()
    {
        return true;
    }

    bool isReplayMode() const noexcept {
        return this->is_replay();
    }

    U8String getResourcePath(const U8String& resource)
    {
        filesystem::path path = this->working_directory_;
        path.append(resource.to_std_string());

        return path.string();
    }

    template <typename... Args>
    void coutln(const char* format, Args&&... args) {
        reactor::engine().coutln(format, std::forward<Args>(args)...);
    }
};

}}
