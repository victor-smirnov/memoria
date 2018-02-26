// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/process/detail/config.hpp>
#include <memoria/v1/process/detail/posix/group_handle.hpp>
#include <memoria/v1/process/detail/posix/handler.hpp>
#include <unistd.h>


namespace boost { namespace process {

namespace detail { namespace posix {



struct group_ref : handler_base_ext
{
    group_handle & grp;


    explicit group_ref(group_handle & g) :
                grp(g)
    {}

    template <class Executor>
    void on_exec_setup(Executor&) const
    {
        if (grp.grp == -1)
            ::setpgid(0, 0);
        else
            ::setpgid(0, grp.grp);
    }

    template <class Executor>
    void on_success(Executor& exec) const
    {
        if (grp.grp == -1)
            grp.grp = exec.pid;

    }

};

}}}}
