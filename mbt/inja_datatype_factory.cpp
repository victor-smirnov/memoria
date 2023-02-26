
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

namespace memoria::codegen {


constexpr const char* CtrInitTemplate = R"(
{% for line in full_includes -%}
#include <{{line}}>
{% endfor %}
)";


class InjaDatatypeGeneratorInstance: public FileGeneratorInstance {
  ShPtr<FileGenerator> gen_;

public:
  InjaDatatypeGeneratorInstance(ShPtr<FileGenerator> gen):
    gen_(gen)
  {}

  void generate_files()
  {
    nlohmann::json data;

    data["full_includes"] = gen_->includes();

    U8String text = inja::render(CtrInitTemplate, data);

    write_text_file_if_different(
          gen_->target_file(), text);
  }
};

ShPtr<FileGeneratorInstance> create_inja_datatype_file_generator_instance(
    ShPtr<FileGenerator> gen
) {
  return std::make_shared<InjaDatatypeGeneratorInstance>(gen);
}


}
