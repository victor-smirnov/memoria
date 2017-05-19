
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

class FileImpl {
protected:
    filesystem::path path_;
public:
    FileImpl (filesystem::path file_path): path_(file_path) {}
    virtual ~FileImpl() noexcept {}
    
    virtual void close() = 0;
    
    virtual uint64_t alignment() = 0;
    
    virtual uint64_t size() {
        return filesystem::file_size(path_);
    }
    
    virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size) = 0;
    virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size) = 0;
    
    virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true) = 0;
    
    virtual void fsync() = 0;
    virtual void fdsync() = 0;
    
    virtual DataInputStream istream(uint64_t position = 0, size_t buffer_size = 4096) = 0;
    virtual DataOutputStream ostream(uint64_t position = 0, size_t buffer_size = 4096) = 0;
    
    const filesystem::path& path() const {return path_;}
};

}}}
