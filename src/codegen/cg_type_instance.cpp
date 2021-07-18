
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

#include <codegen.hpp>
#include <codegen_ast_tools.hpp>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>

#include <llvm/Support/raw_ostream.h>

#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>

#include <vector>
#include <filesystem>

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

namespace py = pybind11;

class TypeInstanceImpl: public TypeInstance, public std::enable_shared_from_this<TypeInstanceImpl> {
    WeakPtr<Project> project_ptr_;
    Project* project_;
    ShPtr<TypeFactory> type_factory_;

    const clang::ClassTemplateSpecializationDecl* ctr_descr_;

    LDDocument config_;
    LDDocument project_config_;

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

    }

    U8String config_string(const U8String& sdn_path) const override {
        return get_value(config_.value(), sdn_path).as_varchar().view();
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

    LDDocumentView config() const override {
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
            auto path = split_path(generator_class_name);

            py::object cg = py::module_::import(path.first.data());
            py::object tf = cg.attr(path.second.data());

            if (profiles_)
            {
                py::object tf_ii = tf(self(), type_factory_, nullptr);
                py::object ii_init = tf_ii.attr("generate_init");
                if (!ii_init.is_none()) {
                    ii_init();
                }

                for (const U8String& profile: profiles_.get())
                {
                    py::object tf_i = tf(self(), type_factory_, py::str(profile.data()));
                    py::object hw = tf_i.attr("generate_files");

                    hw();
                }
            }
            else {
                py::object tf_i = tf(self(), type_factory_);
                py::object hw = tf_i.attr("generate_files");

                hw();
            }
        }
        else {
            println("TypeInstance for {} has no associated TypeFactory. Skipping.", type().getAsString());
        }
    }

    std::vector<U8String> generated_files() override
    {
        std::vector<U8String> files;

        U8String file_path = U8String("BYPRODUCT:") + target_folder_ + "/" + name_ + ".hpp";

        files.push_back(file_path);
        files.push_back(file_path + ".pch");

        std::function<void(std::string)> fn = [&](std::string str){
            files.push_back(str);
        };

        U8String generator_class_name = type_factory()->generator();
        auto path = split_path(generator_class_name);

        py::object cg = py::module_::import(path.first.data());
        py::object tf = cg.attr(path.second.data());

        if (profiles_)
        {
            for (const U8String& profile: profiles_.get())
            {
                py::object tf_i = tf(self(), type_factory_, py::str(profile.to_std_string()));
                py::object hw = tf_i.attr("dry_run");
                hw(fn);
            }
        }
        else {
            py::object tf_i = tf(self(), type_factory_, nullptr);
            py::object hw = tf_i.attr("dry_run");
            hw(fn);
        }

        return files;
    }

    Optional<std::vector<U8String>> profiles() const override {
        return profiles_;
    }

    ShPtr<TypeInstance> self() {
        return shared_from_this();
    }

    LDDMapView ld_config() const {
        return config_.value().as_typed_value().constructor().as_map();
    }

    U8String name() const override {
        return name_;
    }

    void configure() override
    {
        auto anns = get_annotations(ctr_descr_);
        if (anns.size()) {
            config_ = LDDocument::parse(anns[anns.size() - 1]);

            auto name = ld_config().get("name");
            if (name) {
                name_ = name.get().as_varchar().view();
            }
            else {
                U8String type_name = type_().getAsString();
                name_ = get_profile_id(type_name);
            }

            auto includes = ld_config().get("includes");
            if (includes) {
                LDDArrayView arr = includes.get().as_array();
                for (size_t c = 0; c < arr.size(); c++)
                {
                    U8String file_name = arr.get(c).as_varchar().view();
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

        auto cfg = config_.value();
        if (find_value(cfg, "$/config")) {
            config_sdn_path_ = cfg.as_varchar().view();
        }
        else {
            config_sdn_path_ = "$/groups/default/containers";
        }

        LDDValueView vv = project_->config().value();
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

        target_folder_ = project_->components_output_folder() + "/" + get_value(project_config_.value(), "$/path").as_varchar().view();

        std::error_code ec;
        if ((!std::filesystem::create_directories(target_folder_.to_std_string(), ec)) && ec) {
            MEMORIA_MAKE_GENERIC_ERROR("Can't create directory '{}': {}", target_folder_, ec.message()).do_throw();
        }

        auto pp = config_.value();
        if (find_value(pp, "$/profiles"))
        {
            if (pp.is_varchar())
            {
                U8String val = pp.as_varchar().view();
                if (val == "ALL") {
                    profiles_ = project_->profiles();
                }
                else {
                    MEMORIA_MAKE_GENERIC_ERROR("Invalid profile attribute '{}' value for TypeInstance for {}", type().getAsString()).do_throw();
                }
            }
            else if (pp.is_array())
            {
                auto arr = pp.as_array();
                profiles_ = std::vector<U8String>();
                for (size_t c = 0; c < arr.size(); c++) {
                    profiles_.get().push_back(arr.get(c).as_varchar().view());
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
