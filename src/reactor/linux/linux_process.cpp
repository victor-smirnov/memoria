
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/reactor/process.hpp>
#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

namespace memoria {
namespace v1 {
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

filesystem::path get_program_path()
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

        str += U8String(tools::ptr_cast<char>(buf.get()), zero_pos);

        if (zero_pos < read)
        {
            break;
        }
    }

    file.close();

    return str;
}

}}}
