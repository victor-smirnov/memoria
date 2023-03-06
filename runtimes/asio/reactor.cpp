
// Copyright 2023 Victor Smirnov
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

#include <memoria/asio/yield.hpp>
#include <memoria/asio/round_robin.hpp>
#include <memoria/asio/reactor.hpp>

#include <memoria/core/types.hpp>
#include <memoria/core/tools/result.hpp>


namespace memoria::asio {

thread_local yield_t yield{};
boost::asio::io_context::id round_robin::service::id;

thread_local IOContextPtr io_context_{};

IOContext& io_context() {
    if (MMA_LIKELY((bool)io_context_)) {
        return *io_context_;
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("ASIO io_service is not set for this thread").do_throw();
    }
}

IOContextPtr io_context_ptr() {
    return io_context_;
}

bool has_io_context() {
    return (bool)io_context_;
}

void set_io_context(IOContextPtr io_context) {
    io_context_ = io_context;
}


}
