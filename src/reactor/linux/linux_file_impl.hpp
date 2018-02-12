
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

#include <memoria/v1/reactor/file.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

class FileImplBase {
protected:
    filesystem::path path_;
public:
    FileImplBase (const filesystem::path& file_path): path_(file_path) {}
    virtual ~FileImplBase() noexcept {}

    const filesystem::path& path() const {return path_;}
};




class BufferedFileImpl: public FileImplBase, public IBinaryIOStream, public std::enable_shared_from_this<BufferedFileImpl> {
    int fd_{};
    bool closed_{true};
public:
    BufferedFileImpl(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);
    virtual ~BufferedFileImpl() noexcept;


    virtual void close();

    virtual bool is_closed() const {return closed_;}

    virtual uint64_t size() {
        return filesystem::file_size(path_);
    }

    virtual uint64_t seek(uint64_t position);

    virtual size_t read(uint8_t* buffer, size_t size);
    virtual size_t write(const uint8_t* buffer, size_t size);

    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size);
    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size);

    virtual void fsync();

    virtual void fdsync();

    virtual void flush() {}

    virtual BinaryInputStream istream()
    {
        return std::static_pointer_cast<IBinaryInputStream>(shared_from_this());
    }

    virtual BinaryOutputStream ostream()
    {
        return std::static_pointer_cast<IBinaryOutputStream>(shared_from_this());
    }
};




class DMAFileImpl: public FileImplBase, public std::enable_shared_from_this<DMAFileImpl> {
    int fd_{};
    bool closed_{true};
public:
    DMAFileImpl(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);

    virtual ~DMAFileImpl() noexcept;

    virtual uint64_t alignment() {return 512;}

    virtual void close();

    virtual uint64_t size() {
        return filesystem::file_size(path_);
    }


    virtual bool is_closed() const {return closed_;}

    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size);

    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size);

    virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error);


private:
    uint64_t process_single_io(uint8_t* buffer, uint64_t offset, uint64_t size, int command, const char* opname);
};




}}}
