

//          Copyright Victor Smirnov 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/config.hpp>

#include <boost/assert.hpp>

#include <memoria/fiber/context.hpp>
#include <memoria/fiber/detail/config.hpp>
#include <memoria/fiber/detail/spinlock.hpp>


#include <memoria/fiber/mutex.hpp>
#include <memoria/fiber/recursive_mutex.hpp>
#include <memoria/fiber/condition_variable.hpp>
#include <memoria/fiber/operations.hpp>

#include <cstdint>
#include <mutex>

#ifdef BOOST_HAS_ABI_HEADERS
#  include MEMORIA_BOOST_ABI_PREFIX
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace memoria {
namespace fibers {
    
    
// Note that this shared mutex is not up/down-gradable. That means an owner holding
// exclusive lock must not obtain shared lock and vise versa.

class shared_mutex {
    using Mutex = mutex;
    
    Mutex lock_;
    condition_variable shared_cv_;
    condition_variable exclusive_cv_;
    
    uint64_t shared_locks_, exclusive_locks_; 
    uint64_t shared_waiters_, exclusive_waiters_;
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
        
        if (shared_locks_ || exclusive_locks_)
        {
            exclusive_waiters_++;
            
            exclusive_cv_.wait(guard, [this]{
                return !(shared_locks_ || exclusive_locks_);
            });
            
            exclusive_waiters_--;
        }
        
        exclusive_locks_ = 1;
    }
    
    
    void unlock() 
    {
        std::unique_lock<Mutex> guard(lock_);

        exclusive_locks_ = 0;
            
        if (exclusive_waiters_) 
        {
            exclusive_cv_.notify_one();
        }
        else if (shared_waiters_)
        {
            shared_cv_.notify_all();
        }
    }
    
    bool try_lock()
    {
        std::unique_lock<Mutex> guard(lock_);
        
        if (shared_locks_ || exclusive_locks_)
        {
            return false;
        }
        
        exclusive_locks_++;
        
        return true;
    }
};
    
}}
