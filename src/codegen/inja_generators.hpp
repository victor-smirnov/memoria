
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

#pragma once

#include <generators.hpp>
#include <code_module.hpp>

#include <vector>

namespace memoria::codegen {

void register_inja_generator_factories();

ShPtr<FileGeneratorInstance> create_inja_ctr_init_file_generator_instance(
    ShPtr<FileGenerator> gen
);


ShPtr<FileGeneratorInstance> create_inja_datatype_file_generator_instance(
    ShPtr<FileGenerator> gen
);

ShPtr<TypeFactoryGeneratorInstance> create_inja_type_factory_generator_instance(
    ShPtr<TypeInstance> ins,
    ShPtr<TypeFactory> tf,
    Optional<U8String> profile
);

}
