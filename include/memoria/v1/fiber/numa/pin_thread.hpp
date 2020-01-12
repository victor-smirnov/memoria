
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_NUMA_PIN_THREAD_H
#define MEMORIA_FIBERS_NUMA_PIN_THREAD_H

#include <cstdint>
#include <thread>

#include <boost/config.hpp>

#include <memoria/v1/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {
namespace numa {

MEMORIA_FIBERS_DECL
void pin_thread( std::uint32_t, std::thread::native_handle_type);

MEMORIA_FIBERS_DECL
void pin_thread( std::uint32_t cpuid);

}}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_NUMA_PIN_THREAD_H
