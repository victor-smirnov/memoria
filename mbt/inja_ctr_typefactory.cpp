
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

#include <memoria/core/strings/format.hpp>

#include "inja_generators.hpp"

#include <inja/inja.hpp>

namespace memoria::codegen {


constexpr const char* TypeFactoryTemplate = R"(
{% for line in full_includes -%}
#include <{{line}}>
{% endfor %}
{% for line in profile_includes -%}
#include <{{line}}>
{% endfor %}

namespace memoria {
using Profile = {{profile}};
using CtrName = {{type_name}};

MMA_INSTANTIATE_CTR_{{factory_type}}(CtrName, Profile)
}
)";


class InjaTypeFactoryGeneratorInstance: public TypeFactoryGeneratorInstance {
  ShPtr<TypeInstance> type_ins_;
  ShPtr<TypeFactory> type_fctr_;
  U8String profile_;

  U8String data_type_;
  ShPtr<Project> project_;
  U8String target_file_;
  ShPtr<FileGenerator> init_gen_;

public:
  InjaTypeFactoryGeneratorInstance(ShPtr<TypeInstance> type_ins,
                                   ShPtr<TypeFactory> tf,
                                   Optional<U8String> profile):
    type_ins_(type_ins),
    type_fctr_(tf)
  {
    data_type_ = type_ins_->type().getAsString();
    profile_ = profile ? profile.value() : "";
    target_file_ = type_ins_->target_file(profile_);

    project_ = type_ins_->project();
    init_gen_ = project_->generator(type_ins_->config_sdn_path() + "/$/init");
  }


  void dry_run(std::function<void(U8StringView)> callback) {
    callback(target_file_);
  }

  void generate_init() {
    init_gen_->add_snippet("ctr_datatype_init_def", format_u8("register_ctr_type_reflection<{}>();", data_type_));

    for (auto ii: type_ins_->includes()) {
      init_gen_->add_snippet("ctr_includes", ii, true);
    }
  }

  void generate_files()
  {
    init_gen_->add_snippet("ctr_metadata_init", format_u8("InitCtrMetadata<{}, {}>();",
                                data_type_,
                                profile_
    ));

    nlohmann::json data;
    data["full_includes"] = type_ins_->full_includes();
    data["profile_includes"] = project_->profile_includes(profile_);
    data["profile"] = profile_;
    data["type_name"] = data_type_;
    data["factory_type"] = type_fctr_->type();

    write_text_file_if_different(
          target_file_, inja::render(TypeFactoryTemplate, data)
    );
  }

};


ShPtr<TypeFactoryGeneratorInstance> create_inja_type_factory_generator_instance(
    ShPtr<TypeInstance> ins,
    ShPtr<TypeFactory> tf,
    Optional<U8String> profile
)
{
  return std::make_shared<InjaTypeFactoryGeneratorInstance>(ins, tf, profile);
}


}
