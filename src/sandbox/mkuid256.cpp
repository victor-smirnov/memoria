
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

#include <memoria/core/tools/uid_256.hpp>


using namespace memoria;
using namespace memoria::hermes;

int main(int, char**)
{
    UID256 uid = UID256::make_random();
    println("{}", uid);
    println("{{{}ull, {}ull, {}ull, {}ull}}", uid.atom(0), uid.atom(1), uid.atom(2), uid.atom(3));

    return 0;
}
