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

#include <memoria/core/strings/format.hpp>

#include <boost/process.hpp>
#include <boost/process/extend.hpp>

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/mutex.hpp>

#include <vector>
#include <functional>

namespace memoria::tests {

namespace bf = boost::fibers;
namespace ba = boost::asio;
namespace bp = boost::process;

enum class ProcessStatus {
    NORMAL, CRASHED, TERMINATED, OTHER
};

class Process: public std::enable_shared_from_this<Process> {
    bp::child child_;
    U8String path_;
    std::vector<std::string> args_;
    bp::pipe p_in_;
    bp::pipe p_out_;
    bp::pipe p_err_;

    std::function<void(std::error_code)> on_start_error_;
    std::function<void()> on_start_success_;

    std::function<void (U8StringView span, bool)> process_output_handler_;

    bf::mutex exit_mtx_;
    bf::condition_variable exit_var_;

public:

    Process(U8String path, const std::vector<U8String>& args);

    void set_output_handler(std::function<void (U8StringView span, bool)> fn) {
        process_output_handler_ = fn;
    }

    void run();
    void kill();
};

}
