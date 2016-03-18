
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_BT_TEST_BASE_HPP_
#define MEMORIA_TESTS_BT_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/profile_tests.hpp>
#include <memoria/tools/tools.hpp>

#include <functional>
#include <memory>

namespace memoria {

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

    using Iterator          = typename Ctr::Iterator;
    using IteratorPtr       = typename Ctr::IteratorPtr;
    using CtrSizesT         = typename Ctr::Types::CtrSizesT;


    using Allocator     = AllocatorType;
    using AllocatorPtr  = typename Allocator::AllocatorPtr;
    using SnapshotPtr   = typename Allocator::SnapshotPtr;

    AllocatorPtr allocator_;
    SnapshotPtr  snapshot_;

    String dump_name_;

    static constexpr Int Streams = Ctr::Types::Streams;

public:

    BTTestBase(StringRef name):
        TestTask(name)
    {
        Ctr::initMetadata();

        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
    }

    virtual ~BTTestBase() throw() {}

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
        ::memoria::check(snapshot, "Snapshot check failed", source);
    }

    void check(const char* source)
    {
        ::memoria::check(snapshot_, "Snapshot check failed", source);
    }

    void check(const SnapshotPtr& snapshot, const char* msg, const char* source)
    {
        ::memoria::check(snapshot, msg, source);
    }

    void check(const char* msg, const char* source)
    {
        ::memoria::check(snapshot_, msg, source);
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
            MEMORIA_ASSERT_NOT_NULL(allocator_.get());
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
        ::memoria::check<Allocator>(this->allocator(), msg, source);
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

}


#endif
