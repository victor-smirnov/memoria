
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

#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/file.hpp>
#include <memoria/v1/reactor/file_streams.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>



namespace memoria {
namespace v1 {
namespace tests {

using namespace memoria::v1::reactor;

struct FileStreamTestState: TestState {
    using Base = TestState;

    size_t buffer_size{4096};

    uint64_t stream_size; // In Kbytes

    virtual void post_configure(TestCoverage coverage)
    {
        stream_size = select_for_coverage<uint64_t>(
            coverage,
            100,
            1000,
            10000,
            100000,
            1000000,
            10000000
        ) * 1024;
    }
};


auto file_stream_test = register_test_in_suite<FnTest<FileStreamTestState>>(u"ReactorSuite", u"FileStreamTest", [](auto& state){

	auto wd = state.working_directory_;
    wd.append("file.bin");

    File file = open_buffered_file(wd, FileFlags::RDWR | FileFlags::CREATE | FileFlags::TRUNCATE);

    auto ostream = file.ostream();
    auto istream = file.istream();

    auto buffer = allocate_system<uint8_t>(state.buffer_size);

    uint8_t stream_state{};
    uint64_t total_written{};
	uint64_t total_write_requested{};

    while (total_written < state.stream_size)
    {
        size_t write_size = getRandomG(state.buffer_size - 1) + 1;
		total_write_requested += write_size;

        for (size_t c = 0; c < write_size; c++) {
            buffer.get()[c] = stream_state++;
        }

        size_t written = ostream.write(buffer.get(), write_size);
		total_written += written;
    }

	assert_equals(total_written, total_write_requested);

    assert_equals(total_written, file.fpos());
    assert_equals(0, file.seek(0));

    uint64_t total_read{};
    stream_state = 0;

    while (total_read < total_written)
    {
        size_t read_size = getRandomG(state.buffer_size - 1) + 1;
        size_t read = istream.read(buffer.get(), read_size);

        if (read == 0) {
            break;
        }

        for (size_t c = 0; c < read; c++)
        {
           assert_equals((int)stream_state, (int)buffer.get()[c]);
           stream_state++;
        }

        total_read += read;
    }

    assert_equals(total_written, total_read, "Check read size");
    assert_equals(total_read, file.fpos(), "Check file position after read");

    file.close();
});



}}}
