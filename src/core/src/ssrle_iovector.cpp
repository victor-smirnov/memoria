// Copyright 2022 Victor Smirnov
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

#include <memoria/core/datatypes/buffer/ssrle_buffer.hpp>

namespace memoria {
namespace io {

std::unique_ptr<IOSSRLEBufferBase> make_packed_ssrle_buffer(size_t alphabet_size)
{
    if (alphabet_size == 1) {
        return std::make_unique<IOSSRLEBufferImpl<1>>();
    }
    else if (alphabet_size == 2) {
        return std::make_unique<IOSSRLEBufferImpl<2>>();
    }
    else if (alphabet_size == 3) {
        return std::make_unique<IOSSRLEBufferImpl<3>>();
    }
    else if (alphabet_size == 4) {
        return std::make_unique<IOSSRLEBufferImpl<4>>();
    }
    else if (alphabet_size == 8) {
        return std::make_unique<IOSSRLEBufferImpl<8>>();
    }
    else if (alphabet_size == 16) {
        return std::make_unique<IOSSRLEBufferImpl<16>>();
    }
    else if (alphabet_size == 32) {
        return std::make_unique<IOSSRLEBufferImpl<32>>();
    }
    else if (alphabet_size == 64) {
        return std::make_unique<IOSSRLEBufferImpl<64>>();
    }
    else if (alphabet_size == 128) {
        return std::make_unique<IOSSRLEBufferImpl<128>>();
    }
    else if (alphabet_size == 256) {
        return std::make_unique<IOSSRLEBufferImpl<256>>();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Unsupported alphabet size value: {}", alphabet_size).do_throw();
    }
}

}}
