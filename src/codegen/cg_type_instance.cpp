
// Copyright 2021-2022 Victor Smirnov
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

#include <memoria/core/strings/format.hpp>

#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/result.hpp>
#include <memoria/core/memory/memory.hpp>

#include <codegen.hpp>
#include <codegen_ast_tools.hpp>
#include <generators.hpp>


#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>

#include <llvm/Support/raw_ostream.h>

#include <vector>
#include <filesystem>

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

class TypeInstanceImpl: public TypeInstance, public std::enable_shared_from_this<TypeInstanceImpl> {
  WeakPtr<Project> project_ptr_;
  Project* project_;
  ShPtr<TypeFactory> type_factory_;

  const clang::ClassTemplateSpecializationDecl* ctr_descr_;

  PoolSharedPtr<hermes::HermesCtr> config_;
  PoolSharedPtr<hermes::HermesCtr> project_config_;

  ShPtr<PreCompiledHeader> precompiled_header_;
  std::vector<U8String> includes_;

  U8String name_;

  U8String target_folder_;
  U8String target_file_;

  Optional<std::vector<U8String>> profiles_;

  U8String config_sdn_path_;

public:
  TypeInstanceImpl(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr) noexcept:
    project_ptr_(project), project_(project.get()), ctr_descr_(descr)
  {
    config_ = hermes::HermesCtr::make_new();
    project_config_ = hermes::HermesCtr::make_new();
  }

  U8String config_string(const U8String& sdn_path) const override {
    return get_value(config_->root(), sdn_path).as_varchar();
  }

  ShPtr<FileGenerator> initializer() override
  {
    U8String sdn_path = config_sdn_path_ + "/init";
    return project_->generator(sdn_path);
  }

  std::vector<U8String> includes() const override {
    return includes_;
  }

  U8String config_sdn_path() const override {
    return config_sdn_path_;
  }

  U8String target_folder() const override {
    return target_folder_;
  }

  U8String target_file(const U8String& profile) const override
  {
    if (profile != "") {
      return target_folder_ + "/" + name_ + "_" + get_profile_id(profile) + "_.cpp";
    }
    else {
      return target_folder_ + "/" + name_ + ".cpp";
    }
  }

  PoolSharedPtr<hermes::HermesCtr> config() const override {
    return config_;
  }

  ShPtr<Project> project() const noexcept override {
    return project_ptr_.lock();
  }

  const clang::ClassTemplateSpecializationDecl* ctr_descr() const override {
    return ctr_descr_;
  }

  U8String describe() const override {
    return describe_decl(ctr_descr_);
  }

  clang::QualType type() const override {
    return type_();
  }

  clang::QualType type_() const {
    return ctr_descr_->getTemplateArgs()[0].getAsType();
  }

  ShPtr<TypeFactory> type_factory() const override {
    return type_factory_;
  }

  void set_type_factory(ShPtr<TypeFactory> tf) override {
    type_factory_ = tf;
  }

  std::vector<U8String> full_includes() const override
  {
    std::vector<U8String> list;

    if (type_factory_)
    {
      auto ii = type_factory_->includes();
      list.insert(list.end(), ii.begin(), ii.end());
    }

    list.insert(list.end(), includes_.begin(), includes_.end());

    return list;
  }

  U8String generate_include_header() const
  {
    U8String code;
    for (const auto& include: includes_)
    {
      code += format_u8("#include <{}>\n", include);
    }

    return code;
  }


  void precompile_headers() override
  {
    if (type_factory_)
    {
      U8String code = generate_include_header();

      U8String header_name = name_ + ".hpp";
      write_text_file_if_different(target_folder_ + "/" + header_name, code);

      precompiled_header_ = PreCompiledHeader::create(
            type_factory_->precompiled_header(),
            target_folder_,
            header_name
            );
    }
  }

  void generate_artifacts() override
  {
    if (type_factory_)
    {
      U8String generator_class_name = type_factory()->generator();
      if (profiles_)
      {
        auto gen_ii = create_type_factory_generator_instance(
              generator_class_name,
              self(),
              type_factory_,
              Optional<U8String>{}
              );

        gen_ii->generate_init();

        for (const U8String& profile: profiles_.get())
        {
          if (project_->is_profile_enabled(profile))
          {
            auto gen = create_type_factory_generator_instance(
                  generator_class_name,
                  self(),
                  type_factory_,
                  profile
                  );

            gen->generate_files();
          }
        }
      }
      else {
        auto gen = create_type_factory_generator_instance(
              generator_class_name,
              self(),
              type_factory_,
              Optional<U8String>{}
              );

        gen->generate_files();
      }
    }
    else {
      println("TypeInstance for {} has no associated TypeFactory. Skipping.", type().getAsString());
    }
  }

