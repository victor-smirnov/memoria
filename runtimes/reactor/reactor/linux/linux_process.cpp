
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

namespace memoria {
namespace reactor {

namespace {

size_t find_zero(const uint8_t* mem, size_t max)
{
    size_t c;
    for (c = 0; c < max; c++)
    {
        if (mem[c] == 0) {
            break;
        }
    }
    return c;
}

}

boost::filesystem::path get_program_path()
{
    const char* link_path = "/proc/self/cmdline";

    size_t buf_size = 1024;
    auto buf = allocate_system_zeroed<uint8_t>(buf_size);
    auto file = open_buffered_file(link_path, FileFlags::RDONLY);

    U8String str;

    while (true)
    {
        auto read = file.read(buf.get(), buf_size - 1);
        *(buf.get() + read) = 0;

        size_t zero_pos = find_zero(buf.get(), read);

        str += U8String(ptr_cast<char>(buf.get()), zero_pos);

        if (zero_pos < read)
        {
            break;
        }
    }

    file.close();

    return str.to_std_string();
}

}}
