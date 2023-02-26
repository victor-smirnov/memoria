
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

#pragma once

#include "hrpc_impl_common.hpp"

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/core/tools/optional.hpp>

namespace memoria::hrpc {

class HRPCEndpointRepositoryImpl final:
        public EndpointRepository,
        public pool::enable_shared_from_this<HRPCEndpointRepositoryImpl>
{
    ska::flat_hash_map<EndpointID, RequestHandlerFn> handlers_;

public:
    HRPCEndpointRepositoryImpl() {}

    void add_handler(const EndpointID& endpoint_id, RequestHandlerFn handler) override {
        handlers_[endpoint_id] = handler;
    }

    void remove_handler(const EndpointID& endpoint_id) override {
        handlers_.erase(endpoint_id);
    }

    Optional<RequestHandlerFn> get_handler(const EndpointID& endpoint_id) override
    {
        auto ii = handlers_.find(endpoint_id);
        if (ii != handlers_.end()) {
            return ii->second;
        }

        return {};
    }
};

}
