
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

#include <memoria/v1/core/memory/smart_ptrs.hpp>
#include <memoria/v1/core/regexp/icu_regexp.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/process.hpp>

#include <memoria/v1/fiber/count_down_latch.hpp>

#include <unistd.h>
#include <fstream>

namespace memoria {
namespace v1 {
namespace reactor {

class ProcessImpl {

    PipeOutputStream in_stream_;
    PipeInputStream  out_stream_;
    PipeInputStream  err_stream_;

    pid_t pid_;

    fibers::count_down_latch<int32_t> process_status_latch_{1};

public:
    ProcessImpl(char* const path, char* const* args, char* const* env)
    {
        auto pipe_out = open_pipe();
        auto pipe_in  = open_pipe();
        auto pipe_err = open_pipe();

        in_stream_  = pipe_in.output;
        out_stream_ = pipe_out.input;
        err_stream_ = pipe_err.input;

        int32_t stdout_child_fd = pipe_out.output.detach();
        int32_t stderr_child_fd = pipe_err.output.detach();
        int32_t stdin_child_fd  = pipe_in.input.detach();

        pid_ = vfork();
        if (pid_ == -1)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't create process {}", path);
        }
        else if (pid_ == 0)
        {
            // child

            if (dup2(stdin_child_fd, 0) < 0)
            {
                std::cout << "Can't dup2 stdin: " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (dup2(stderr_child_fd, 2) < 0)
            {
                std::cout << "Can't dup2 stderr: " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (dup2(stdout_child_fd, 1) < 0)
            {
                std::cout << "Can't dup2 stdout: " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (execve(path, args, env) < 0)
            {
                std::cout << "Can't execute process: " << path << ", reason: " << strerror(errno) << std::endl;
                exit(-1);
            }
        }
        else {
            // parent
            close(stdout_child_fd);
            close(stderr_child_fd);
            close(stdin_child_fd);

            engine().run_in_thread_pool_special([&]() noexcept {
                return 1;
            }, [&](auto status, auto fiber_context){

            });
        }
    }

    virtual ~ProcessImpl() noexcept {}

    int join() {

        waitpid(pid_, NULL, 0);

        return 0;
    }

    void terminate()
    {

    }

    auto out_stream() {
        return out_stream_.ptr();
    }

    auto err_stream() {
        return err_stream_.ptr();
    }

    auto in_stream() {
        return in_stream_.ptr();
    }
};


}}}
