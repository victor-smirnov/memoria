// Copyright 2021 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>

namespace memoria {

using LifetimeInvalidationListener = std::function<void()>;

namespace detail {

class LifetimeGuardShared {
protected:
    uint64_t refs_;
    bool valid_;
public:
    LifetimeGuardShared() noexcept:
        refs_(0), valid_(true)
    {}

    virtual ~LifetimeGuardShared() noexcept = default;

    void ref() noexcept {
        ++refs_;
    }

    bool unref() noexcept {
        return --refs_ == 0;
    }

    bool has_refs() const noexcept {
        return refs_ != 0;
    }

    virtual void destroy() noexcept = 0;
    virtual void invalidate() noexcept = 0;

    bool is_invalid() const noexcept {
        return !valid_;
    }
};

class DefaultLifetimeGuardSharedImpl: public detail::LifetimeGuardShared {
    using CleanupFn = std::function<void()>;
    CleanupFn cleanup_fn_;
public:
    DefaultLifetimeGuardSharedImpl(CleanupFn fn) noexcept:
         cleanup_fn_(fn)
    {}

    void destroy() noexcept {
        if (valid_) {
            cleanup_fn_();
        }

        delete this;
    }

    void invalidate() noexcept {
        valid_ = false;
    }
};

}


class LifetimeGuard {

    detail::LifetimeGuardShared* shared_;

public:
    LifetimeGuard(detail::LifetimeGuardShared* shared) noexcept:
        shared_(shared)
    {
        shared_->ref();
    }

    LifetimeGuard() noexcept:
        shared_(nullptr)
    {}

    LifetimeGuard(const LifetimeGuard& other) noexcept:
        shared_(other.shared_)
    {
        if (shared_) {
            shared_->ref();
        }
    }

    LifetimeGuard(LifetimeGuard&& other) noexcept:
        shared_(other.shared_)
    {
        other.shared_ = nullptr;
    }

    ~LifetimeGuard() noexcept {
        do_unref();
    }

    LifetimeGuard& operator=(const LifetimeGuard& other) noexcept {
        if (this == &other) {
            return *this;
        }

        do_unref();
        shared_ = other.shared_;

        if (shared_) {
            shared_->ref();
        }

        return *this;
    }

    LifetimeGuard& operator=(LifetimeGuard&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        shared_ = other.shared_;

        other.shared_ = nullptr;

        return *this;
    }

    bool is_valid() const noexcept {
        return shared_ && !shared_->is_invalid();
    }

private:
    void do_unref() noexcept {
        if (shared_) {
            if (shared_->unref()) {
                shared_->destroy();
            }
            shared_ = nullptr;
        }
    }
};


template <typename View>
class GuardedView {
    View view_;
    LifetimeGuard guard_;
public:
    GuardedView() noexcept {}

    GuardedView(View view, LifetimeGuard guard) noexcept :
        view_(view), guard_(guard)
    {}

    GuardedView(const GuardedView& gview) noexcept :
        view_(gview.view_), guard_(gview.guard_)
    {}

    GuardedView(GuardedView&& gview) noexcept :
        view_(std::move(gview.view_)), guard_(std::move(gview.guard_))
    {}

    GuardedView& operator=(const GuardedView& other) noexcept {
        if (this == &other) {
            return *this;
        }

        view_ = other.view_;
        guard_ = other.guard_;

        return *this;
    }

    GuardedView& operator=(GuardedView&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        view_  = std::move(other.view_);
        guard_ = std::move(other.guard_);

        return *this;
    }

    View& view() {
        if (guard_.is_valid()) {
            return view_;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Accessing stale view");
        }
    }

    const View& view() const {
        if (guard_.is_valid()) {
            return view_;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Accessing stale view");
        }
    }

    bool is_valid() const noexcept {
        return guard_.is_valid();
    }

    LifetimeGuard guard() const noexcept {
        return guard_;
    }
};



}
