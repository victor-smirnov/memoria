
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

#include "../os_al.hpp"

#include <sys/wait.h>

namespace memoria::tests {

bool is_normal_exit(int code) {
    return WIFEXITED(code);
}

bool is_terminated(int code) {
     return WTERMSIG(code);
}

bool is_crashed(int code) {
    return WIFSIGNALED(code);
}

void kill_process(boost::process::pid_t pid) {
    kill(pid, SIGKILL);
}

}
