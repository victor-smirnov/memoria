

//          Copyright Victor Smirnov 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/config.hpp>

#include <boost/assert.hpp>

#include <memoria/v1/fiber/context.hpp>
#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/detail/spinlock.hpp>


#include <memoria/v1/fiber/mutex.hpp>
#include <memoria/v1/fiber/recursive_mutex.hpp>
#include <memoria/v1/fiber/condition_variable.hpp>
#include <memoria/v1/fiber/operations.hpp>

#include <cstdint>
#include <mutex>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

// Note that this shared mutex is not up/down-gradable. That means an owner holding
// exclusive lock must not obtain shared lock and vise versa.
    
class recursive_shared_mutex {
    using Mutex     = mutex;
    using FiberId   = typename fiber::id;
    
    Mutex lock_;
    condition_variable shared_cv_;
    condition_variable exclusive_cv_;
    
    uint64_t shared_locks_, exclusive_locks_; 
    uint64_t shared_waiters_, exclusive_waiters_;
    
    FiberId exclusive_owner_id_;
public:
    void lock_shared() 
    {
        std::unique_lock<Mutex> guard(lock_);
        
        if (exclusive_locks_ || exclusive_waiters_) 
        {
            shared_waiters_++;
            
            shared_cv_.wait(guard, [this]{
                return !(exclusive_locks_ || exclusive_waiters_);
            });
            
            shared_waiters_--;
        }
        
        shared_locks_++;
    }
    
    void unlock_shared()
    {
        std::unique_lock<Mutex> guard(lock_);
        
        shared_locks_--;
    
        if (exclusive_waiters_) 
        {
            exclusive_cv_.notify_one();
        }
    }
    
    bool try_lock_shared()
    {
        std::unique_lock<Mutex> guard(lock_);
        
        if (exclusive_locks_ || exclusive_waiters_)
        {
            return false;
        }
        
        shared_locks_++;
        
        return true;
    }
    
    void lock()
    {
        std::unique_lock<Mutex> guard(lock_);
        
        if (shared_locks_ || is_locked_exclusively_not_by_me())
        {
            exclusive_waiters_++;
            
            exclusive_cv_.wait(guard, [this]{
                return !(shared_locks_ || is_locked_exclusively_not_by_me());
            });
            
            exclusive_waiters_--;
        }
        
        exclusive_locks_++;
        exclusive_owner_id_ = this_fiber::get_id();
    }
    
    
    void unlock() 
    {
        std::unique_lock<Mutex> guard(lock_);

        if (--exclusive_locks_ == 0) 
        {
            exclusive_owner_id_ = FiberId();
            
            if (exclusive_waiters_) 
            {
                exclusive_cv_.notify_one();
            }
            else if (shared_waiters_)
            {
                shared_cv_.notify_all();
            }   
        }
    }
    
    bool try_lock() 
    {
        std::unique_lock<Mutex> guard(lock_);
        
        if (shared_locks_ || is_locked_exclusively_not_by_me()) 
        {
            return false;
        }
        
        exclusive_locks_++;
        
        return true;
    }
    
private:
    bool is_locked_exclusively_not_by_me() const {
        return exclusive_locks_ && exclusive_owner_id_ != this_fiber::get_id();
    }
};



    
}}}
