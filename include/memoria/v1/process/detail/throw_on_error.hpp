// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/process/detail/config.hpp>
#include <memoria/v1/process/detail/handler_base.hpp>

namespace boost { namespace process { namespace detail {

struct throw_on_error_ : ::boost::process::detail::handler
{
    template <class Executor>
    void on_error(Executor& exec, const std::error_code & ec) const
    {
        throw process_error(ec, "process creation failed");
    }

    const throw_on_error_ &operator()() const {return *this;}
};

}

constexpr boost::process::detail::throw_on_error_ throw_on_error;

}}

