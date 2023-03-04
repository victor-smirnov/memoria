
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

#include <memoria/reactor/file.hpp>

#include <memoria/reactor/reactor.hpp>

#include <boost/filesystem.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace memoria {
namespace reactor {

class FileImplBase {
protected:
    boost::filesystem::path path_;
public:
    FileImplBase (const boost::filesystem::path& file_path): path_(file_path) {}
    virtual ~FileImplBase() noexcept {}

    const boost::filesystem::path& path() const {return path_;}
};




class BufferedFileImpl: public FileImplBase, public IBinaryIOStream, public EnableSharedFromThis<BufferedFileImpl> {
    int fd_{};
    bool closed_{true};
public:
    BufferedFileImpl(boost::filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);
    virtual ~BufferedFileImpl() noexcept;


    virtual void close();

    virtual bool is_closed() const {return closed_;}

    virtual uint64_t size() {
        return boost::filesystem::file_size(path_);
    }

    virtual uint64_t seek(uint64_t position);
    virtual uint64_t fpos();

    virtual size_t read(uint8_t* buffer, size_t size);
    virtual size_t write(const uint8_t* buffer, size_t size);

    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size);
    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size);

    virtual void fsync();

    virtual void fdsync();

    virtual void flush() {}

    virtual BinaryInputStream istream()
    {
        return BinaryInputStream(StaticPointerCast<IBinaryInputStream>(shared_from_this()));
    }

    virtual BinaryOutputStream ostream()
    {
        return BinaryOutputStream(StaticPointerCast<IBinaryOutputStream>(shared_from_this()));
    }
};




class DMAFileImpl: public FileImplBase, public EnableSharedFromThis<DMAFileImpl> {
    int fd_{};
    bool closed_{true};
public:
    DMAFileImpl(boost::filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);

    virtual ~DMAFileImpl() noexcept;

    virtual uint64_t alignment() {return 512;}

    virtual void close();

    virtual uint64_t size() {
        return boost::filesystem::file_size(path_);
    }


    virtual bool is_closed() const {return closed_;}

    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size);

    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size);

    virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error);


private:
    uint64_t process_single_io(uint8_t* buffer, uint64_t offset, uint64_t size, int command, const char* opname);
};




}}
