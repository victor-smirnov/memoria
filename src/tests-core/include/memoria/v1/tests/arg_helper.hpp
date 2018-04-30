
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/tests/tests.hpp>

#include <string>
#include <vector>
#include <string.h>

namespace memoria {
namespace v1 {
namespace tests {

class ThreadsArgHelper {
    std::vector<char*> args_;
public:
    ThreadsArgHelper(char** args)
    {
        while (args && *args)
        {
            args_.push_back(make_copy(*args));
            ++args;
        }

        args_.push_back(nullptr);

        fix_thread_arg();
    }

    ~ThreadsArgHelper() noexcept
    {
        for (auto& arg: args_){
            if (arg) {
                free_system(arg);
            }
        }
    }

    int argc() const noexcept {
        return args_.size() - 1;
    }

    char** args() noexcept {
        return args_.data();
    }

private:
    char* make_copy(const char* str);
    char* has_arg(const std::string& target_arg_name);
    void fix_thread_arg();
};


}}}
