
// Copyright 2022 Victor Smirnov
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


#include <memoria/core/ssrle/ssrle.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/iovector/io_substream_ssrle.hpp>

#include <bitset>
#include <iostream>

using namespace memoria;

int main(int, char**)
{
    auto buf = io::make_packed_ssrle_buffer(2);

    SSRLERun<8> rr(7, 0x010203040506AA, 9);
    println("Run: {}", rr);
}
