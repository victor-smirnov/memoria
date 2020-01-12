
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "memoria/v1/fiber/numa/topology.hpp"

#include <system_error>

#include "memoria/v1/fiber/exceptions.hpp"

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {
namespace numa {

#if BOOST_COMP_CLANG || \
    BOOST_COMP_GNUC || \
    BOOST_COMP_INTEL ||  \
    BOOST_COMP_MSVC 
# pragma message "topology() not supported"
#endif

MEMORIA_FIBERS_DECL
std::vector< node > topology() {
    throw fiber_error{
        std::make_error_code( std::errc::function_not_supported),
            "boost fiber: topology() not supported" };
    return std::vector< node >{};
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_SUFFIX
#endif
