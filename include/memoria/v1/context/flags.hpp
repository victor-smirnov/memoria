
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_FLAGS_H
#define MEMORIA_CONTEXT_FLAGS_H

# include <boost/config.hpp>

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
# endif

namespace memoria { namespace v1 {
namespace context {

struct exec_ontop_arg_t {};
const exec_ontop_arg_t exec_ontop_arg{};

}}}

# ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
# endif

#endif // MEMORIA_CONTEXT_FLAGS_H
