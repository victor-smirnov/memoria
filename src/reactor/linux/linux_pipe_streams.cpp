
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

#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/pipe_streams.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>

#include "linux_io_messages.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace memoria {
namespace v1 {
namespace reactor {

class PipeInputStreamImpl: public IPipeInputStream {
    IOHandle handle_;

    SocketIOMessage fiber_io_message_;

    bool op_closed_{false};
    bool data_closed_{false};

public:
    PipeInputStreamImpl(IOHandle handle):
        handle_(handle),
        fiber_io_message_(engine().cpu(), "::input")
    {
        int flags = fcntl(handle_, F_GETFL, 0);
        if (fcntl(handle_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA1_THROW(SystemException()) << WhatCInfo("Can't set pipe property O_NONBLOCK");
        }

        epoll_event event = tools::make_zeroed<epoll_event>();

        event.data.ptr = &fiber_io_message_;

        event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

        int sres = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, handle_, &event);
        if (sres < 0) {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't configure poller for {}", handle_);
        }
    }

    virtual IOHandle hande() const {return handle_;}

    virtual size_t read(uint8_t* data, size_t size)
    {
        size_t total{};

        while (total < size) {
            total += read_(data + total, size - total);
        }

        return total;
    }

    size_t total_r{};

    size_t read_(uint8_t* data, size_t size)
    {
        while (true)
        {
            ssize_t result = ::read(handle_, data, size);

            if (result >= 0) {
                data_closed_ = result == 0;

                total_r += result;

                std::cout << "read: " << result << " " << total_r << std::endl;
                return result;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "Wait for read: " << size << " " << total_r << " " << &fiber_io_message_ << std::endl;
                fiber_io_message_.wait_for();
            }
            else {
                MMA1_THROW(SystemException()) << fmt::format_ex(u"Error reading from pipe for {}", handle_);
            }
        }
    }

    virtual void close()
    {
        if (!op_closed_)
        {
            op_closed_ = true;
            int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, handle_, nullptr);
            if (res < 0)
            {
                int32_t err_code = errno;
                ::close(handle_);
                MMA1_THROW(SystemException(err_code)) << fmt::format_ex(u"Can't remove epoller for pipe {}", handle_);
            }

            engine().drain_pending_io_events(&fiber_io_message_);

            if (::close(handle_) < 0)
            {
                MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't close pipe {}", handle_);
            }
        }
    }

    virtual bool is_closed() const {
        return op_closed_ || fiber_io_message_.connection_closed() || data_closed_;
    }
};

class PipeOutputStreamImpl: public IPipeOutputStream {
    IOHandle handle_;

    SocketIOMessage fiber_io_message_;

    bool op_closed_{false};
    bool data_closed_{false};

public:
    PipeOutputStreamImpl(IOHandle handle):
        handle_(handle),
        fiber_io_message_(engine().cpu(), "::output")
    {
        int flags = fcntl(handle_, F_GETFL, 0);
        if (fcntl(handle_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA1_THROW(SystemException()) << WhatInfo("Can't set pipe property O_NONBLOCK");
        }

        epoll_event event = tools::make_zeroed<epoll_event>();

        event.data.ptr = &fiber_io_message_;

        event.events = EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

        int sres = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, handle_, &event);
        if (sres < 0) {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't configure poller for {}", handle_);
        }
    }

    virtual IOHandle hande() const {return handle_;}

    virtual size_t write(const uint8_t* data, size_t size)
    {
        size_t total{};
        while (total < size) {
            total += write_(data + total, size - total);
        }
        return total;
    }

    size_t total_w{};

    size_t write_(const uint8_t* data, size_t size)
    {
        while (true)
        {
            ssize_t result = ::write(handle_, data, size);

            if (result >= 0)
            {
                data_closed_ = result == 0;
                total_w += result;
                std::cout << "written: " << result << " " << total_w << std::endl;
                return result;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "Wait for write: " << size << " " << total_w << " " << &fiber_io_message_ << std::endl;
                fiber_io_message_.wait_for();
            }
            else {
                MMA1_THROW(SystemException()) << fmt::format_ex(u"Error writing pipe for {}", handle_);
            }
        }
    }

    virtual void close()
    {
        if (!op_closed_)
        {
            op_closed_ = true;
            int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, handle_, nullptr);
            if (res < 0)
            {
                int32_t err_code = errno;
                ::close(handle_);
                MMA1_THROW(SystemException(err_code)) << fmt::format_ex(u"Can't remove epoller for pipe {}", handle_);
            }

            engine().drain_pending_io_events(&fiber_io_message_);

            if (::close(handle_) < 0)
            {
                MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't close pipe {}", handle_);
            }
        }
    }

    virtual bool is_closed() const {
        return op_closed_ || fiber_io_message_.connection_closed() || data_closed_;
    }

    virtual void flush() {}
};


PipeStreams open_pipe()
{
    int32_t fds[2];
    if (::pipe2(fds, 0) < 0) {
        MMA1_THROW(SystemException()) << WhatCInfo("Can't create a pair of pipes");
    }

    return PipeStreams{
        PipeInputStream(MakeLocalShared<PipeInputStreamImpl>(fds[0])),
        PipeOutputStream(MakeLocalShared<PipeOutputStreamImpl>(fds[1])),
    };
}

PipeInputStream open_input_pipe(const char16_t* name) {
    return PipeInputStream();
}

PipeOutputStream open_output_pipe(const char16_t* name){
    return PipeOutputStream();
}


}}}
