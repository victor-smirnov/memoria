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

#include <memoria/asio/reactor.hpp>
#include <memoria/asio/round_robin.hpp>
#include <memoria/tests/runner/process.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/strings/u8_string_view.hpp>

#include <boost/fiber/all.hpp>

#include <memory>

using namespace memoria;
using namespace memoria::asio;

int main(int argc, const char** argv)
{
    IOContextPtr io_ctx = std::make_shared<IOContext>();
    set_io_context(io_ctx);

    boost::fibers::use_scheduling_algorithm< memoria::asio::round_robin>(io_ctx);
    int result = 0;

    boost::fibers::fiber([&]{
        std::vector<U8String> args;
        //args.push_back("1");

        auto proc = std::make_shared<memoria::tests::Process>("/usr/bin/ls", args);

        proc->set_output_handler([](auto str, bool){
            println("out: {}", str);
        });

        proc->run();

        io_context().stop();
    }).detach();

    io_ctx->run();

    return result;
}
