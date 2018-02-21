
//
//  boost/detail/spinlock.hpp
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  struct spinlock
//  {
//      void lock();
//      bool try_lock();
//      void unlock();
//
//      class scoped_lock;
//  };
//
//  #define MMA1_SP_DETAIL_SPINLOCK_INIT <unspecified>
//

#pragma once

#include <boost/config.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/sp_has_sync.hpp>
#include <memoria/v1/reactor/smart_ptr/detail/spinlock_std_atomic.hpp>