  void dry_run(ObjectMap map) override
  {
    auto sources = get_or_add_array(map, "sources");
    auto byproducts = get_or_add_array(map, "byproducts");

    DefaultResourceNameConsumerImpl consumer(sources, byproducts);

    U8String file_path = target_folder_ + "/" + name_ + ".hpp";

    consumer.add_byproduct_file(file_path);
    consumer.add_byproduct_file(file_path + ".pch");

    U8String generator_class_name = type_factory()->generator();

    auto fn1 = [&](U8StringView str){
      consumer.add_source_file(U8String(str));
    };

    if (profiles_)
    {
      for (const U8String& profile: profiles_.get())
      {
        if (project_->is_profile_enabled(profile))
        {
          auto gen = create_type_factory_generator_instance(
                generator_class_name,
                self(),
                type_factory_,
                profile
                );

          gen->dry_run(fn1);
        }
      }
    }
    else {
      auto gen = create_type_factory_generator_instance(
            generator_class_name,
            self(),
            type_factory_,
            Optional<U8String>{}
            );

      gen->dry_run(fn1);
    }
  }

  Optional<std::vector<U8String>> profiles() const override {
    return profiles_;
  }

  ShPtr<TypeInstance> self() {
    return shared_from_this();
  }

  ObjectMap ld_config() const {
    return config_->root().as_typed_value().constructor().as_object_map();
  }

  U8String name() const override {
    return name_;
  }

  void configure() override
  {
    auto anns = get_annotations(ctr_descr_);
    if (anns.size()) {
      config_ = hermes::HermesCtr::parse_document(anns[anns.size() - 1]);

      auto name = ld_config().get("name");
      if (name.is_not_empty()) {
        name_ = name.as_varchar();
      }
      else {
        U8String type_name = type_().getAsString();
        name_ = get_profile_id(type_name);
      }

      auto includes = ld_config().get("includes");
      if (includes.is_not_empty()) {
        auto arr = includes.as_object_array();
        for (size_t c = 0; c < arr.size(); c++)
        {
          U8String file_name = arr.get(c).as_varchar();
          includes_.push_back(file_name);
        }
      }
      else {
        MEMORIA_MAKE_GENERIC_ERROR("Configuration attribute 'includes' must be specified for TypeInstance {}", type_().getAsString()).do_throw();
      }
    }
    else {
      MEMORIA_MAKE_GENERIC_ERROR("Configuration must be specified for TypeInstance {}", type_().getAsString()).do_throw();
    }

    auto cfg = config_->root();
    if (find_value(cfg, "$/config")) {
      config_sdn_path_ = cfg.as_varchar();
    }
    else {
      config_sdn_path_ = "$/groups/default/containers";
    }

    auto vv = project_->config()->root();
    if (find_value(vv, config_sdn_path_)) {
      project_config_ = vv.clone();
    }
    else {
      MEMORIA_MAKE_GENERIC_ERROR(
            "CodeGen Project configuration '{}' must be specified for TypeInstance {}",
            config_sdn_path_,
            type_().getAsString()
      ).do_throw();
    }

    target_folder_ = project_->components_output_folder() + "/" + get_value(project_config_->root(), "$/path")
        .as_varchar();

    std::error_code ec;
    if ((!std::filesystem::create_directories(target_folder_.to_std_string(), ec)) && ec) {
      MEMORIA_MAKE_GENERIC_ERROR("Can't create directory '{}': {}", target_folder_, ec.message()).do_throw();
    }

    auto pp = config_->root();
    if (find_value(pp, "$/profiles"))
    {
      if (pp.is_varchar())
      {
        U8String val = pp.as_varchar();
        if (val == "ALL") {
          profiles_ = project_->profiles();
        }
        else {
          MEMORIA_MAKE_GENERIC_ERROR("Invalid profile attribute '{}' value for TypeInstance for {}", type().getAsString()).do_throw();
        }
      }
      else if (pp.is_object_array())
      {
        auto arr = pp.as_object_array();
        profiles_ = std::vector<U8String>();
        for (size_t c = 0; c < arr.size(); c++) {
          profiles_.get().push_back(arr.get(c).as_varchar());
        }
      }
      else {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid profile attribute '{}' value for TypeInstance for {}", type().getAsString()).do_throw();
      }
    }
  }
};

ShPtr<TypeInstance> TypeInstance::create(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr) {
  return std::make_shared<TypeInstanceImpl>(project, descr);
}

}}
