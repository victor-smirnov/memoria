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

#include <memoria/tests/runner/process.hpp>
#include <memoria/asio/reactor.hpp>
#include <boost/process/async.hpp>

#include <boost/asio/post.hpp>

#include "os_al.hpp"

#include <thread>

namespace memoria::tests {

Process::Process(U8String path, const std::vector<U8String>& args):
    path_(path)
{
    for (const auto& arg: args) {
        args_.emplace_back(arg.to_std_string());
    }
}



void Process::run()
{
    std::weak_ptr<Process> wself = this->shared_from_this();


    auto io_ctx = asio::io_context_ptr();

    std::thread th([io_ctx, wself, this]{

        std::thread pout_reader;
        std::thread perr_reader;

        if (process_output_handler_)
        {
            child_ = boost::process::child(
                path_.to_std_string(),
                args_,
                bp::std_out > p_out_,
                bp::std_err > p_err_
            );

            pout_reader = std::thread([io_ctx, wself, this]{
                char buf[4096];
                auto proc = wself.lock();

                while (p_out_.is_open())
                {
                    auto res = p_out_.read(buf, sizeof(buf));
                    if (res > 0) {
                        proc->process_output_handler_(U8StringView(buf, res), true);
                    }
                    else {
                        break;
                    }
                }
            });

            perr_reader = std::thread([io_ctx, wself, this]{
                char buf[4096];
                auto proc = wself.lock();

                while (p_err_.is_open())
                {
                    auto res = p_err_.read(buf, sizeof(buf));
                    if (res > 0) {
                        proc->process_output_handler_(U8StringView(buf, res), false);
                    }
                    else {
                        break;
                    }
                }
            });
        }
        else {
            child_ = boost::process::child(
                path_.to_std_string(),
                args_,
                bp::std_out > bp::null,
                bp::std_err > bp::null
            );
        }

        child_.wait();

        int code = child_.native_exit_code();
        println("Exit status: {}", code);

        if (is_normal_exit(code)) {
            println("Normal exit");
        }
        else if (is_crashed(code)) {
            println("Crashed");
        }
        else if (is_terminated(code)) {
            println("Terminated");
        }
        else {
            println("Other");
        }

        if (pout_reader.joinable()) {
            pout_reader.join();
        }

        if (perr_reader.joinable()) {
            perr_reader.join();
        }

        ba::post(*io_ctx, [this](){
            exit_var_.notify_all();
        });
    });

    th.detach();

    std::unique_lock<bf::mutex> lock(exit_mtx_);
    exit_var_.wait(lock);
}

}
