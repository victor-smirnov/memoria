
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

Response input_stream_handler(ContextPtr context)
{
    println("Start input_stream_handler");
    auto ch = context->input_channel(0);

    int64_t sum{}, cnt{};
    Message msg;
    while (ch->pop(msg)) {
        cnt++;
        auto val = msg.data().to_i32();
        sum += val;
        if (cnt % 10000 == 0) {
            println("cnt: {}", cnt);
        }
    }

    println("Finish input_stream_handler");
    return Response::ok(sum);
}

Response output_stream_handler(ContextPtr context)
{
    println("Start output_stream_handler");

    int32_t size = context->request().parameters().expect(ARG1).to_i32();
    auto ch = context->output_channel(0);

    int64_t sum = 0;
    for (int c = 0; c < size; c++)
    {
        Message msg(hermes::HermesCtr::make_pooled());
        msg.set_data(msg.object().ctr().make(c));
        ch->push(msg);
        sum += c;
    }

    ch->close();

    println("Finish output_stream_handler");
    return Response::ok(sum);
}

}

using namespace memoria;
using namespace memoria::hrpc;

int32_t STREAM_SIZE = 100000;

TEST_CASE( "Input channel handling" ) {
    auto rq = Request::make();
    rq.set_output_channels(1);

    auto call = session()->call(INPUT_CHANNEL_TEST, rq);
    auto ch = call->output_channel(0);

    size_t sum = 0;
    for (int c = 0; c < STREAM_SIZE; c++)
    {
        Message msg(hermes::HermesCtr::make_pooled());
        msg.set_data(msg.object().ctr().make(c));
        sum += c;
        ch->push(msg);
    }

    ch->close();
    call->wait();

    hrpc::Response rs = call->response();
    REQUIRE(rs.status_code() == hrpc::StatusCode::OK);

    int64_t result = rs.result().to_i64();
    CHECK(result == sum);
}


TEST_CASE( "Output channel handling" ) {
    auto rq = Request::make();
    rq.set_input_channels(1);

    rq.set_parameter(ARG1, STREAM_SIZE);

    auto call = session()->call(OUTPUT_CHANNEL_TEST, rq);
    auto ch = call->input_channel(0);

    Message msg;
    size_t sum{}, cnt{};
    while (ch->pop(msg)) {
        cnt++;
        auto val = msg.data().to_i32();
        sum += val;
        if (cnt % 10000 == 0) {
            println("cnt: {}", cnt);
        }
    }

    call->wait();

    hrpc::Response rs = call->response();
    REQUIRE(rs.status_code() == hrpc::StatusCode::OK);

    int64_t result = rs.result().to_i64();
    CHECK(result == sum);
}
