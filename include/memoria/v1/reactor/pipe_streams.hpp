
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

#pragma once

#include <memoria/v1/core/tools/iostreams.hpp>
#include <memoria/v1/reactor/io_base.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

class IPipeInputStream: public IBinaryInputStream {
public:
    virtual IOHandle hande() const = 0;
};

class IPipeOutputStream: public IBinaryOutputStream {
public:
    virtual IOHandle hande() const = 0;
};


class PipeInputStream: public PimplBase<IPipeInputStream> {

    using Base = PimplBase<IPipeInputStream>;
public:
    PipeInputStream(): Base(PtrType()){}
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS_NO_DTR(PipeInputStream)

    size_t read(uint8_t* data, size_t size) {
        return this->ptr_->read(data, size);
    }

    void close() {
        this->ptr_->close();
    }

    bool is_closed() const {
        return this->ptr_->is_closed();
    }

    IOHandle hande() const {
        return this->ptr_->is_closed();
    }};


class PipeOutputStream: public PimplBase<IPipeOutputStream> {
    using Base = PimplBase<IPipeOutputStream>;
public:
    PipeOutputStream(): Base(PtrType()){}
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS_NO_DTR(PipeOutputStream)

    size_t write(const uint8_t* data, size_t size) {
        return this->ptr_->write(data, size);
    }

    void flush() {
        this->ptr_->flush();
    }

    void close() {
        this->ptr_->close();
    }

    bool is_closed() const {
        return this->ptr_->is_closed();
    }

    IOHandle hande() const {
        return this->ptr_->is_closed();
    }
};

#ifdef MMA1_POSIX

struct PipeStreams {
    PipeInputStream  input;
    PipeOutputStream output;
};

PipeStreams open_pipe();

//PipeInputStream create_input_pipe(IOHandle handle);
//PipeOutputStream create_output_pipe(IOHandle handle);

#endif

PipeInputStream open_input_pipe(const char16_t* name);
PipeOutputStream open_output_pipe(const char16_t* name);

}}}
