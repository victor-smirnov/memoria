
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

#include "inja_generators.hpp"

#include <inja/inja.hpp>

#include <unordered_map>

namespace memoria::codegen {

class InjaGeneratorsFactory: public GeneratorInstanceFactory {

  using FileGeneratorInstanceFn = std::function<ShPtr<FileGeneratorInstance>(
    ShPtr<FileGenerator>
  )>;

  using TypeFactoryGeneratorInstanceFn = std::function<ShPtr<TypeFactoryGeneratorInstance>(
    ShPtr<TypeInstance>,
    ShPtr<TypeFactory>,
    Optional<U8String>
  )>;

  std::unordered_map<U8String, FileGeneratorInstanceFn> file_generators_;
  std::unordered_map<U8String, TypeFactoryGeneratorInstanceFn> tf_generators_;

public:

  virtual ShPtr<FileGeneratorInstance> create_file_generator(
          U8StringView name,
          ShPtr<FileGenerator> gen
  ) const {
    auto ii = file_generators_.find(name);
    if (ii != file_generators_.end())
    {
      return ii->second(gen);
    }
    else {
      MEMORIA_MAKE_GENERIC_ERROR("File generator '{}' is not found.", name).do_throw();
    }
  }


  virtual ShPtr<TypeFactoryGeneratorInstance> create_type_factory_generator(
          U8StringView name,
          ShPtr<TypeInstance> ins,
          ShPtr<TypeFactory> tf,
          Optional<U8String> profile
  ) const {
    auto ii = tf_generators_.find(name);
    if (ii != tf_generators_.end())
    {
      return ii->second(ins, tf, profile);
    }
    else {
      MEMORIA_MAKE_GENERIC_ERROR("Type factory '{}' is not found.", name).do_throw();
    }
  }

  void add_file_generator(U8StringView name, FileGeneratorInstanceFn fn) {
    file_generators_[name] = fn;
  }

  void add_tf_generator(U8StringView name, TypeFactoryGeneratorInstanceFn fn) {
    tf_generators_[name] = fn;
  }
};


void register_inja_generator_factories() {
  auto factory = std::make_shared<InjaGeneratorsFactory>();

  factory->add_file_generator("CtrInitGenerator", [](auto file_gen){
    return create_inja_ctr_init_file_generator_instance(file_gen);
  });

  factory->add_file_generator("DatatypeInitGenerator", [](auto file_gen){
    return create_inja_datatype_file_generator_instance(file_gen);
  });

  factory->add_tf_generator("CtrTypeFactory", [](auto type_instance, auto type_factory, auto profile){
    return create_inja_type_factory_generator_instance(type_instance, type_factory, profile);
  });

  register_generator_factory("codegen.ctr_init", factory);
  register_generator_factory("codegen.ctr_typefactory", factory);
  register_generator_factory("codegen.datatype_init", factory);


}

}
