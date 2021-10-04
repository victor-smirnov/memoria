
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

#pragma once

#include <memoria/core/tools/result.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/tools/optional.hpp>

#include <code_module.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <pybind11/embed.h>

#include <boost/variant2/variant.hpp>

#include <clang/AST/Type.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/PrettyPrinter.h>

#include <memory>
#include <typeinfo>

namespace llvm {
class APSInt;
}

namespace memoria {
namespace codegen {

struct CodegenEntity;
struct Project;
struct CodeModule;
struct TypeInstance;
struct TypeFactory;
struct FileGenerator;


struct CodegenEntity {
    virtual ~CodegenEntity() noexcept = default;
    virtual U8String describe() const = 0;
    virtual ShPtr<Project> project() const noexcept = 0;

    virtual std::vector<U8String> includes() const = 0;

    virtual void dry_run(LDDMapView consumer) = 0;

    virtual void generate_artifacts() = 0;
    virtual void configure() = 0;

    virtual U8String config_string(const U8String& sdn_path) const = 0;
};


struct Project {
    virtual ~Project() noexcept = default;

    virtual void parse_configuration() = 0;

    virtual ShPtr<CodeModule> config_unit() const noexcept = 0;
    virtual LDDocumentView config() const noexcept = 0;
    virtual LDDMapView config_map() const = 0;
    virtual U8String project_output_folder() const = 0;
    virtual U8String components_output_folder() const = 0;
    virtual U8String config_string(const U8String& sdn_path) const = 0;

    virtual std::vector<ShPtr<TypeInstance>> type_instances() const = 0;
    virtual std::vector<ShPtr<TypeFactory>> type_factories() const = 0;
    virtual std::vector<ShPtr<FileGenerator>> file_generators() const = 0;

    virtual std::vector<U8String> profiles() const = 0;

    virtual LDDocument dry_run() = 0;

    virtual void generate_artifacts() = 0;

    virtual ShPtr<FileGenerator> generator(const U8String& sdn_path) const = 0;

    virtual std::vector<U8String> profile_includes(const U8String& profile) const = 0;
    virtual bool is_profile_enabled(const U8String& profile) const = 0;

    virtual void add_enabled_profile(const U8String& profile_name) = 0;
    virtual void add_disabled_profile(const U8String& profile_name) = 0;

    static std::shared_ptr<Project> create(std::vector<U8String> config_file_names, U8String project_output_folder, U8String components_output_folder);
};

struct TypeInstance: CodegenEntity {
    virtual ~TypeInstance() noexcept = default;

    virtual const clang::ClassTemplateSpecializationDecl* ctr_descr() const = 0;
    virtual clang::QualType type() const = 0;
    virtual LDDocumentView config() const = 0;
    virtual U8String name() const = 0;
    virtual U8String target_folder() const = 0;
    virtual U8String target_file(const U8String& profile) const = 0;

    virtual U8String config_sdn_path() const = 0;

    virtual ShPtr<TypeFactory> type_factory() const = 0;
    virtual void set_type_factory(ShPtr<TypeFactory> tf) = 0;

    virtual void precompile_headers() = 0;
    virtual Optional<std::vector<U8String>> profiles() const = 0;

    virtual ShPtr<FileGenerator> initializer() = 0;

    virtual std::vector<U8String> full_includes() const = 0;

    static ShPtr<TypeInstance> create(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr);
};

struct TypeFactory: CodegenEntity {
    virtual ~TypeFactory() noexcept = default;

    virtual U8String name() const = 0;

    virtual U8String factory_id() const = 0;
    virtual LDDocumentView config() const = 0;
    virtual U8String type_pattern() const = 0;

    virtual void precompile_headers() = 0;
    virtual ShPtr<PreCompiledHeader> precompiled_header() const = 0;

    virtual U8String generator() const = 0;
    virtual U8String type() const = 0;


    virtual const clang::ClassTemplateSpecializationDecl* factory_descr() const = 0;

    static ShPtr<TypeFactory> create(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr);
};



struct FileGenerator: CodegenEntity {
    virtual ~FileGenerator() noexcept = default;

    virtual void add_snippet(const U8String& collection, const U8String& text, bool distinct = false) = 0;
    virtual std::vector<U8String> snippets(const U8String& collection) const = 0;

    virtual U8String target_file() const = 0;
    virtual U8String target_folder() const = 0;

    static ShPtr<FileGenerator> create(ShPtr<Project> project, const U8String& sdn_path, LDDocument&& config);
};

void create_codegen_python_bindings();

void create_codegen_module(pybind11::module mm);
void create_ast_module(pybind11::module mm);

std::pair<U8String, U8String> split_path(U8String class_path);

U8String get_profile_id(U8String profile_name);

U8String get_same_level_path(U8String path, U8String step);
U8String join_sdn_path(Span<const U8String> path);

void for_each_value(LDDValueView elem, const std::function<bool (const std::vector<U8String>&, LDDValueView)>& consumer);

template <typename T>
T&& get_or_fail(Optional<T>&& opt, U8StringView msg)
{
    if (opt.has_value()) {
        return std::move(opt.get());
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("{}", msg).do_throw();
    }
}

LDDArrayView get_or_add_array(LDDMapView map, const U8String& name);

struct ResourceNameConsumer {
    virtual ~ResourceNameConsumer() noexcept = default;

    virtual void add_source_file(std::string name) = 0;
    virtual void add_byproduct_file(std::string name) = 0;
};

class DefaultResourceNameConsumerImpl: public ResourceNameConsumer {
    LDDArrayView sources_;
    LDDArrayView byproducts_;

public:
    DefaultResourceNameConsumerImpl(LDDArrayView sources, LDDArrayView byproducts):
        sources_(sources), byproducts_(byproducts)
    {}

    void add_source_file(std::string name) {
        sources_.add_varchar(name);
    }

    void add_byproduct_file(std::string name) {
        byproducts_.add_varchar(name);
    }
};

std::string build_output_list(const LDDocumentView& doc);

}}
