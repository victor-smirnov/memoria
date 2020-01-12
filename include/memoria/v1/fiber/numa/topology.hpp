
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_FIBERS_NUMA_TOPOLOGY_H
#define MEMORIA_FIBERS_NUMA_TOPOLOGY_H

#include <cstdint>
#include <set>
#include <vector>

#include <boost/config.hpp>

#include <memoria/v1/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_PREFIX
#endif

namespace memoria { namespace v1 {
namespace fibers {
namespace numa {

struct node {
    std::uint32_t                   id;
    std::set< std::uint32_t >       logical_cpus;
    std::vector< std::uint32_t >    distance;
};

inline
bool operator<( node const& lhs, node const& rhs) noexcept {
    return lhs.id < rhs.id;
}

MEMORIA_FIBERS_DECL
std::vector< node > topology();

}}}}

#ifdef BOOST_HAS_ABI_HEADERS
# include MEMORIA_BOOST_ABI_SUFFIX
#endif

#endif // MEMORIA_FIBERS_NUMA_TOPOLOGY_H
