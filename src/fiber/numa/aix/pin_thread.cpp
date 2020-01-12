
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "memoria/v1/fiber/numa/pin_thread.hpp"

extern "C" {
#include <sys/errno.h>
#include <sys/processor.h>
#include <sys/thread.h>
}

#include <system_error>

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {
namespace numa {

MEMORIA_FIBERS_DECL
void pin_thread( std::uint32_t cpuid) {
    pin_thread( cpuid, ::thread_self() );
}

MEMORIA_FIBERS_DECL
void pin_thread( std::uint32_t cpuid, std::thread::native_handle_type h) {
    if ( BOOST_UNLIKELY( -1 == ::bindprocessor( BINDTHREAD, h, static_cast< cpu_t >( cpuid) ) ) ) {
        throw std::system_error(
                std::error_code( errno, std::system_category() ),
                "bindprocessor() failed");
    }
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_SUFFIX
#endif
