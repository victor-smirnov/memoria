
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

#include "hrpc_impl_connection.hpp"
#include "hrpc_impl_call.hpp"


namespace memoria::hrpc {

void HRPCConnectionImpl::handle_return(const MessageHeader& header)
{
    auto msg = read_message(header.message_size());
    Response rs(msg.root().as_tiny_object_map());

    auto ii = calls_.find(header.call_id());
    if (ii != calls_.end())
    {
        auto ptr = ii->second.lock();
        if (!ptr.is_null()) {
            ptr->set_response(rs);
        }
        calls_.erase(ii);
    }
}


}
