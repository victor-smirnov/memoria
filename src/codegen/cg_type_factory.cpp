
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

#include <llvm/Support/raw_ostream.h>

#include <vector>

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

namespace py = pybind11;

class TypeFactoryImpl: public TypeFactory, public std::enable_shared_from_this<TypeFactoryImpl> {
    WeakPtr<Project> project_ptr_;
    Project* project_;
    const clang::ClassTemplateSpecializationDecl* tf_decl_;

    U8String id_;
    U8String name_;
    LDDocument config_;

    ShPtr<PreCompiledHeader> precompiled_header_;
    std::vector<U8String> includes_;

public:
    TypeFactoryImpl(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr) noexcept:
        project_ptr_(project), project_(project.get()), tf_decl_(descr)
    {
        auto anns = get_annotations(tf_decl_);
        if (anns.size()) {
            config_ = LDDocument::parse(anns[anns.size() - 1]);
        }

        for (auto dd: tf_decl_->decls())
        {
            if (clang::VarDecl* vd = clang::dyn_cast_or_null<clang::VarDecl>(dd)) {
                if (vd->getNameAsString() == "ID")
                {
                    clang::APValue* val = vd->getEvaluatedValue();
                    if (!val) {
                        val = vd->evaluateValue();
                    }

                    APSInt ii = val->getInt();
                    id_ = ii.toString(10);
                }
            }
        }

    }

    U8String name() const override {
        return name_;
    }

    void precompile_headers() override
    {
        U8String code;

        for (const auto& include: includes_)
        {
            code += format_u8("#include <{}>\n", include);
        }

        U8String target_folder = project()->project_output_folder();

        U8String header_name = name_ + ".hpp";
        write_text_file_if_different(target_folder + "/" + header_name, code);

        precompiled_header_ = PreCompiledHeader::create({"-std=c++20"}, target_folder, header_name);
    }

    std::vector<U8String> includes() const override {
        return includes_;
    }

    ShPtr<PreCompiledHeader> precompiled_header() const override {
        return precompiled_header_;
    }


    U8String type_pattern() const override
    {
        const auto& args = tf_decl_->getTemplateArgs();

        if (args.size() == 1)
        {
            std::string ss;
            llvm::raw_string_ostream os(ss);
            args.get(0).print(clang::PrintingPolicy{clang::LangOptions{}}, os);
            return U8String{ss};
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid number of template arguments for TypeFactory: {}", describe()).do_throw();
        }
    }

    LDDocumentView config() const override {
        return config_;
    }

    ShPtr<Project> project() const noexcept override {
        return project_ptr_.lock();
    }

    const clang::ClassTemplateSpecializationDecl* factory_descr() const override {
        return tf_decl_;
    }

    U8String describe() const override {
        return describe_decl(tf_decl_);
    }

    U8String factory_id() const override {
        return id_;
    }

    U8String generator() const override
    {
        auto gen = ld_config().get("generator");
        if (gen) {
            return gen.get().as_varchar().view();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("TypeFactory for {} has no generator attribute defined. Skipping: ", type_pattern()).do_throw();
        }
    }

    ShPtr<TypeFactory> self() {
        return shared_from_this();
    }

    LDDMapView ld_config() {
        return config_.value().as_typed_value().constructor().as_map();
    }

    LDDMapView ld_config() const {
        return config_.value().as_typed_value().constructor().as_map();
    }

    void generate_artifacts() override {}

    std::vector<U8String> generated_files() override
    {
        std::vector<U8String> files;

        U8String file_path = U8String("BYPRODUCT:") + project()->project_output_folder() + "/" + name_ + ".hpp";

        files.push_back(file_path);
        files.push_back(file_path + ".pch");

        return files;
    }

    void configure() override
    {
        auto includes = ld_config().get("includes");
        if (includes)
        {
            LDDArrayView arr = includes.get().as_array();
            for (size_t c = 0; c < arr.size(); c++)
            {
                U8String file_name = arr.get(c).as_varchar().view();
                includes_.push_back(file_name);
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("TypeFactory for {} must define 'includes' config attribute", type_pattern()).do_throw();
        }

        auto name = ld_config().get("name");
        if (name)
        {
            name_ = name.get().as_varchar().view();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("TypeFactory for {} must define 'name' config attribute", type_pattern()).do_throw();
        }
    }
};

ShPtr<TypeFactory> TypeFactory::create(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr) {
    return std::make_shared<TypeFactoryImpl>(project, descr);
}

}}
