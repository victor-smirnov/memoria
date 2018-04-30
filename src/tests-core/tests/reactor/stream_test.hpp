
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

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/reactor/reactor.hpp>

#include <memoria/v1/fiber/fiber.hpp>

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace memoria {
namespace v1 {
namespace tests {

using TestDuration = std::chrono::duration<double>;

template <typename T>
using Fn = std::function<T()>;

template <typename InputStream, typename OutputStream>
class StreamTester {
    Fn<OutputStream> sender_ostream_fn_;

    Fn<InputStream> looper_istream_fn_;
    Fn<OutputStream> looper_ostream_fn_;

    Fn<InputStream> receiver_istream_fn_;

    TestDuration test_length_;
    size_t block_size_min_;
    size_t block_size_max_;

    fibers::fiber sender_;
    fibers::fiber receiver_;
    fibers::fiber looper_;

    uint64_t total_sent_{};
    uint64_t total_received_{};
    uint64_t total_transferred_{};

public:
    StreamTester(
        Fn<OutputStream> sender_ostream,
        Fn<InputStream> looper_istream,
        Fn<OutputStream> looper_ostream,
        Fn<InputStream> receiver_istream,

        TestDuration test_length,
        size_t block_size_min,
        size_t block_size_max
    ):
        sender_ostream_fn_(std::move(sender_ostream)),
        looper_istream_fn_(std::move(looper_istream)),
        looper_ostream_fn_(std::move(looper_ostream)),
        receiver_istream_fn_(std::move(receiver_istream)),
        test_length_(test_length),
        block_size_min_(block_size_min),
        block_size_max_(block_size_max)
    {}

    void start()
    {
        sender_ = fibers::fiber([&]{
            try {
                send();
            }
            catch (MemoriaThrowable& th) {
                th.dump(reactor::engine().cout());
            }
        });

        receiver_ = fibers::fiber([&]{
            try {
                receive();
            }
            catch (MemoriaThrowable& th) {
                th.dump(reactor::engine().cout());
            }
        });

        looper_ = fibers::fiber([&]{
            try {
                loop();
            }
            catch (MemoriaThrowable& th) {
                th.dump(reactor::engine().cout());
            }
        });
    }

    void join()
    {
        sender_.join();
        looper_.join();
        receiver_.join();
    }

    uint64_t total_sent() {return total_sent_;}
    uint64_t total_received() {return total_received_;}
    uint64_t total_transferred() {return total_received_;}

    bool send_is_done_{false};
    bool resend_is_done_{false};

private:

    void send()
    {
        auto buffer = allocate_system<uint8_t>(block_size_max_);
        uint8_t* buffer_ptr = buffer.get();

        auto sender_ostream = sender_ostream_fn_();

        uint8_t stream_state_{};

        auto start = std::chrono::system_clock::now();
        while ((std::chrono::system_clock::now() - start) < test_length_)
        //for (int c = 0; c < 10; c++)
        {
            size_t block_size = getRandomG(block_size_max_ - block_size_min_) + block_size_min_;

            for (size_t c = 0; c < block_size; c++)
            {
                buffer_ptr[c] = stream_state_++;
            }

            size_t written = sender_ostream.write(buffer_ptr, block_size);
            assert_equals(block_size, written, "Sender stream write size");
            total_sent_ += written;
        }

        send_is_done_ = true;
        sender_ostream.close();
    }

    void loop()
    {
        auto buffer = allocate_system<uint8_t>(block_size_max_);
        uint8_t* buffer_ptr = buffer.get();

        auto looper_ostream = looper_ostream_fn_();
        auto looper_istream = looper_istream_fn_();

        uint8_t stream_state_{};

        uint64_t loop_received{};

        //while (!send_is_done_ || ((loop_received < total_sent_) && total_transferred_ < total_sent_))
        while(!looper_istream.is_closed())
        {
            size_t block_size = getRandomG(block_size_max_ - block_size_min_) + block_size_min_;

            size_t read = looper_istream.read(buffer_ptr, block_size);
            loop_received += read;

            for (size_t c = 0; c < read; c++, stream_state_++)
            {
                assert_equals(stream_state_, buffer_ptr[c], "Looper stream state");
            }

            size_t written = looper_ostream.write(buffer_ptr, read);
            assert_equals(read, written, "Looper write size");
            total_transferred_ += written;
        }

        resend_is_done_ = true;
        looper_ostream.close();
    }

    void receive()
    {
        auto buffer = allocate_system<uint8_t>(block_size_max_);
        uint8_t* buffer_ptr = buffer.get();

        auto receiver_istream = receiver_istream_fn_();

        uint8_t stream_state_{};

        //while (!resend_is_done_ || (total_received_ < total_sent_))
        while (!receiver_istream.is_closed())
        {
            size_t block_size = getRandomG(block_size_max_ - block_size_min_) + block_size_min_;

            size_t read = receiver_istream.read(buffer_ptr, block_size);

            for (size_t c = 0; c < read; c++, stream_state_++)
            {
                assert_equals(stream_state_, buffer_ptr[c], "Receiver stream state");
            }

            total_received_ += read;
        }
    }
};


}}}
