
//          Copyright Oliver Kowalke 2013.
//          Copyright Victor Smirnov 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <memoria/v1/fiber/context.hpp>
#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/convert.hpp>
#include <memoria/v1/fiber/detail/spinlock.hpp>
#include <memoria/v1/fiber/exceptions.hpp>
#include <memoria/v1/fiber/mutex.hpp>
#include <memoria/v1/fiber/operations.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
//# pragma warning(disable:4251)
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

enum class cv_status {
    no_timeout = 1,
    timeout
};

class MEMORIA_V1_FIBERS_DECL condition_variable_any {
private:
    typedef context::wait_queue_t   wait_queue_t;

    wait_queue_t        wait_queue_{};
    detail::spinlock    wait_queue_splk_{};

public:
    condition_variable_any() = default;

    ~condition_variable_any() {
        BOOST_ASSERT( wait_queue_.empty() );
    }

    condition_variable_any( condition_variable_any const&) = delete;
    condition_variable_any & operator=( condition_variable_any const&) = delete;

    void notify_one() noexcept;

    void notify_all() noexcept;

    template< typename LockType >
    void wait( LockType & lt) {
        context * ctx = context::active();
        // atomically call lt.unlock() and block on *this
        // store this fiber in waiting-queue
        detail::spinlock_lock lk( wait_queue_splk_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // unlock external lt
        lt.unlock();
        // suspend this fiber
        ctx->suspend( lk);
        // relock local lk
        lk.lock();
        // remove from waiting-queue
        ctx->wait_unlink();
        // unlock local lk
        lk.unlock();
        // relock external again before returning
        try {
            lt.lock();
        } catch (...) {
            std::terminate();
        }
        // post-conditions
        BOOST_ASSERT( ! ctx->wait_is_linked() );
    }

    template< typename LockType, typename Pred >
    void wait( LockType & lt, Pred pred) {
        while ( ! pred() ) {
            wait( lt);
        }
    }

    template< typename LockType, typename Clock, typename Duration >
    cv_status wait_until( LockType & lt, std::chrono::time_point< Clock, Duration > const& timeout_time_) {
        cv_status status = cv_status::no_timeout;
        std::chrono::steady_clock::time_point timeout_time(
                detail::convert( timeout_time_) );
        context * ctx = context::active();
        // atomically call lt.unlock() and block on *this
        // store this fiber in waiting-queue
        detail::spinlock_lock lk( wait_queue_splk_);
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        ctx->wait_link( wait_queue_);
        // unlock external lt
        lt.unlock();
        // suspend this fiber
        if ( ! ctx->wait_until( timeout_time, lk) ) {
            status = cv_status::timeout;
        }
        // relock local lk
        lk.lock();
        // remove from waiting-queue
        ctx->wait_unlink();
        // unlock local lk
        lk.unlock();
        // relock external again before returning
        try {
            lt.lock();
        } catch (...) {
            std::terminate();
        }
        // post-conditions
        BOOST_ASSERT( ! ctx->wait_is_linked() );
        return status;
    }

    template< typename LockType, typename Clock, typename Duration, typename Pred >
    bool wait_until( LockType & lt,
                     std::chrono::time_point< Clock, Duration > const& timeout_time, Pred pred) {
        while ( ! pred() ) {
            if ( cv_status::timeout == wait_until( lt, timeout_time) ) {
                return pred();
            }
        }
        return true;
    }

    template< typename LockType, typename Rep, typename Period >
    cv_status wait_for( LockType & lt, std::chrono::duration< Rep, Period > const& timeout_duration) {
        return wait_until( lt,
                           std::chrono::steady_clock::now() + timeout_duration);
    }

    template< typename LockType, typename Rep, typename Period, typename Pred >
    bool wait_for( LockType & lt, std::chrono::duration< Rep, Period > const& timeout_duration, Pred pred) {
        return wait_until( lt,
                           std::chrono::steady_clock::now() + timeout_duration,
                           pred);
    }
};

class MEMORIA_V1_FIBERS_DECL condition_variable {
private:
    condition_variable_any      cnd_;

public:
    condition_variable() = default;

    condition_variable( condition_variable const&) = delete;
    condition_variable & operator=( condition_variable const&) = delete;

    void notify_one() noexcept {
        cnd_.notify_one();
    }

    void notify_all() noexcept {
        cnd_.notify_all();
    }

    template <typename Mutex>
    void wait( std::unique_lock< Mutex > & lt) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cnd_.wait( lt);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
    }

    template< typename Mutex, typename Pred >
    void wait( std::unique_lock< Mutex > & lt, Pred pred) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cnd_.wait( lt, pred);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
    }

    template< typename Mutex, typename Clock, typename Duration >
    cv_status wait_until( std::unique_lock< Mutex > & lt,
                          std::chrono::time_point< Clock, Duration > const& timeout_time) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cv_status result = cnd_.wait_until( lt, timeout_time);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }

    template< typename Mutex, typename Clock, typename Duration, typename Pred >
    bool wait_until( std::unique_lock< Mutex > & lt,
                     std::chrono::time_point< Clock, Duration > const& timeout_time, Pred pred) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        bool result = cnd_.wait_until( lt, timeout_time, pred);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }

    template< typename Mutex, typename Rep, typename Period >
    cv_status wait_for(std::unique_lock< Mutex > & lt,
                        std::chrono::duration< Rep, Period > const& timeout_duration) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        cv_status result = cnd_.wait_for( lt, timeout_duration);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }

    template< typename Mutex, typename Rep, typename Period, typename Pred >
    bool wait_for( std::unique_lock< Mutex > & lt,
                   std::chrono::duration< Rep, Period > const& timeout_duration, Pred pred) {
        // pre-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        bool result = cnd_.wait_for( lt, timeout_duration, pred);
        // post-condition
        BOOST_ASSERT( lt.owns_lock() );
        BOOST_ASSERT( context::active() == lt.mutex()->owner_);
        return result;
    }
};

}}}

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif


