
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
};


struct Project {
    virtual ~Project() noexcept = default;

    virtual void parse_configuration() = 0;

    virtual ShPtr<CodeModule> config_unit() const noexcept = 0;
    virtual LDDocumentView config() const noexcept = 0;
    virtual LDDMapView config_map() const = 0;
    virtual U8String target_folder() const = 0;

    virtual std::vector<ShPtr<TypeInstance>> type_instances() const = 0;
    virtual std::vector<ShPtr<TypeFactory>> type_factories() const = 0;
    virtual std::vector<ShPtr<FileGenerator>> file_generators() const = 0;

    virtual std::vector<U8String> profiles() const = 0;

    virtual std::vector<U8String> build_file_names() = 0;
    virtual void generate_artifacts() = 0;

    static std::shared_ptr<Project> create(U8String config_file_name, U8String output_folder);
};

struct TypeInstance: CodegenEntity {
    virtual ~TypeInstance() noexcept = default;

    virtual const clang::ClassTemplateSpecializationDecl* ctr_descr() const = 0;
    virtual clang::QualType type() const = 0;
    virtual LDDocumentView config() const = 0;

    virtual ShPtr<TypeFactory> type_factory() const = 0;
    virtual void set_type_factory(ShPtr<TypeFactory> tf) = 0;

    virtual void precompile_headers() = 0;
    virtual void generate_artifacts() = 0;
    virtual std::vector<U8String> generated_files() = 0;
    virtual Optional<std::vector<U8String>> profiles() const = 0;

    virtual U8String name() const = 0;

    virtual void configure() = 0;

    static ShPtr<TypeInstance> create(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr);
};

struct TypeFactory: CodegenEntity {
    virtual ~TypeFactory() noexcept = default;

    virtual U8String name() const = 0;

    virtual U8String factory_id() const = 0;
    virtual LDDocumentView config() const = 0;
    virtual U8String type_pattern() const = 0;

    virtual std::vector<U8String> includes() const = 0;
    virtual std::vector<U8String> generated_files() const = 0;

    virtual void precompile_headers() = 0;
    virtual ShPtr<PreCompiledHeader> precompiled_header() const = 0;

    virtual U8String generator() const = 0;

    virtual const clang::ClassTemplateSpecializationDecl* factory_descr() const = 0;

    virtual void configure() = 0;

    static ShPtr<TypeFactory> create(ShPtr<Project> project, const clang::ClassTemplateSpecializationDecl* descr);
};



struct FileGenerator {
    virtual ~FileGenerator() noexcept = default;

    //static ShPtr<FileGenerato> create(ShPtr<Project> project, U8String output_folder, const clang::ClassTemplatePartialSpecializationDecl* descr);
};

void create_codegen_python_bindings();

void create_codegen_module(pybind11::module mm);
void create_ast_module(pybind11::module mm);

std::pair<U8String, U8String> split_path(U8String class_path);

U8String get_profile_id(U8String profile_name);

}}
