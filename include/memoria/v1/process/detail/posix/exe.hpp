// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace boost
{
namespace process
{
namespace detail
{
namespace posix
{

template<class StringType, class Executor>
inline void apply_exe(const StringType & exe, Executor & e)
{
    e.exe = exe.c_str();
}

}



}
}
}

