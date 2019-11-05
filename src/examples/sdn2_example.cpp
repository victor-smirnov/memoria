
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

#include <memoria/v1/core/linked/document/linked_document.hpp>

#include <memoria/v1/core/tools/uuid.hpp>

using namespace memoria::v1;

int main()
{
    LDDocument doc = LDDocument::parse(
        "#{MyDouble_7_3ааа: Double(7, 3)} {'Кл\\'юч0': '132493258.4' @#MyDouble_7_3ааа, 'b':1, 'c':2}"
    );

    doc.dump(std::cout) << std::endl;

    return 0;
}
