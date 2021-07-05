
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

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

namespace py = pybind11;

class ProjectImpl;

namespace {

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

}

class ProjectImpl: public Project, public std::enable_shared_from_this<ProjectImpl> {

    U8String config_file_name_;
    U8String output_folder_;

    ShPtr<CodeModule> config_unit_;
    ShPtr<PreCompiledHeader> precompiled_config_;

    std::vector<ShPtr<TypeInstance>> type_instances_;
    std::vector<ShPtr<TypeFactory>> type_factories_;
    std::vector<ShPtr<FileGenerator>> file_generators_;

    LDDocument config_;

    friend class ConfigVisitor;

    U8String codegen_config_file_name_{"codegen.hpp"};
    U8String codegen_config_file_name_pch_;

public:
    ProjectImpl(U8String config_file_name, U8String output_folder) noexcept:
        config_file_name_(std::move(config_file_name)),
        output_folder_(std::move(output_folder))
    {
    }

    void parse_configuration() override
    {
        U8String tgt_header = output_folder_ + "/" + codegen_config_file_name_;

        write_text_file(tgt_header, format_u8("#include <{}>", config_file_name_));

        precompiled_config_ = PreCompiledHeader::create({"-std=c++17"}, output_folder_, codegen_config_file_name_);
        codegen_config_file_name_pch_ = precompiled_config_->file_path();

        config_unit_ = precompiled_config_->compile_with("");

        ConfigVisitor visitor(*this);
        visitor.TraverseAST(config_unit_->ast_unit().getASTContext());

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
                println("Warning: TypeInstance<{}> has not associated TypeFactory!", type_instances_[c]->type().getAsString());
            }
        }
    }

    std::vector<ShPtr<TypeInstance>> type_instances() const override {
        return type_instances_;
    }

    std::vector<U8String> build_file_names() override
    {
        std::vector<U8String> files{};

        U8String file_path = U8String("BYPRODUCT:") + output_folder_ + "/" + config_file_name_;

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

        return files;
    }


    std::vector<ShPtr<TypeFactory>> type_factories() const override {
        return type_factories_;
    }

    std::vector<ShPtr<FileGenerator>> file_generators() const override {
        return file_generators_;
    }

    U8String target_folder() const override {
        return output_folder_;
    }

    void generate_artifacts() override
    {
        for (auto& ti: type_factories_) {
            ti->precompile_headers();
        }

        for (auto& ti: type_instances_) {
            ti->precompile_headers();
        }

        for (auto& ti: type_instances_) {
            ti->generate_artifacts();
        }
    }

    ShPtr<ProjectImpl> self() {
        return shared_from_this();
    }

    ShPtr<CodeModule> config_unit() const noexcept override {
        return config_unit_;
    }

    LDDocumentView config() const noexcept override {
        return config_;
    }
};


ShPtr<Project> Project::create(U8String config_file_name, U8String output_folder) {
    return std::make_shared<ProjectImpl>(config_file_name, output_folder);
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


}}
