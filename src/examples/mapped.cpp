
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/linked/linked.hpp>

#include <memoria/core/strings/u8_string.hpp>

#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/core/datatypes/buffer/buffer_generic.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

using namespace memoria;

int main()
{
    DataTypeBuffer<Varchar> buffer;

    LifetimeGuard gg = buffer.data_guard();

    for (int c = 0; c < 1000; c++) {
        buffer.append(std::string("cool string :: :: :: -- ") + std::to_string(c));
        std::cout << c << " -- " << gg.is_valid() << std::endl;

        if (!gg.is_valid()) {
            gg = buffer.data_guard();
        }
    }

    return 0;
}
