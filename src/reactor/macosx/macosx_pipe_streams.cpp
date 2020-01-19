
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

#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/pipe_streams.hpp>
#include <memoria/core/memory/smart_ptrs.hpp>

#include "macosx_io_messages.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace memoria {
namespace reactor {

class PipeInputStreamImpl: public IPipeInputStream {
    IOHandle fd_;

    SocketIOMessage fiber_io_read_message_;

    bool op_closed_{false};
    bool data_closed_{false};

public:
    PipeInputStreamImpl(IOHandle handle):
        fd_(handle),
        fiber_io_read_message_(engine().cpu())
    {
        int flags = fcntl(fd_, F_GETFL, 0);
        if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA1_THROW(SystemException()) << WhatCInfo("Can't set pipe property O_NONBLOCK");
        }

        int queue_fd = engine().io_poller().queue_fd();

        KEvent64(queue_fd, fd_, EVFILT_READ, EV_ADD | EV_ERROR, &fiber_io_read_message_);
    }

    virtual ~PipeInputStreamImpl() noexcept {
        close();
    }

    virtual IOHandle handle() const {return fd_;}

    size_t read(uint8_t* data, size_t size)
    {
        size_t available_size = size;

        while (!data_closed_)
        {
            ssize_t result = ::read(fd_, data, available_size);

            if (result >= 0) {
                data_closed_ = result == 0;
                return result;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // FIXME: handle eof flag properly

                fiber_io_read_message_.wait_for();

                size_t av = fiber_io_read_message_.available();
                available_size = av >= size ? size : av;
            }
            else {
                MMA1_THROW(SystemException()) << format_ex(
                    "Error reading from pipe {}", fd_
                );
            }
        }

        return 0;
    }

    virtual IOHandle detach()
    {
        if (!op_closed_)
        {
            op_closed_ = true;

            int queue_fd = engine().io_poller().queue_fd();

            KEvent64(queue_fd, fd_, EVFILT_READ, EV_DELETE, nullptr);
            return fd_;
        }

        return -1;
    }

    virtual void close()
    {
        if (!op_closed_)
        {
            op_closed_ = true;

            int queue_fd = engine().io_poller().queue_fd();

            KEvent64(queue_fd, fd_, EVFILT_READ, EV_DELETE, nullptr);

            if (::close(fd_) < 0)
            {
                MMA1_THROW(SystemException()) << format_ex("Can't close pipe {}", fd_);
            }
        }
    }

    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
    }
};

class PipeOutputStreamImpl: public IPipeOutputStream {
    IOHandle fd_;

    SocketIOMessage fiber_io_write_message_;

    bool op_closed_{false};
    bool data_closed_{false};

public:
    PipeOutputStreamImpl(IOHandle handle):
        fd_(handle),
        fiber_io_write_message_(engine().cpu())
    {
        int flags = fcntl(fd_, F_GETFL, 0);
        if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA1_THROW(SystemException()) << WhatInfo("Can't set pipe property O_NONBLOCK");
        }

        int queue_fd = engine().io_poller().queue_fd();

        KEvent64(queue_fd, fd_, EVFILT_WRITE, EV_ADD | EV_ERROR, &fiber_io_write_message_);
    }

    virtual ~PipeOutputStreamImpl() noexcept {
        close();
    }

    virtual IOHandle handle() const {return fd_;}

    virtual size_t write(const uint8_t* data, size_t size)
    {
        size_t total{};
        while (total < size)
        {
            size_t written = write_(data + total, size - total);

            if (written > 0) {
                total += written;
            }
            else {
                break;
            }
        }
        return total;
    }

    size_t write_(const uint8_t* data, size_t size)
    {
        size_t available_size = size;
        while (!data_closed_)
        {
            ssize_t result = ::write(fd_, data, available_size);

            if (result >= 0) {
                data_closed_ = result == 0;
                return result;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // FIXME: handle eof flag properly
                fiber_io_write_message_.wait_for();

                size_t av = fiber_io_write_message_.available();
                available_size = av >= size ? size : av;
            }
            else {
                MMA1_THROW(SystemException()) << format_ex(
                    "Error writing to pipe {}", fd_
                );
            }
        }

        return 0;
    }

    virtual IOHandle detach()
    {
        if (!op_closed_)
        {
            op_closed_ = true;

            int queue_fd = engine().io_poller().queue_fd();

            KEvent64(queue_fd, fd_, EVFILT_WRITE, EV_DELETE, nullptr);
            return fd_;
        }

        return -1;
    }

    virtual void close()
    {
        if (!op_closed_)
        {
            op_closed_ = true;

            int queue_fd = engine().io_poller().queue_fd();

            KEvent64(queue_fd, fd_, EVFILT_WRITE, EV_DELETE);

            if (::close(fd_) < 0)
            {
                MMA1_THROW(SystemException()) << format_ex("Can't close pipe {}", fd_);
            }
        }
    }

    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
    }

    virtual void flush() {}
};


PipeStreams open_pipe()
{
    int32_t fds[2];
    if (::pipe(fds) < 0) {
        MMA1_THROW(SystemException()) << WhatCInfo("Can't create a pair of pipes");
    }

    return PipeStreams{
        PipeInputStream(MakeLocalShared<PipeInputStreamImpl>(fds[0])),
        PipeOutputStream(MakeLocalShared<PipeOutputStreamImpl>(fds[1])),
    };
}

PipeInputStream open_input_pipe(const char* name) {
    return PipeInputStream();
}

PipeOutputStream open_output_pipe(const char* name){
    return PipeOutputStream();
}


}}
