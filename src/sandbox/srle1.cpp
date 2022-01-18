
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

int main(int, char**)
{
    using Run1T = SSRLERun<1>;

    Run1T run1(32, std::bitset<32>("01100100100011011101001").to_ullong(), 50);
    Run1T run2(32, std::bitset<32>("01100100100011011101001").to_ullong(), 25);
    Run1T run3(32, std::bitset<32>("001011100011110111010011101").to_ullong(), 15);

    println("{}", run1);
    println("{}", run2);
    println("{}", run3);

    auto seq0 = io::make_packed_ssrle_symbol_sequence(1);
    auto& seq = io::substream_cast<io::PackedSSRLESymbolSequence<1>>(*seq0.get());

    seq.append(Span<const Run1T>{&run1, 1});
    seq.append(Span<const Run1T>{&run2, 1});
    for (size_t c = 0; c < 1000; c++) {
        //println("C: {}", c);

        Run1T run0 (run3.pattern_length(), run3.pattern() + c, run3.run_length());

        seq.append(Span<const Run1T>{&run0, 1});
    }

    seq.reindex();

    seq.dump(std::cout);
}
