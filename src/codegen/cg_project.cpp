
// Copyright 2021 Victor Smirnov
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
#include <memoria/core/regexp/icu_regexp.hpp>

#include <codegen.hpp>
#include <codegen_ast_tools.hpp>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>

#include <llvm/Support/raw_ostream.h>

#include <vector>
#include <cctype>
#include <filesystem>

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

namespace py = pybind11;

class ProjectImpl;

class ConfigVisitor: public RecursiveASTVisitor<ConfigVisitor> {
    using Base = RecursiveASTVisitor<ConfigVisitor>;

    ProjectImpl& project_;

public:
    ConfigVisitor(ProjectImpl& project) noexcept : project_(project) {}
    bool shouldVisitTemplateInstantiations() const {
        return true;
    }

    bool VisitCXXRecordDecl(CXXRecordDecl* RD);
};



class ProjectImpl: public Project, public std::enable_shared_from_this<ProjectImpl> {

    U8String config_file_name_;
    U8String project_output_folder_;
    U8String components_output_folder_;

    ShPtr<CodeModule> config_unit_;
    ShPtr<PreCompiledHeader> precompiled_config_;

    std::vector<ShPtr<TypeInstance>> type_instances_;
    std::vector<ShPtr<TypeFactory>> type_factories_;
    std::unordered_map<U8String, ShPtr<FileGenerator>> file_generators_;

    LDDocument config_;

    friend class ConfigVisitor;

    U8String codegen_config_file_name_{"generated-codegen.hpp"};
    U8String codegen_config_file_name_pch_;

    std::vector<U8String> profiles_;

public:
    ProjectImpl(U8String config_file_name, U8String project_output_folder, U8String components_output_folder) noexcept:
        config_file_name_(std::move(config_file_name)),
        project_output_folder_(std::move(project_output_folder)),
        components_output_folder_(std::move(components_output_folder))
    {
    }

    U8String config_string(const U8String& sdn_path) const override {
        return get_value(config_.value(), sdn_path).as_varchar().view();
    }

    void parse_configuration() override
    {
        std::error_code ec;
        if ((!std::filesystem::create_directories(project_output_folder_.to_std_string(), ec)) && ec) {
            MEMORIA_MAKE_GENERIC_ERROR("Can't create directory '{}': {}", project_output_folder_, ec.message()).do_throw();
        }

        U8String tgt_header = project_output_folder_ + "/" + codegen_config_file_name_;

        write_text_file(tgt_header, format_u8("#include <{}>", config_file_name_));

        precompiled_config_ = PreCompiledHeader::create({"-std=c++20"}, project_output_folder_, codegen_config_file_name_);
        codegen_config_file_name_pch_ = precompiled_config_->file_path();

        config_unit_ = precompiled_config_->compile_with("");

        ConfigVisitor visitor(*this);
        visitor.TraverseAST(config_unit_->ast_unit().getASTContext());

        auto profiles = config_map().get("profiles");
        if (profiles)
        {
            auto map = profiles.get().as_map();
            map.for_each([&](auto key, auto){
                profiles_.push_back(key);
            });
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("No profiles are defined for this configuration").do_throw();
        }

        for_each_value(config_.value(), [&](const std::vector<U8String>& path, LDDValueView value) -> bool {
            if (value.is_typed_value())
            {
                LDDTypedValueView tvv = value.as_typed_value();
                if (tvv.type().name() == "FileGenerator")
                {
                    U8String full_path = join_sdn_path(path);
                    LDDocument cfg = value.clone();

                    println("Generator: {}", full_path);

                    file_generators_[full_path] = FileGenerator::create(self(), full_path, std::move(cfg));
                    return false;
                }
            }

            return true;
        });

        std::vector<Inference> types;

        for (const auto& type: type_instances_)
        {
            types.push_back(Inference::as_value(format_u8("memoria::TypeFactory<{0}>::ID", type->type().getAsString())));
        }

        auto values = precompiled_config_->infer(types);

        std::unordered_map<U8String, ShPtr<TypeFactory>> factories;
        for (auto& tf: type_factories_) {
            factories[tf->factory_id()] = tf;
        }

        for (size_t c = 0; c < values.size(); c++)
        {
            auto ii = factories.find(values[c]);
            if (ii != factories.end()) {
                type_instances_[c]->set_type_factory(ii->second);
            }
            else {
                println("Warning: TypeInstance<{}> has no associated TypeFactory!", type_instances_[c]->type().getAsString());
            }
        }

        for (auto& pair: file_generators_) {
            pair.second->configure();
        }

        for (auto& ptr: type_factories_) {
            ptr->configure();
        }

        for (auto& ptr: type_instances_) {
            ptr->configure();
        }
    }

    std::vector<ShPtr<TypeInstance>> type_instances() const override {
        return type_instances_;
    }

    std::vector<U8String> build_file_names() override
    {
        std::vector<U8String> files{};

        U8String file_path = U8String("BYPRODUCT:") + project_output_folder_ + "/" + config_file_name_;

        files.push_back(file_path);
        files.push_back(file_path + ".pch");

        for (auto& tf: type_factories_)
        {
            auto names = tf->generated_files();
            files.insert(files.end(), names.begin(), names.end());
        }

        for (auto& ti: type_instances_)
        {
            auto names = ti->generated_files();
            files.insert(files.end(), names.begin(), names.end());
        }

        for (auto& ti: file_generators_)
        {
            auto names = ti.second->generated_files();
            files.insert(files.end(), names.begin(), names.end());
        }

        return files;
    }


