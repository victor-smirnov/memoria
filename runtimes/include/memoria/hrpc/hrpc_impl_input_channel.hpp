
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


#include <memoria/hrpc/hrpc_impl_common.hpp>

#include <list>

namespace memoria::hrpc::st {

class HRPCInputChannelImpl:
        public InputChannel,
        public pool::enable_shared_from_this<HRPCInputChannelImpl>
{
protected:
    SessionImplPtr session_;
    CallID call_id_;
    ChannelCode channel_code_;
    bool closed_;
    bool call_side_;

    uint64_t batch_size_{};
    uint64_t batch_size_limit_;

public:
    HRPCInputChannelImpl(
        SessionImplPtr session,
        CallID call_id,
        ChannelCode stream_id,
        uint64_t batch_size_limit,
        bool call_side
    );

    ChannelCode code() override {
        return channel_code_;
    }

    PoolSharedPtr<Session> session() override;

    virtual void new_message(Message&& msg) = 0;
    virtual void do_close_channel() = 0;
private:
};

}
