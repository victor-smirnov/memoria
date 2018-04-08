
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

#include "file_tests.hpp"

#include <algorithm>

namespace memoria {
namespace v1 {
namespace tests {

using namespace memoria::v1::reactor;

struct FileBlockTestState: TestState {
    using Base = TestState;

    size_t buffer_size{4096};

    uint64_t file_size; // In Kbytes

    virtual void post_configure(TestCoverage coverage)
    {
        file_size = select_for_coverage<uint64_t>(
            coverage,
            100,
            1000,
            10000,
            100000
        ) * 1024;
    }
};

using FileChunk = std::pair<uint64_t, uint64_t>;



auto file_block_test = register_test_in_suite<FnTest<FileBlockTestState>>(u"ReactorSuite", u"FileBlockTest", [](auto& state){

    auto wd = state.working_directory_;
    wd.append("file.bin");

    File file = open_buffered_file(wd, FileFlags::RDWR | FileFlags::CREATE | FileFlags::TRUNCATE);

    auto buffer = allocate_system<uint8_t>(state.buffer_size);

    uint8_t stream_state{};
    uint64_t total_written{};

    engine().coutln(u"Prepare file of size: {}", state.file_size);

    while (total_written < state.file_size)
    {
        size_t write_size = getRandomG(state.buffer_size - 1) + 1;

        for (size_t c = 0; c < write_size; c++) {
            buffer.get()[c] = stream_state++;
        }

        size_t written = file.write(buffer.get(), write_size);
        total_written += written;
    }

    assert_equals(total_written, file.fpos());
    assert_equals(0, file.seek(0));

    engine().coutln(u"Read file in chunks", "");

    for (auto& chunk: create_random_chunks_vector<>(state.buffer_size, total_written))
    {
        uint64_t chunk_pos;
        uint64_t chunk_size;
        std::tie(chunk_pos, chunk_size) = chunk;

        uint8_t read_stream_state = static_cast<uint8_t>(chunk_pos % 256);

        size_t read = file.read(buffer.get(), chunk_pos, chunk_size);
        assert_equals(chunk_pos + read, file.fpos());
        assert_equals(read, chunk_size);

        for (size_t c = 0; c < read; c++)
        {
            assert_equals((int)read_stream_state, (int)buffer.get()[c]);
            read_stream_state++;
        }
    }

	
    engine().coutln(u"Rewrite file in chunks", "");

    for (auto& chunk: create_random_chunks_vector<>(state.buffer_size, total_written))
    {
        uint64_t chunk_pos;
        uint64_t chunk_size;
        std::tie(chunk_pos, chunk_size) = chunk;

        uint8_t write_stream_state = static_cast<uint8_t>((chunk_pos - total_written + 1) % 256);

        for (size_t c = 0; c < chunk_size; c++){
            buffer.get()[c] = write_stream_state++;
        }

        size_t written = file.write(buffer.get(), chunk_pos, chunk_size);
        assert_equals(chunk_pos + written, file.fpos());
        assert_equals(written, chunk_size);
    }

    engine().coutln(u"Read file in chunks again", "");

    for (auto& chunk: create_random_chunks_vector<>(state.buffer_size, total_written))
    {
        uint64_t chunk_pos;
        uint64_t chunk_size;
        std::tie(chunk_pos, chunk_size) = chunk;

        uint8_t read_stream_state = static_cast<uint8_t>((chunk_pos - total_written + 1) % 256);

        size_t read = file.read(buffer.get(), chunk_pos, chunk_size);
        assert_equals(chunk_pos + read, file.fpos());
        assert_equals(read, chunk_size);

        for (size_t c = 0; c < read; c++)
        {
            assert_equals((int)read_stream_state, (int)buffer.get()[c]);
            read_stream_state++;
        }
    }

    file.close();
});



}}}
