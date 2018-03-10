
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

namespace memoria {
namespace v1 {
namespace tests {

using TestDuration = std::chrono::duration<double>;

template <typename InputStream, typename OutputStream>
class StreamTester {
    InputStream sender_istream_;
    OutputStream sender_ostream_;

    InputStream receiver_istream_;
    OutputStream receiver_ostream_;

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
        InputStream sender_istream, OutputStream sender_ostream,
        InputStream receiver_istream, OutputStream receiver_ostream,
        TestDuration test_length,
        size_t block_size_min,
        size_t block_size_max
    ):
        sender_istream_(sender_istream), sender_ostream_(sender_ostream),
        receiver_istream_(receiver_istream), receiver_ostream_(receiver_ostream),
        test_length_(test_length),
        block_size_min_(block_size_min),
        block_size_max_(block_size_max)
    {}

    void start()
    {
        sender_ = fibers::fiber([&]{send();});
        receiver_ = fibers::fiber([&]{receive();});
        looper_ = fibers::fiber([&]{loop();});
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

private:

    void send()
    {
        auto buffer = allocate_system<uint8_t>(block_size_max_);
        uint8_t* buffer_ptr = buffer.get();

        auto start = std::chrono::system_clock::now();

        uint8_t stream_state_{};

        while ((std::chrono::system_clock::now() - start) < test_length_)
        {
            size_t block_size = getRandomG(block_size_max_ - block_size_min_) + block_size_min_;

            for (size_t c = 0; c < block_size; c++)
            {
                buffer_ptr[c] = stream_state_++;
            }

            size_t written = sender_ostream_.write(buffer_ptr, block_size);
            assert_equals(written, block_size, "Stream write size");
            total_sent_ += written;
        }

        sender_ostream_.close();
    }

    void loop()
    {
        auto buffer = allocate_system<uint8_t>(block_size_max_);
        uint8_t* buffer_ptr = buffer.get();

        uint8_t stream_state_{};

        while (!sender_istream_.is_closed())
        {
            size_t block_size = getRandomG(block_size_max_ - block_size_min_) + block_size_min_;

            size_t read = sender_istream_.read(buffer_ptr, block_size);

            for (size_t c = 0; c < read; c++, stream_state_++)
            {
                assert_equals(buffer_ptr[c], stream_state_, "Looper stream state");
            }

            size_t written = receiver_ostream_.write(buffer_ptr, read);
            assert_equals(written, read, "Looper write size");
            total_transferred_ += written;
        }

        receiver_ostream_.close();
    }

    void receive()
    {
        auto buffer = allocate_system<uint8_t>(block_size_max_);
        uint8_t* buffer_ptr = buffer.get();

        uint8_t stream_state_{};

        while (!receiver_istream_.is_closed())
        {
            size_t block_size = getRandomG(block_size_max_ - block_size_min_) + block_size_min_;

            size_t read = receiver_istream_.read(buffer_ptr, block_size);

            for (size_t c = 0; c < read; c++, stream_state_++)
            {
                assert_equals(buffer_ptr[c], stream_state_, "Receiver stream state");
            }

            total_received_ += read;
        }
    }
};


}}}
