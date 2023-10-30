
// Copyright 2017 Victor Smirnov
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

#include <memoria/core/strings/string_buffer.hpp>
#include <memoria/core/strings/format.hpp>

#include <cstring>
#include <cstdio>
#include <exception>
#include <iostream>


namespace memoria {
namespace tools {

static inline void report_perror(const SBuf& message)
{
    SBuf buf;
    
    buf << message.str() << ": " << std::strerror(errno);

    std::cout << "Exception: " << buf.str() << " -- errno: " << errno << std::endl;
}

static inline void report_perror(int errno0, const SBuf& message) 
{
    SBuf buf;
    
    buf << message.str() << ": " << std::strerror(errno0);

    std::cout << "Exception: " << buf.str() << " -- errno: " << errno0 << std::endl;
}


}}
