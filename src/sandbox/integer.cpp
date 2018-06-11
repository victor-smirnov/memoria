
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

#include <memoria/v1/core/tools/type_name.hpp>
#include <memoria/v1/core/integer/integer.hpp>

#include <iostream>
#include <type_traits>

using namespace memoria::v1;

namespace mp = boost::multiprecision;

int main()
{
    using UAcc = UnsignedAccumulator<256>;
    bmp::uint256_t tt{0};

    UAcc aa{"12345678901234567890123456789012345678901234567890"};

	std::cout << aa << std::endl;

    return 0;
}
