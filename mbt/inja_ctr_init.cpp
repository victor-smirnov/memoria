
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
{% for line in global_includes -%}
#include <{{line}}>
{% endfor %}

{% for line in ctr_includes -%}
#include <{{line}}>
{% endfor %}


{% for incl in profile_includes -%}
#include <{{incl}}>
{% endfor %}


namespace memoria {

{% for dt_init in ctr_datatype_init_decl -%}
    {{dt_init}}
{% endfor %}


void {{function_name}}() {
{% for dt_init in ctr_datatype_init_def -%}
    {{dt_init}}
{% endfor %}

{% for meta_init in ctr_metadata_init -%}
    {{meta_init}}
{% endfor %}
}

})";


class InjaCtrInitGeneratorInstance: public FileGeneratorInstance {
  ShPtr<FileGenerator> gen_;

public:
  InjaCtrInitGeneratorInstance(ShPtr<FileGenerator> gen):
    gen_(gen)
  {}

  void generate_files()
  {
    nlohmann::json data;

    std::vector<U8String> profile_includes;

    for (auto profile: gen_->project()->profiles()) {
      auto incs = gen_->project()->profile_includes(profile);
      profile_includes.insert(profile_includes.end(), incs.begin(), incs.end());
    }

    data["profile_includes"] = profile_includes;

    data["global_includes"] = gen_->includes();

    data["ctr_metadata_init"] = gen_->snippets("ctr_metadata_init");
    data["ctr_datatype_init_def"] = gen_->snippets("ctr_datatype_init_def");
    data["ctr_datatype_init_decl"] = gen_->snippets("ctr_datatype_init_decl");
    data["ctr_includes"] = gen_->snippets("ctr_includes");
    data["function_name"] = gen_->config_string("$/function");

    U8String text = inja::render(CtrInitTemplate, data);

    write_text_file_if_different(
          gen_->target_file(), text);
  }
};

ShPtr<FileGeneratorInstance> create_inja_ctr_init_file_generator_instance(
    ShPtr<FileGenerator> gen
) {
  return std::make_shared<InjaCtrInitGeneratorInstance>(gen);
}


}
