
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
#include <memoria/core/memory/memory.hpp>

#include <codegen.hpp>
#include <codegen_ast_tools.hpp>

#include <llvm/Support/raw_ostream.h>
#include <vector>

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

class TypeFactoryImpl: public TypeFactory, public std::enable_shared_from_this<TypeFactoryImpl> {
    WeakPtr<Project> project_ptr_;
    Project* project_;
    const clang::ClassTemplateSpecializationDecl* tf_decl_;

    U8String id_;
    U8String name_;
    hermes::HermesCtr config_;

    ShPtr<PreCompiledHeader> precompiled_header_;
    std::vector<U8String> includes_;

public:
    TypeFactoryImpl(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr) noexcept:
        project_ptr_(project), project_(project.get()), tf_decl_(descr)
    {
        auto anns = get_annotations(tf_decl_);
        if (anns.size()) {
            config_ = hermes::HermesCtrView::parse_document(anns[anns.size() - 1]);
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
                    SmallString<16> str;
                    ii.toString(str, 10);
                    id_ = str.str().str();
                }
            }
        }

    }

    U8String config_string(const U8String& sdn_path) const override {
        return get_value(config_.root(), sdn_path).as_varchar();
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

        precompiled_header_ = PreCompiledHeader::create({"-std=c++20", "-stdlib=libc++"}, target_folder, header_name);
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
            args.get(0).print(clang::PrintingPolicy{clang::LangOptions{}}, os, true);
            return U8String{ss};
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid number of template arguments for TypeFactory: {}", describe()).do_throw();
        }
    }

    hermes::HermesCtr config() const override {
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
        if (gen.is_not_empty()) {
            return gen.as_varchar();
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("TypeFactory for {} has no generator attribute defined. Skipping: ", type_pattern()).do_throw();
        }
    }

    virtual U8String type() const override {
        return get_value(config_.root(), "$/type").as_varchar();
    }

    ShPtr<TypeFactory> self() {
        return shared_from_this();
    }

    ObjectMap ld_config() {
        return config_.root().as_typed_value().constructor().as_object_map();
    }

    ObjectMap ld_config() const {
        return config_.root().as_typed_value().constructor().as_object_map();
    }

    void generate_artifacts() override {}

    void dry_run(ObjectMap map) override
    {
        auto sources = get_or_add_array(map, "sources");
        auto byproducts = get_or_add_array(map, "byproducts");

        DefaultResourceNameConsumerImpl consumer(sources, byproducts);

        U8String file_path = project()->project_output_folder() + "/" + name_ + ".hpp";

        consumer.add_byproduct_file(file_path);
        consumer.add_byproduct_file(file_path + ".pch");
    }



    void configure() override
    {
        auto includes = ld_config().get("includes");
        if (includes.is_not_empty())
        {
            auto arr = includes.as_object_array();
            for (size_t c = 0; c < arr.size(); c++)
            {
                U8String file_name = arr.get(c).as_varchar();
                includes_.push_back(file_name);
            }
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("TypeFactory for {} must define 'includes' config attribute", type_pattern()).do_throw();
        }

        auto name = ld_config().get("name");
        if (name.is_not_empty())
        {
            name_ = name.as_varchar();
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
