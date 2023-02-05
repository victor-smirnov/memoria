
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

#include <memoria/core/tools/iostreams.hpp>
#include <memoria/reactor/io_base.hpp>

namespace memoria {
namespace reactor {

class IPipeInputStream: public IBinaryInputStream {
public:
    virtual IOHandle handle() const = 0;
    virtual IOHandle detach() = 0;
};

class IPipeOutputStream: public IBinaryOutputStream {
public:
    virtual IOHandle handle() const = 0;
    virtual IOHandle detach() = 0;
};


class PipeInputStream: public PimplBase<IPipeInputStream> {

    using Base = PimplBase<IPipeInputStream>;
public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS_NO_DTR(PipeInputStream)

    size_t read(uint8_t* data, size_t size) {
        return this->ptr_->read(data, size);
    }

    void close() {
        this->ptr_->close();
    }

    bool is_closed() const {
        return this->ptr_->is_closed();
    }

    IOHandle handle() const {
        return this->ptr_->handle();
    }

    IOHandle detach() {
        return this->ptr_->detach();
    }

    operator BinaryInputStream () const {
        return BinaryInputStream(this->ptr_);
    }
};


class PipeOutputStream: public PimplBase<IPipeOutputStream> {
    using Base = PimplBase<IPipeOutputStream>;
public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS_NO_DTR(PipeOutputStream)

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

    IOHandle handle() const {
        return this->ptr_->handle();
    }

    IOHandle detach() {
        return this->ptr_->detach();
    }

    operator BinaryOutputStream () const {
        return BinaryOutputStream(this->ptr_);
    }
};



struct PipeStreams {
    PipeInputStream  input;
    PipeOutputStream output;
};

PipeStreams open_pipe();
PipeStreams duplicate_pipe(IOHandle input, IOHandle output);

PipeInputStream open_input_pipe(const char* name);
PipeOutputStream open_output_pipe(const char* name);

}}
