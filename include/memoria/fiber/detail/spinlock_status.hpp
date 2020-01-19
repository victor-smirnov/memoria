
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_SPINLOCK_STATUS_H
#define MEMORIA_FIBERS_SPINLOCK_STATUS_H

namespace memoria {
namespace fibers {
namespace detail {

enum class spinlock_status {
    locked = 0,
    unlocked
};

}}}

#endif // MEMORIA_FIBERS_SPINLOCK_STATUS_H
