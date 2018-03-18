
#include <memoria/v1/reactor/console_streams.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <unistd.h>

namespace memoria {
namespace v1 {
namespace reactor {

size_t ConsoleInputStream::read(uint8_t* data, size_t size)
{
    ssize_t result{};
    int32_t errno_v{};

    std::tie(result, errno_v) = engine().run_in_thread_pool([&]{
        ssize_t result0 = ::read(fd_, data, size);
        return std::make_tuple(result0, (int32_t)errno);
    });

    if (result < 0)
    {
        MMA1_THROW(SystemException()) << WhatInfo(strerror(errno_v));
    }

    return result;
}

size_t ConsoleOutputStream::write(const uint8_t* data, size_t size)
{
    size_t result{};

    if (size > 0 && !ext_closed_)
    {
        int32_t errno_v{};
        bool error_v{};

        std::tie(result, errno_v, error_v) = engine().run_in_thread_pool([&]{
            size_t total{};

            while (total < size)
            {
                ssize_t result0 = ::write(fd_, data + total, size - total);
                if (result0 < 0)
                {
                    return std::make_tuple(total, (int32_t)errno, true);
                }

                total += result0;
            }

            return std::make_tuple(total, (int32_t)errno, false);
        });

        if (error_v)
        {
            MMA1_THROW(SystemException()) << WhatInfo(strerror(errno_v));
        }
    }

    return result;
}

void ConsoleOutputStream::flush() {

}






}}}
