
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

#include <memoria/tests/tests.hpp>


#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/file.hpp>
#include <memoria/reactor/file_streams.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/random.hpp>

#include <algorithm>
#include <random>

namespace memoria {
namespace tests {


using FileChunk = std::pair<uint64_t, uint64_t>;

template <typename ChunkT = FileChunk>
std::vector<ChunkT> create_random_chunks_vector(size_t max_chunk_size, uint64_t file_size)
{
    std::vector<ChunkT> chunks;

    for (uint64_t pos = 0; pos < file_size;)
    {
        size_t chunk_size = getRandomG(max_chunk_size - 1) + 1;

        if (pos + chunk_size >= file_size) {
            chunk_size = file_size - pos;
        }

        chunks.emplace_back(pos, chunk_size);

        pos += chunk_size;
    }

    std::random_shuffle(chunks.begin(), chunks.end(), getGlobalInt64Generator());

    return chunks;
}



template <typename ChunkT = FileChunk>
std::vector<ChunkT> create_fixed_chunks_vector(size_t chunk_size, uint64_t file_size)
{
    std::vector<ChunkT> chunks;

    for (uint64_t pos = 0; pos < file_size;)
    {
        if (pos + chunk_size >= file_size) {
            chunk_size = file_size - pos;
        }

        chunks.emplace_back(pos, chunk_size);

        pos += chunk_size;
    }

    std::random_shuffle(chunks.begin(), chunks.end(), getGlobalInt64Generator());

    return chunks;
}


}}
