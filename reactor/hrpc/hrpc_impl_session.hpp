
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

#include "hrpc_impl_session_base.hpp"

namespace memoria::hrpc {

class HRPCSessionImpl:
        public HRPCSessionBase,
        public pool::enable_shared_from_this<HRPCSessionImpl>
{    
    using Base = HRPCSessionBase;
    MessageProviderPtr message_provider_;

public:
    HRPCSessionImpl(
        EndpointRepositoryImplPtr endpoints,
        MessageProviderPtr message_provider,
        ProtocolConfig config,
        SessionSide session_side
    ):
        Base(endpoints, config, session_side, message_provider->needs_session_id()),
        message_provider_(message_provider)
    {
    }


    virtual void handle_messages() override
    {        
        try {
            start_session();

            while (!is_closed())
            {
                RawMessagePtr raw_msg = message_provider_->read_message();
                if (MMA_UNLIKELY(!raw_msg)) {                    
                    break;
                }

                Base::handle_message(std::move(raw_msg));
            }

            do_session_close();
            do_session_cleanup();
        }
        catch (...) {
            println("Exception in handle_messages(): {}", (int)session_side_);
            try {
                do_session_close();
            }
            catch (...) {
                println("Can't close HRPC session");
            }

            do_session_cleanup();
            throw;
        }
    }

    virtual bool is_transport_closed() override {
        return message_provider_->is_closed();
    }

private:
    virtual SessionImplPtr self() override {
        return shared_from_this();
    }


    void write_message(const MessageHeader& header, const uint8_t* data) override {
        message_provider_->write_message(header, data);
    }




    void do_session_close() noexcept override {
        message_provider_->close();
    }
};

}
