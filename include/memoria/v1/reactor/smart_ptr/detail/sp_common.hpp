
//  detail/sp_common.hpp
//
//  Copyright 2018 Victor Smirnov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <memoria/v1/core/types.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

namespace detail {

template <typename Fn>
void run_at_engine(int32_t cpu, Fn&& fn);

template<typename T = void>
int32_t engine_current_cpu();

template<typename T = void>
int32_t engine_cpu_num();

}

}}}
