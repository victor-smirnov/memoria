
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CONTEXT_STACK_TRAITS_H
#define MEMORIA_CONTEXT_STACK_TRAITS_H

#include <cstddef>

#include <boost/config.hpp>

#include <memoria/context/detail/config.hpp>


namespace memoria {
namespace context {

struct MEMORIA_CONTEXT_DECL stack_traits
{
    static bool is_unbounded() noexcept;

    static std::size_t page_size() noexcept;

    static std::size_t default_size() noexcept;

    static std::size_t minimum_size() noexcept;

    static std::size_t maximum_size() noexcept;
};

}}


#endif // MEMORIA_CONTEXT_STACK_TRAITS_H
