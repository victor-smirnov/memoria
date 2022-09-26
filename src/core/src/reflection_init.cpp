
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

#include <memoria/core/reflection/reflection.hpp>

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/hermes/reflection.hpp>

#include <memoria/core/datatypes/reflection.hpp>


namespace memoria {

using namespace hermes;

void InitTypeReflections() {
    register_type_reflection(std::make_unique<HermesTypeReflectionImpl<Array<Value>>>());
    register_type_reflection(std::make_unique<HermesTypeReflectionImpl<Map<Varchar, Value>>>());
    register_type_reflection(std::make_unique<HermesTypeReflectionImpl<Datatype<Varchar>>>());
}

}
