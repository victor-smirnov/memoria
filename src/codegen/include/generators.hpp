
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

#include <memoria/core/types.hpp>

#include <memoria/core/strings/format.hpp>

#include <codegen.hpp>

namespace memoria::codegen {

struct AbstractGeneratorInstance {
    virtual ~AbstractGeneratorInstance() noexcept = default;
};

struct FileGeneratorInstance: AbstractGeneratorInstance {
    virtual void generate_files() = 0;
};


struct TypeFactoryGeneratorInstance: AbstractGeneratorInstance {
    virtual void dry_run(std::function<void(U8StringView)>) = 0;
    virtual void generate_init() = 0;
    virtual void generate_files() = 0;
};

struct GeneratorInstanceFactory {
    virtual ~GeneratorInstanceFactory() noexcept = default;

    virtual ShPtr<FileGeneratorInstance> create_file_generator(
            U8StringView name,
            ShPtr<FileGenerator> gen
    ) const = 0;


    virtual ShPtr<TypeFactoryGeneratorInstance> create_type_factory_generator(
            U8StringView name,
            ShPtr<TypeInstance> ins,
            ShPtr<TypeFactory> tf,
            Optional<U8String> profile
    ) const = 0;
};


ShPtr<FileGeneratorInstance> create_file_generator_instance(
    U8StringView name,
    ShPtr<FileGenerator> gen
);


ShPtr<TypeFactoryGeneratorInstance> create_type_factory_generator_instance(
    U8StringView name,
    ShPtr<TypeInstance> ins,
    ShPtr<TypeFactory> tf,
    Optional<U8String> profile
);


void register_generator_factory(U8StringView prefix, ShPtr<GeneratorInstanceFactory> factory);

}
