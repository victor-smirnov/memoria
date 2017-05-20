
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

#include "file_impl.hpp"

namespace memoria {
namespace v1 {
namespace reactor {

    
File::File(Ptr pimpl): pimpl_(pimpl) {}
File::~File() noexcept {}

File::File(const File& file): pimpl_(file.pimpl_) {}
File::File(File&& file): pimpl_(std::move(file.pimpl_)) {}

File& File::operator=(const File& other) 
{
    pimpl_ = other.pimpl_;
    return *this;
}

File& File::operator=(File&& other)
{
    pimpl_ = std::move(other.pimpl_);
    return *this;
}

bool File::operator==(const File& other) const {
    return pimpl_ == other.pimpl_;
}

void File::close() {
    return pimpl_->close();
}

uint64_t File::alignment() {
    return pimpl_->alignment();
}

uint64_t File::size() {
    return pimpl_->size();
}

size_t File::read(uint8_t* buffer, uint64_t offset, size_t size)
{
    return pimpl_->read(buffer, offset, size);
}

size_t File::write(const uint8_t* buffer, uint64_t offset, size_t size)
{
    return pimpl_->write(buffer, offset, size);
}

size_t File::process_batch(IOBatchBase& batch, bool rise_ex_on_error)
{
    return pimpl_->process_batch(batch, rise_ex_on_error);
}

void File::fsync() {
    return pimpl_->fsync();
}
void File::fdsync()
{
    return pimpl_->fdsync();
}

DataInputStream File::istream(uint64_t position, size_t buffer_size)
{
    return pimpl_->istream(position, buffer_size);
}

DataOutputStream File::ostream(uint64_t position, size_t buffer_size)
{
    return pimpl_->ostream(position, buffer_size);
}
    
const filesystem::path& File::path()
{
    return pimpl_->path();
}

    
}}}
