
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>

#include <boost/config.hpp>

#include <memoria/v1/fiber/condition_variable.hpp>
#include <memoria/v1/fiber/detail/config.hpp>
#include <memoria/v1/fiber/mutex.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace memoria {
namespace v1 {    
namespace fibers {

class MEMORIA_V1_FIBERS_DECL barrier {
private:
	std::size_t         initial_;
	std::size_t         current_;
	bool                cycle_{ true };
	mutex               mtx_{};
	condition_variable  cond_{};

public:
	explicit barrier( std::size_t);

    barrier( barrier const&) = delete;
    barrier & operator=( barrier const&) = delete;

	bool wait();
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

