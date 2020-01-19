

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

#include <memoria/core/types.hpp>

#include <memoria/core/tools/perror.hpp>
#include <memoria/core/types.hpp>
#include <memoria/core/tools/ptr_cast.hpp>
#include <memoria/core/tools/referenceable.hpp>
#include <memoria/core/tools/pimpl_base.hpp>

#include <limits>
#include <memory>
#include <utility>
#include <type_traits>

#include <stdint.h>

namespace memoria {

class IBinaryInputStream: public Referenceable {
public:
    virtual ~IBinaryInputStream() noexcept {}
    virtual size_t read(uint8_t* data, size_t size) = 0;
    virtual void close() = 0;
    virtual bool is_closed() const = 0;
};


class IBinaryOutputStream: public Referenceable {
public:
    virtual ~IBinaryOutputStream() noexcept {}
    virtual size_t write(const uint8_t* data, size_t size) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual bool is_closed() const = 0;
};

class IBinaryIOStream: public IBinaryInputStream, public IBinaryOutputStream {
public:
    virtual ~IBinaryIOStream() noexcept {}
};


struct BinaryInputStream final: PimplBase<IBinaryInputStream> {

    using Base = PimplBase<IBinaryInputStream>;

    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(BinaryInputStream)


    size_t read(uint8_t* data, size_t size) {
        return ptr_->read(data, size);
    }

    bool is_closed() const {
        return this->ptr_->is_closed();
    }

    void close() {
        return this->ptr_->close();
    }

};


struct BinaryOutputStream final: PimplBase<IBinaryOutputStream>{

    using Base = PimplBase<IBinaryOutputStream>;

    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(BinaryOutputStream)
    
    size_t write(const uint8_t* data, size_t size) {
        return ptr_->write(data, size);
    }
    
    void flush() {
        return ptr_->flush();
    }

    void close() {
        ptr_->close();
    }


    bool is_closed() const {
        return this->ptr_->is_closed();
    }
};

    
}
