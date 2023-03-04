
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

#include <memoria/reactor/pipe_streams.hpp>

#include <boost/fiber/all.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <vector>

namespace memoria {
namespace reactor {

class InputStreamReader {

	using Buffer = std::vector<uint8_t>;

	PipeInputStream pipe_in_;
	Buffer buffer_;

    boost::fibers::fiber reader_;

public:
	InputStreamReader(PipeInputStream pipe_in): 
		pipe_in_(pipe_in)
	{
        reader_ = boost::fibers::fiber([&] {
			do_read_data();
		});
	}

	Buffer& buffer() { return buffer_; }
	const Buffer& buffer() const { return buffer_; }

	void join() {
		reader_.join();
	}

    U8String to_u8() const {
        return U8String(ptr_cast<const char>(buffer_.data()), buffer_.size());
    }

	U16String to_u16() const {
		return U16String(ptr_cast<const char>(buffer_.data()), buffer_.size());
	}

private:
	void do_read_data() 
	{
		uint8_t buf[256];
		while (!pipe_in_.is_closed())
		{
			size_t read = pipe_in_.read(buf, sizeof(buf));
			buffer_.insert(buffer_.end(), buf, buf + read);
		}
	}
};


class InputStreamReaderWriter {

    PipeInputStream pipe_in_;
    BinaryOutputStream output_;

    boost::fibers::fiber reader_;

public:
    InputStreamReaderWriter(PipeInputStream pipe_in, BinaryOutputStream output):
        pipe_in_(pipe_in),
        output_(output)
    {
        reader_ = boost::fibers::fiber([&] {
            do_read_data();
        });
    }

    void join() {
        reader_.join();
    }
private:
    void do_read_data()
    {
        uint8_t buf[1024];
        while (!pipe_in_.is_closed())
        {
            size_t read = pipe_in_.read(buf, sizeof(buf));
            if (read > 0)
            {
                size_t written = output_.write(buf, read);
                if (written < read) {
                    break;
                }
            }
            else {
                break;
            }
        }
    }
};




}}
