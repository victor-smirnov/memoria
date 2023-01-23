
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

#include "hrpc_impl_context.hpp"
#include "hrpc_impl_connection.hpp"


namespace memoria::hrpc {

PoolSharedPtr<Connection> HRPCContextImpl::connection() {
    return connection_;
}

void HRPCContextImpl::close_stream(bool input, StreamCode code)
{
    if (input) {
        if (code < input_streams_.size() && !input_streams_[code].is_null()) {
            input_streams_[code]->do_close_stream();
        }
    }
    else {
        if (code < output_streams_.size() && !output_streams_[code].is_null()) {
            output_streams_[code]->do_close_stream();
        }
    }
}

void HRPCContextImpl::new_message(StreamBatch&& msg, StreamCode code)
{
    if (code < input_streams_.size() && !input_streams_[code].is_null()) {
        input_streams_[code]->new_message(std::move(msg));
    }
}

}
