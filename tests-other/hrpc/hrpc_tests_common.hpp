
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

#include <memoria/core/tools/uid_256.hpp>
#include <memoria/hrpc/hrpc.hpp>

#include <memoria/seastar/seastar.hpp>
#include <seastar/core/thread.hh>

#include <catch2/catch_test_macros.hpp>

namespace memoria {

constexpr UID256 MISSING_ENDPOINT    = UID256{7464156137039056724ull, 8026313588769890819ull, 15146744332510506992ull, 115762686851554294ull};

constexpr UID256 ENDPOINT1           = UID256{9070215745021199395ull, 13062572892696294180ull, 1686871169247065673ull, 136488551204032119ull};
constexpr UID256 NORMAL_RQ_TEST      = UID256{12704329573075247253ull, 13343965424605052971ull, 597559489269328553ull, 124611251894768530ull};
constexpr UID256 ERRORS_TEST         = UID256{13033736633808828634ull, 16986347482380745847ull, 9310091279496329336ull, 96711675074291270ull};
constexpr UID256 INPUT_CHANNEL_TEST  = UID256{3809053522172463906ull, 7190220530456867245ull, 8338883021968102619ull, 136189001619003787ull};
constexpr UID256 OUTPUT_CHANNEL_TEST = UID256{6671401374715659763ull, 1623900077831920038ull, 17292896288624218196ull, 88380478914819099ull};
constexpr UID256 CANCEL_RQ_TEST      = UID256{6023599194013054336ull, 8480974567391737759ull, 16398945128188874689ull, 79721979216655488ull};

using ContextPtr = PoolSharedPtr<hrpc::st::Context>;
using SessionPtr = PoolSharedPtr<hrpc::st::Session>;

constexpr NamedCode ARG1 = NamedCode(1, "arg1");
constexpr NamedCode ARG2 = NamedCode(2, "arg2");
constexpr NamedCode ARG3 = NamedCode(3, "arg3");

constexpr NamedCode STREAM1 = NamedCode(1, "stream1");
constexpr NamedCode STREAM2 = NamedCode(2, "stream2");
constexpr NamedCode STREAM3 = NamedCode(3, "stream3");

SessionPtr session();
void set_session(SessionPtr session);

hrpc::Response normal_rq_handler(ContextPtr context);
hrpc::Response errors_handler(ContextPtr context);
hrpc::Response input_stream_handler(ContextPtr context);
hrpc::Response output_stream_handler(ContextPtr context);
hrpc::Response cancel_rq_handler(ContextPtr context);


}
