
// Copyright 2018 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <memoria/core/types.hpp>

#if defined(MMA_WINDOWS)
#include <boost/winapi/handles.hpp>
#endif


namespace memoria {
namespace reactor {

#ifdef MMA_POSIX

using IOHandle = int32_t;
constexpr IOHandle INVALID_IO_HANDLE = -1;

#elif defined(MMA_WINDOWS)

using IOHandle = boost::winapi::HANDLE_;
const IOHandle INVALID_IO_HANDLE = boost::winapi::INVALID_HANDLE_VALUE_;

#endif


}}
