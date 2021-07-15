
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
#include <memoria/python/python_commons.hpp>

#include <codegen.hpp>

#include <pybind11/embed.h>

#include <fstream>

using namespace memoria;
namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(codegen, cg_module) {

    auto ll = py::list();
    ll.append("codegen");

    cg_module.attr("__path__") = ll;

    auto clang_module = cg_module.def_submodule("clang");
    auto ast_module = clang_module.def_submodule("ast");

    codegen::create_codegen_module(cg_module);
    codegen::create_ast_module(ast_module);
}

namespace memoria {
namespace codegen {

void create_codegen_python_bindings() {}

void create_codegen_module(py::module mm)
{
    mm.def("get_profile_id", &get_profile_id);
    mm.def("load_text_file", &load_text_file);
    mm.def("write_text_file", &write_text_file);
    mm.def("write_text_file_if_different", &write_text_file_if_different);

    py::class_<CodegenEntity, ShPtr<CodegenEntity>> cg_entity(mm, "CodegenEntiry");
    cg_entity.def("describe", &CodegenEntity::describe);
    cg_entity.def("project", &CodegenEntity::project);
    cg_entity.def("includes", &CodegenEntity::includes);
    cg_entity.def("generated_files", &CodegenEntity::generated_files);

    py::class_<Project, ShPtr<Project>> project(mm, "Project");
    project.def("config_unit", &Project::config_unit);
    project.def("profiles", &Project::profiles);
    project.def("project_output_folder", &Project::project_output_folder);
    project.def("components_output_folder", &Project::components_output_folder);
    project.def("profile_includes", &Project::profile_includes);

    py::class_<TypeInstance, ShPtr<TypeInstance>> type_instance(mm, "TypeInstance", cg_entity);
    type_instance.def("type_factory", &TypeInstance::type_factory);
    type_instance.def("type", &TypeInstance::type);
    type_instance.def("name", &TypeInstance::name);
    type_instance.def("target_folder", &TypeInstance::target_folder);
    type_instance.def("target_file", &TypeInstance::target_file);
    type_instance.def("config_sdn_path", &TypeInstance::config_sdn_path);
    type_instance.def("initializer", &TypeInstance::initializer);
    type_instance.def("full_includes", &TypeInstance::full_includes);

    py::class_<TypeFactory, ShPtr<TypeFactory>> type_factory(mm, "TypeFactory", cg_entity);
    type_factory.def("name", &TypeFactory::name);
    type_factory.def("factory_id", &TypeFactory::factory_id);
    type_factory.def("type", &TypeFactory::type);

    py::class_<FileGenerator, ShPtr<FileGenerator>> file_generator(mm, "FileGenerator", cg_entity);
    file_generator.def("add_snippet", &FileGenerator::add_snippet);
    file_generator.def("snippets", &FileGenerator::snippets);
    file_generator.def("describe", &FileGenerator::describe);
    file_generator.def("target_file", &FileGenerator::target_file);
    file_generator.def("target_folder", &FileGenerator::target_folder);

    py::enum_<InferenceType>(mm, "InferenceType", py::arithmetic())
        .value("TYPE", InferenceType::TYPE)
        .value("VALUE", InferenceType::VALUE);

    py::class_<Inference> inference(mm, "Inference");
    inference.def(py::init<U8String>());
    inference.def(py::init<InferenceType, U8String>());
    inference.def("type", &Inference::type);
    inference.def("expression", &Inference::expression);
    inference.def_static("as_type", &Inference::as_type);
    inference.def_static("as_value", &Inference::as_value);

    py::class_<CodeModule, ShPtr<CodeModule>> code_module(mm, "CodeModule");
    code_module.def("file_path", &CodeModule::file_path);

    py::class_<PreCompiledHeader, ShPtr<PreCompiledHeader>> pch(mm, "PreCompiledHeader");
    pch.def("file_path", &PreCompiledHeader::file_path);
    pch.def("parent", &PreCompiledHeader::parent);
    pch.def("options", &PreCompiledHeader::options);
    pch.def("target_folder", &PreCompiledHeader::target_folder);
    pch.def("compile_with", &PreCompiledHeader::compile_with);
    pch.def("infer", &PreCompiledHeader::infer);
}


}}
