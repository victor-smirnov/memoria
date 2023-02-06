
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

Response normal_rq_handler(ContextPtr context)
{
    int arg1 = context->request().parameters().get(ARG1).to_i32();
    int arg2 = context->request().parameters().get(ARG2).to_i32();

    auto rs = Response::ok(arg1 + arg2);

    println("Response: {}", rs.to_pretty_string());

    return rs;
}

void normal_rq_cient(SessionPtr session)
{
    Request rq = Request::make();

    rq.set_parameter(ARG1, 1);
    rq.set_parameter(ARG2, 2);

    println("RQ: {}", rq.object().ctr().to_pretty_string());

    auto call = session->call(NORMAL_RQ_TEST, rq);
    call->wait();

    println("RS: {}", call->response().object().ctr().to_pretty_string());

    int32_t result = call->response().result().cast_to<Integer>().value_t();
    if (result == 3) {
        println("Normal RQ test: OK");
    }
    else {
        println("Normal RQ test failed: expected: {}, actual: {}", 3, result);
    }
}

}
