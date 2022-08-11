
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


#include <memoria/core/linked/document/linked_document.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/core/datatypes/arena/arena.hpp>
#include <memoria/core/datatypes/arena/vector.hpp>
#include <memoria/core/datatypes/arena/traits.hpp>

#include <inja/inja.hpp>

using namespace memoria;

int main(int, char**)
{
    nlohmann::json data;
    data["name"] = "world";

    std::cout << data << std::endl;

    inja::render_to(std::cout, "Hello {{ name }}!", data);

    std::cout << std::endl;

    arena::ArenaSegmentImpl arena(1024);
    size_t ptr0 = arena.allocate_space(8, 1, 0);
    println("ptr0: {}", ptr0);

    using Vc = arena::Vector<int>;

    arena::SegmentPtr<Vc> vv = allocate<Vc>(&arena);
    println("{}", vv.offset());

    Vc* vc = vv.write(&arena);

    vc->push_back(&arena, 10);

    println("size: {}, tag: {}", vc->size(), arena.read_type_tag(vv.offset()));

    for (auto ii: vc->span(&arena)) {
        println("ii: {}", ii);
    }

    arena::ArenaTypeMeta* mm;
}
