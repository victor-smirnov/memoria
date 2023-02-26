
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

#include <generators.hpp>

#include <memoria/core/tools/result.hpp>

#include <unordered_map>


namespace memoria::codegen {

namespace {

std::unordered_map<U8String, ShPtr<GeneratorInstanceFactory>> factories;

}

ShPtr<FileGeneratorInstance> create_file_generator_instance(
    U8StringView generator_class_name,
    ShPtr<FileGenerator> gen
) {
  auto path = split_path(generator_class_name);

  auto ii = factories.find(path.first);
  if (ii != factories.end())
  {
    return ii->second->create_file_generator(path.second, gen);
  }
  else {
    MEMORIA_MAKE_GENERIC_ERROR("Generator factory '{}' is not found.", path.first).do_throw();
  }
}


ShPtr<TypeFactoryGeneratorInstance> create_type_factory_generator_instance(
    U8StringView generator_class_name,
    ShPtr<TypeInstance> ins,
    ShPtr<TypeFactory> tf,
    Optional<U8String> profile
) {
  auto path = split_path(generator_class_name);

  auto ii = factories.find(path.first);
  if (ii != factories.end())
  {
    return ii->second->create_type_factory_generator(path.second, ins, tf, profile);
  }
  else {
    MEMORIA_MAKE_GENERIC_ERROR("Generator factory '{}' is not found.", path.first).do_throw();
  }
}


void register_generator_factory(U8StringView prefix, ShPtr<GeneratorInstanceFactory> factory) {
  factories[prefix] = factory;
}

}
