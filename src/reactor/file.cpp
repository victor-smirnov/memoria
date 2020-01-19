
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


#ifdef __linux__
#include "linux/linux_file_impl.hpp"
#elif __APPLE__
#include "macosx/macosx_file_impl.hpp"
#elif _WIN32
#include "msvc/msvc_file_impl.hpp"
#else
#error "Unsupported platform"
#endif





namespace memoria {
namespace reactor {

    

void File::close() {
    return this->ptr_->close();
}

bool File::is_closed() const {
    return this->ptr_->is_closed();
}


uint64_t File::size() {
    return this->ptr_->size();
}

size_t File::read(uint8_t* buffer, size_t size)
{
    return this->ptr_->read(buffer, size);
}

size_t File::write(const uint8_t* buffer, size_t size)
{
    return this->ptr_->write(buffer, size);
}

size_t File::read(uint8_t* buffer, uint64_t offset, size_t size)
{
    return this->ptr_->read(buffer, offset, size);
}

size_t File::write(const uint8_t* buffer, uint64_t offset, size_t size)
{
    return this->ptr_->write(buffer, offset, size);
}



void File::fsync() {
    return this->ptr_->fsync();
}

void File::fdsync()
{
    return this->ptr_->fdsync();
}

BinaryInputStream File::istream()
{
    return this->ptr_->istream();
}

BinaryOutputStream File::ostream()
{
    return this->ptr_->ostream();
}
    
const filesystem::path& File::path()
{
    return this->ptr_->path();
}

uint64_t File::seek(uint64_t pos) {
    return this->ptr_->seek(pos);
}

uint64_t File::fpos() {
    return this->ptr_->fpos();
}



void DMAFile::close() {
    return this->ptr_->close();
}

bool DMAFile::is_closed() const {
    return this->ptr_->is_closed();
}



uint64_t DMAFile::alignment() {
    return this->ptr_->alignment();
}

uint64_t DMAFile::size() {
    return this->ptr_->size();
}

size_t DMAFile::read(uint8_t* buffer, uint64_t offset, size_t size)
{
    return this->ptr_->read(buffer, offset, size);
}

size_t DMAFile::write(const uint8_t* buffer, uint64_t offset, size_t size)
{
    return this->ptr_->write(buffer, offset, size);
}

size_t DMAFile::process_batch(IOBatchBase& batch, bool rise_ex_on_error)
{
    return this->ptr_->process_batch(batch, rise_ex_on_error);
}


const filesystem::path& DMAFile::path()
{
    return this->ptr_->path();
}


}}
