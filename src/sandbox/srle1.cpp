
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

#include <memoria/core/packed/sseq/packed_ssrle_searchable_seq.hpp>
#include <memoria/core/ssrle/ssrle.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/iovector/io_substream_ssrle_symbol_sequence.hpp>

#include <bitset>
#include <iostream>

using namespace memoria;

size_t BitsPerSym(size_t syms) {
    return syms <= 2 ? 1 : Log2U(syms - 1);
}

int main(int, char**)
{
    //using Run1T = SSRLERun<1>;

//    for (size_t c = 1; c <= 256; c++ )
//    println("BitsPerSym({}) = {}", c, BitsPerSymbolConstexpr(c));

    UAcc128T aa(10);
    println("AccP: {}", aa);

    std::cout << "AccC:" << aa << std::endl;
}
