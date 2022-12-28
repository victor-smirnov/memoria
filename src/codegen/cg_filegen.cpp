
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

#include <codegen.hpp>
#include <codegen_ast_tools.hpp>
#include <generators.hpp>

#include <llvm/Support/raw_ostream.h>

#include <vector>
#include <unordered_map>
#include <filesystem>

namespace memoria::codegen {

using namespace clang;
using namespace llvm;

class FileGeneratorImpl: public FileGenerator, public std::enable_shared_from_this<FileGeneratorImpl> {
  WeakPtr<Project> project_ptr_;
  Project* project_;

  U8String sdn_path_;
  HermesCtr config_;

  std::unordered_map<U8String, std::vector<U8String>> snippets_;

  std::vector<U8String> includes_;

  U8String target_folder_;
  U8String target_file_;

  U8String handler_;

public:
  FileGeneratorImpl(
          ShPtr<Project> project,
          U8String sdn_path,
          hermes::HermesCtr&& config
  ):
    project_ptr_(project), project_(project.get()), sdn_path_(sdn_path), config_(std::move(config))
  {

  }

  U8String target_file() const override {
    return target_file_;
  }

  U8String target_folder() const override {
    return target_folder_;
  }

  ShPtr<Project> project() const noexcept override {
    return project_ptr_.lock();
  }

  U8String describe() const override {
    return format_u8("FileGenerator for: {}", config_.to_pretty_string());
  }

  void add_snippet(const U8String& collection, const U8String& text, bool distinct) override
  {
    if (distinct && snippets_.find(collection) != snippets_.end()){
      for (const auto& str: snippets_[collection]){
        if (str == text) {
          return;
        }
      }
    }

    snippets_[collection].push_back(text);
  }


  void generate_artifacts() override
  {
    auto gen = create_file_generator_instance(handler_, self());
    gen->generate_files();
  }

  void dry_run(hermes::ObjectMap map) override
  {
    auto sources = get_or_add_array(map, "sources");

    U8String file_path = target_file_;
    sources = sources.push_back_t<Varchar>(file_path);
  }

  virtual std::vector<U8String> includes() const override {
    return includes_;
  }

  virtual void configure() override
  {
    auto ii = ld_config().get("includes");
    if (ii.is_not_null()) {
      auto arr = ii.as_object_array();
      for (size_t c = 0; c < arr.size(); c++)
      {
        includes_.push_back(arr.get(c).as_varchar());
      }
    }

    U8String path = get_or_fail(
          ld_config().get("filename"),
          "filename property is not specified for file generator"
        ).as_varchar();
    target_file_ = project_->components_output_folder() + "/" + path;

    std::filesystem::path pp0(target_file_.to_std_string());
    target_folder_ = pp0.parent_path().string();

    std::error_code ec;
    std::filesystem::create_directories(pp0.parent_path(), ec);
    if (ec) {
      MEMORIA_MAKE_GENERIC_ERROR("Can't create folder '{}'", target_folder_).do_throw();
    }

    handler_ = get_or_fail(
          ld_config().get("handler"),
          "handler property is not specified for file generator"
        ).as_varchar();
  }

  hermes::ObjectMap ld_config() const {
    return config_.root().cast_to<hermes::TypedValue>().constructor().as_object_map();
  }

  std::vector<U8String> snippets(const U8String& collection) const override
  {
    auto ii = snippets_.find(collection);
    if (ii != snippets_.end()) {
      return ii->second;
    }
    else {
      return std::vector<U8String>{};
    }
  }

  U8String config_string(const U8String& sdn_path) const override {
    return get_value(config_.root(), sdn_path).as_varchar();
  }

  ShPtr<FileGenerator> self() {
    return shared_from_this();
  }
};

ShPtr<FileGenerator> FileGenerator::create(
        ShPtr<Project> project,
        const U8String& sdn_path,
        HermesCtr config
){
  return std::make_shared<FileGeneratorImpl>(project, sdn_path, std::move(config));
}

}
