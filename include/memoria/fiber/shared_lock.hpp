
//          Copyright Victor Smirnov 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once


#include <cstdint>
#include <mutex>

namespace memoria {
namespace fibers {


template <typename Mutex>    
class shared_lock {
    Mutex& mutex_;
    
public:
    shared_lock(Mutex& mutex): mutex_(mutex)
    {
        mutex_.lock_shared();
    }
    
    shared_lock(Mutex& mutex, std::adopt_lock_t t): mutex_(mutex)
    {}
    
    ~shared_lock()
    {
        mutex_.unlock_shared();
    }
};



    
}}
