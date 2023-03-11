
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

#include "hrpc_tests_common.hpp"

namespace memoria {

using namespace hrpc;

Response cancel_rq_handler(ContextPtr context)
{
    bool cancelled = false;
    context->set_cancel_listener([&]{
        println("RQ Cancelled");
        cancelled = true;
    });

    auto ch = context->input_channel(0);
    Message msg;
    while (ch->pop(msg)) {}

    println("Input Stream closed");

    if (!context->is_cancelled()) {
        MEMORIA_MAKE_GENERIC_ERROR("'cancelled' flag must be set here").do_throw();
    }
    else {
        return Response::ok(cancelled);
    }
}

}

using namespace memoria;
using namespace memoria::hrpc;

TEST_CASE( "Cancel request" ) {
    auto rq = Request::make();

    rq.set_output_channels(1);

    auto call = session()->call(CANCEL_RQ_TEST, rq);

    call->cancel();
    call->output_channel(0)->close();

    call->wait();

    hrpc::Response rs = call->response();
    REQUIRE(rs.status_code() == hrpc::StatusCode::OK);

    bool result = rs.result().to_bool();
    CHECK(result == true);
}


