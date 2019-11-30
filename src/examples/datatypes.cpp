
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


#include <memoria/v1/profiles/default/default.hpp>
#include <memoria/v1/api/datatypes/datatypes.hpp>

#include <memoria/v1/api/datatypes/varchars/varchar_builder.hpp>
#include <memoria/v1/api/datatypes/buffer/buffer.hpp>

#include <iostream>

using namespace memoria::v1;



int main()
{
    DataTypeBuffer<LDDocument> buffer;
    LDDocument::parse("{'aaaa': 'bbbbb'}").dump(std::cout) << std::endl;

//    std::cout << LDDocument::is_identifier("{}") << std::endl;

    return 0;
}
