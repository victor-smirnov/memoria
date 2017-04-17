
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


#include "../../filesystem/path.hpp"
#include "../../filesystem/operations.hpp"
#include "../message/fiber_io_message.hpp"

#include <memoria/v1/core/tools/iostreams.hpp>

#include "../dma_buffer.hpp"
#include "msvc_buffer_vec.hpp"
#include "msvc_io_poller.hpp"

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory>

#include <FileAPI.h>

namespace memoria {
namespace v1 {
namespace reactor {

enum class FileFlags : uint32_t {
	RDONLY     = 1 << 0,
	WRONLY     = 1 << 1,
	RDWR       = 1 << 2,
	TRUNCATE   = 1 << 3,
	APPEND     = 1 << 4,
	CREATE     = 1 << 5,
	CLOEXEC    = 1 << 6,
	EXCL       = 1 << 7,
	NONE       = 0,

	DEFAULT = RDWR
};

static inline FileFlags operator|(FileFlags f1, FileFlags f2) {
    return (FileFlags)((uint32_t)f1 | (uint32_t)f2);
}


static inline FileFlags operator&(FileFlags f1, FileFlags f2) {
	return (FileFlags)((uint32_t)f1 & (uint32_t)f2);
}

static inline bool to_bool(FileFlags f) {
	return ((uint32_t)f) != 0;
}

using mode_t = uint32_t;

enum class FileMode: mode_t {
    IRWXU,
    IRUSR,
    IWUSR,
    IXUSR,
    IRWXG,
    IRGRP,
    IWGRP,
    IXGRP,
    IRWXO,
    IROTH,
    IWOTH,
    IXOTH,
    ISUID,
    ISGID,
    ISVTX,
    
    IRWUSR = IRUSR | IWUSR,
    IDEFLT = IRWUSR | IRGRP | IROTH
};




class File {
protected:
    filesystem::path path_;
public:
	File(filesystem::path file_path): path_(file_path) {};
	virtual ~File() noexcept {};
    
	virtual uint64_t alignment() = 0;

	virtual uint64_t size() {
		return filesystem::file_size(path_);
	}

    virtual void close() = 0;
    
    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size) = 0;
    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size) = 0;
    
    virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true) = 0;
    
    virtual void fsync() = 0;
    virtual void fdsync() = 0;
    
    const filesystem::path& path() const {return path_;}

    virtual DataInputStream istream(uint64_t position = 0, size_t buffer_size = 4096) = 0;
    virtual DataOutputStream ostream(uint64_t position = 0, size_t buffer_size = 4096) = 0;
};

std::shared_ptr<File> open_dma_file(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);
std::shared_ptr<File> open_buffered_file(filesystem::path file_path, FileFlags flags, FileMode mode = FileMode::IDEFLT);




    
}}}
