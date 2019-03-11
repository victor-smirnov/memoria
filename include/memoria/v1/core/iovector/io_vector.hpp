
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/iovector/io_symbol_sequence.hpp>
#include <memoria/v1/core/iovector/io_substream.hpp>

#include <vector>

namespace memoria {
namespace v1 {
namespace io {

class IOVector {
    IOSymbolSequence symbol_sequence_;

    std::vector<std::unique_ptr<IOSubstream>> substreams_;

public:
    IOVector(int32_t symbols):
        symbol_sequence_(make_packed_owning_symbol_sequence(symbols))
    {}

    void reset()
    {
        symbol_sequence_.reset();

        for (auto& substream: substreams_) {
            substream->reset();
        }
    }

    void add_substream(std::unique_ptr<IOSubstream>&& ptr)
    {
        substreams_.push_back(std::move(ptr));
    }

    IOSymbolSequence& symbol_sequence() {
        return symbol_sequence_;
    }

    const IOSymbolSequence& symbol_sequence() const {
        return symbol_sequence_;
    }

    size_t substreams() const {
        return substreams_.size();
    }

    IOSubstream* substream(size_t num) {
        return substreams_[num].get();
    }

    const IOSubstream* substream(size_t num) const {
        return substreams_[num].get();
    }
};


struct IOVectorProducer {
    virtual ~IOVectorProducer() noexcept {}

    virtual bool populate(IOVector& io_vectors) = 0;
};

}}}
