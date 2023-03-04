
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

#include <boost/fiber/asio/yield.hpp>
#include <boost/fiber/asio/round_robin.hpp>


namespace {
void fn_123456() {};
}

namespace boost::fibers::asio {

thread_local yield_t yield{};
boost::asio::io_context::id round_robin::service::id;

}
