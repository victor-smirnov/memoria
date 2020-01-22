
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



#include <memoria/core/memory/malloc.hpp>

#include <memoria/reactor/process.hpp>
#include <memoria/reactor/reactor.hpp>

#include <memoria/core/strings/string.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include <mach-o/dyld.h>

namespace memoria {
namespace reactor {


filesystem::path get_program_path()
{
    uint32_t bufsize{};
    _NSGetExecutablePath(nullptr, &bufsize);
    auto path_buf = allocate_system_zeroed<char>(bufsize + 1);
    _NSGetExecutablePath(path_buf.get(), &bufsize);

    return filesystem::path(U8String(path_buf.get(), bufsize));
}

}}