    std::vector<ShPtr<TypeFactory>> type_factories() const override {
        return type_factories_;
    }

    std::vector<ShPtr<FileGenerator>> file_generators() const override
    {
        std::vector<ShPtr<FileGenerator>> generators;

        for (const auto& pair: file_generators_) {
            generators.push_back(pair.second);
        }

        return generators;
    }

    U8String project_output_folder() const override {
        return project_output_folder_;
    }

    U8String components_output_folder() const override {
        return components_output_folder_;
    }

    void generate_artifacts() override
    {
        for (auto& ti: type_factories_) {
            ti->precompile_headers();
            ti->generate_artifacts();
        }

        for (auto& ti: type_instances_) {
            ti->precompile_headers();
            ti->generate_artifacts();
        }

        for (auto& pair: file_generators_) {
            pair.second->generate_artifacts();
        }
    }

    ShPtr<ProjectImpl> self() {
        return shared_from_this();
    }

    ShPtr<CodeModule> config_unit() const noexcept override {
        return config_unit_;
    }

    LDDocumentView config() const noexcept override {
        return config_.as_immutable_view();
    }

    LDDMapView config_map() const override {
        return config_.value().as_typed_value().constructor().as_map();
    }

    std::vector<U8String> profiles() const override {
        return profiles_;
    }

    ShPtr<FileGenerator> generator(const U8String& sdn_path) const override
    {
        auto ii = file_generators_.find(sdn_path);
        if (ii != file_generators_.end()) {
            return ii->second;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("FileGenerator for '{}' is not found", sdn_path).do_throw();
        }
    }

    std::vector<U8String> profile_includes(const U8String& profile) const override
    {
        U8String path = U8String("$/profiles/") + profile + "/includes";
        LDDArrayView ii = get_value(config_.value(), path).as_array();

        std::vector<U8String> incs;

        ii.for_each([&](auto value){
            incs.push_back(value.as_varchar().view());
        });

        return incs;
    }
};


ShPtr<Project> Project::create(U8String config_file_name, U8String project_output_folder, U8String components_output_folder) {
    return std::make_shared<ProjectImpl>(config_file_name, project_output_folder, components_output_folder);
}



bool ConfigVisitor::VisitCXXRecordDecl(CXXRecordDecl* RD)
{
    using CTSD = ClassTemplateSpecializationDecl;

    if (RD->getNameAsString() == "CodegenConfig") {
        for (const auto& ann: get_annotations(RD)) {
            project_.config_ = LDDocument::parse(ann);
        }
    }
    else if (RD->getNameAsString() == "TypeInstance") {
        if (isa<CTSD>(RD)){
            project_.type_instances_.emplace_back(
                TypeInstance::create(
                    project_.self(),
                    cast<CTSD>(RD)
                )
            );
        }
    }
    else if (RD->getNameAsString() == "TypeFactory") {
        if (isa<CTSD>(RD))
        {
            CTSD* SD = cast<CTSD>(RD);

            project_.type_factories_.emplace_back(
                TypeFactory::create(
                    project_.self(),
                    SD
                )
            );
        }
    }

    return true;
}


std::pair<U8String, U8String> split_path(U8String class_path)
{
    size_t pos = class_path.find_last_of('.', class_path.size() - 1);
    if (pos != U8String::NPOS) {
        size_t next_pos = pos + 1;
        return std::make_pair(class_path.substring(0, pos), class_path.substring(next_pos, class_path.size() - next_pos));
    }
    return std::make_pair(U8String(), class_path);
}

U8String get_profile_id(U8String profile_name)
{
    U8String str;

    auto pattern = ICURegexPattern::compile(u"[^a-zA-Z0-9]+");
    auto tokens = pattern.split(profile_name);

    for (auto& token: tokens) {
        if (!(token == "class" || token == "struct" || token == "union" || token == "memoria")) {
            str += token;
        }
    }

    return str;
}

U8String join_sdn_path(Span<const U8String> tokens)
{
    U8String tmp;
    for (size_t c = 0; c < tokens.size(); c++) {
        tmp += tokens[c];
        if (c < tokens.size() - 1) {
            tmp += "/";
        }
    }

    return tmp;
}

U8String get_same_level_path(U8String path, U8String step)
{
    auto tokens = parse_path_expression(path);
    if (tokens.size() > 0)
    {
        tokens.erase(tokens.end() - 1);
        tokens.push_back(step);

        return join_sdn_path(tokens);
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Invalid path expression: '{}'", path).do_throw();
    }
}

void for_each_value(std::vector<U8String>& path, LDDValueView elem, const std::function<bool (const std::vector<U8String>&, LDDValueView)>& consumer)
{
    if (consumer(path, elem))
    {
        if (elem.is_typed_value()) {
            path.push_back("$");
            LDDValueView next = elem.as_typed_value().constructor();
            for_each_value(path, next, consumer);
            path.pop_back();
        }
        else if (elem.is_map()) {
            LDDMapView map = elem.as_map();
            map.for_each([&](auto key, auto value) {
                path.push_back(key);
                for_each_value(path, value, consumer);
                path.pop_back();
            });
        }
    }
}

void for_each_value(LDDValueView elem, const std::function<bool(const std::vector<U8String>&, LDDValueView)>& consumer)
{
    std::vector<U8String> path;
    for_each_value(path, elem, consumer);
}

}}
