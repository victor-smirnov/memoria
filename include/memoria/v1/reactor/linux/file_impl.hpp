
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

#include "../message/fiber_io_message.hpp"
#include "buffer_vec.hpp"
#include "io_poller.hpp"

#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

namespace memoria {
namespace v1 {
namespace reactor {

enum class FileFlags: int {
    RDONLY      = O_RDONLY, 
    WRONLY      = O_WRONLY, 
    RDWR        = O_RDWR, 
    TRUNCATE    = O_TRUNC, 
    APPEND      = O_APPEND, 
    CREATE      = O_CREAT, 
    CLOEXEC     = O_CLOEXEC, 
    
    DEFAULT = RDWR,
    
    NONE = 0
};

static inline FileFlags operator|(FileFlags f1, FileFlags f2) {
    return (FileFlags)((int)f1 | (int)f2);
}

enum class FileMode: mode_t {
    IRWXU = S_IRWXU,
    IRUSR = S_IRUSR,
    IWUSR = S_IWUSR,
    IXUSR = S_IXUSR,
    IRWXG = S_IRWXG,
    IRGRP = S_IRGRP,
    IWGRP = S_IWGRP,
    IXGRP = S_IXGRP,
    IRWXO = S_IRWXO,
    IROTH = S_IROTH,
    IWOTH = S_IWOTH,
    IXOTH = S_IXOTH,
    ISUID = S_ISUID,
    ISGID = S_ISGID,
    ISVTX = S_ISVTX,
    
    IRWUSR = IRUSR | IWUSR,
    IDEFLT = IRWUSR | IRGRP | IROTH
};

enum class FileSeek: int {
    BEGIN   = SEEK_SET,
    CURRENT = SEEK_CUR,
    END     = SEEK_END
};


class File {
    int fd_{};
    std::string path_;
public:
    File (std::string path, FileFlags flags, FileMode mode = FileMode::IDEFLT);
    virtual ~File() noexcept;
    
    void close();
    
    int64_t seek(int64_t pos, FileSeek whence);
    
    int64_t read(char* buffer, int64_t offset, int64_t size);
    int64_t write(const char* buffer, int64_t offset, int64_t size);
    
    size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true);
    
    void fsync();
    void fdsync();
    
private:
    int64_t process_single_io(char* buffer, int64_t offset, int64_t size, int command, const char* opname);
};


namespace details {
	template<typename T>
	struct aligned_delete {
		void operator()(T* ptr) const {
			free(ptr);
		}
	};
}

using DMABuffer = std::unique_ptr<char, details::aligned_delete<char>>;

DMABuffer allocate_dma_buffer(size_t size);

    
}}}
