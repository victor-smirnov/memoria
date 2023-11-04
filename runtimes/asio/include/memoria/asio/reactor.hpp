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

#include <utility>

#include <boost/asio.hpp>

namespace memoria::asio {

using IOContext = boost::asio::io_context;
using IOContextPtr = std::shared_ptr<IOContext>;


IOContext& io_context();
IOContextPtr io_context_ptr();
bool has_io_context();
void set_io_context(IOContextPtr io_context);

}
