
// Copyright 2017 Victor Smirnov
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

#include <memoria/core/config.hpp>

#ifdef MMA1_WINDOWS
#include "msvc/msvc_file.hpp"
#elif defined(MMA1_MACOSX)
#include "macosx/macosx_file.hpp"
#elif defined(MMA1_LINUX)
#include "linux/linux_file.hpp"
#endif

#include <memoria/core/tools/pimpl_base.hpp>
#include <memoria/core/tools/iostreams.hpp>


namespace memoria {
namespace reactor {

class BufferedFileImpl;
class DMAFileImpl;

class File: public PimplBase<BufferedFileImpl> {
    using Base = PimplBase<BufferedFileImpl>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(File)

    void close();
    bool is_closed() const;

    uint64_t size();

    uint64_t fpos();
    uint64_t seek(uint64_t pos);

    size_t read(uint8_t* buffer, size_t size);
    size_t write(const uint8_t* buffer, size_t size);

    size_t read(uint8_t* buffer, uint64_t offset, size_t size);
    size_t write(const uint8_t* buffer, uint64_t offset, size_t size);

    void fsync();
    void fdsync();

    BinaryInputStream istream();
    BinaryOutputStream ostream();

    const filesystem::path& path();
};


File open_buffered_file(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);


class DMAFile: public PimplBase<DMAFileImpl> {
    using Base = PimplBase<DMAFileImpl>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(DMAFile)

    void close();
    bool is_closed() const;

    uint64_t alignment();
    uint64_t size();

    size_t read(uint8_t* buffer, uint64_t offset, size_t size);
    size_t write(const uint8_t* buffer, uint64_t offset, size_t size);

    size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true);

    const filesystem::path& path();
};


DMAFile open_dma_file(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);




}}

