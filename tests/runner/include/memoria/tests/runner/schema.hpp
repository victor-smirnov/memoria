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

#include <memoria/hrpc/hrpc.hpp>

namespace memoria::tests {

using namespace memoria::hrpc;

namespace endpoints {
// Run on the Runner's side
constexpr EndpointID RUNNER_COMMANDS    = {15316478630603264947ull, 14206805795163287033ull, 16075681587746764376ull, 127309690043863733ull};
constexpr EndpointID REPORT_TASK_STATUS = {4081560332323748938ull, 7728413522935963240ull, 15267250968660499015ull, 125343238917219289ull};

// Run on the Worker's side
constexpr EndpointID WORKER_COMMANDS  = {3018454931530045254ull, 7957733998182560416ull,  17711118674485430522ull, 112336914281210907ull};
constexpr EndpointID RUN_NEW_TASK     = {5586959636792308245ull, 17389222160606271799ull, 16088609924246256743ull, 140035112821765600ull};
}

enum class RunnersCommands: uint32_t {
    REGISTER_WORKER, WORKER_IS_READY
};

enum class WorkersCommands: uint32_t {
    SHUTDOWN_WORKER
};

// Runner
Response runner_commands(PoolSharedPtr<st::Context>);
Response report_task_status(PoolSharedPtr<st::Context>);

// Worker
Response worker_commands(PoolSharedPtr<st::Context>);
Response run_new_task(PoolSharedPtr<st::Context>);

}
