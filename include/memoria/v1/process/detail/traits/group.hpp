// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <memoria/v1/process/detail/config.hpp>
#include <memoria/v1/process/detail/traits/decl.hpp>



namespace boost { namespace process {

struct group;

namespace detail {


struct group_tag {};

template<>
struct make_initializer_t<group_tag>;


template<> struct initializer_tag_t<::boost::process::group> { typedef group_tag type;};




}}}
