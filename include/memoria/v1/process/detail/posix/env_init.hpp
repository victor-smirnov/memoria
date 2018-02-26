// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#pragma once


#include <memoria/v1/process/detail/config.hpp>
#include <memoria/v1/process/detail/posix/handler.hpp>
#include <memoria/v1/process/environment.hpp>

namespace boost { namespace process { namespace detail { namespace posix {

template<typename Char>
struct env_init;

template<>
struct env_init<char> : handler_base_ext
{
    boost::process::environment env;

    env_init(boost::process::environment && env) : env(std::move(env)) {};
    env_init(const boost::process::environment & env) : env(env) {};


    template <class Executor>
    void on_setup(Executor &exec) const
    {
        exec.env = env._env_impl;
    }

};

}}}}
