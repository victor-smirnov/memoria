
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_FUTURE_STATUS_HPP
#define MEMORIA_FIBERS_FUTURE_STATUS_HPP

#include <future>

#include <boost/config.hpp>

#include <memoria/v1/fiber/detail/config.hpp>

namespace memoria { namespace v1 {
namespace fibers {

enum class future_status {
    ready = 1,
    timeout,
    deferred
};

}}}

#endif // MEMORIA_FIBERS_FUTURE_STATUS_HPP
