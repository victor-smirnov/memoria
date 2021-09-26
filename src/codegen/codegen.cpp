
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


#include <memoria/core/types.hpp>
#include <memoria/core/linked/document/ld_document.hpp>
#include <memoria/memoria_core.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/result.hpp>

#include <codegen.hpp>

#include <boost/program_options.hpp>
#include <pybind11/embed.h>

#include <fstream>
#include <filesystem>

using namespace memoria;
using namespace memoria::codegen;

namespace po = boost::program_options;
namespace py = pybind11;

std::string build_generated_files_list(const LDDocumentView& doc)
{
    std::stringstream ss;
    std::vector<U8String> files;

    if (doc.value().is_map())
    {
        LDDMapView mm = doc.value().as_map();
        auto byproducts = mm.get("byproducts");
        if (byproducts.is_initialized()) {
            if (byproducts.get().is_array())
            {
                LDDArrayView arr = byproducts.get().as_array();
                for (size_t c = 0; c < arr.size(); c++) {
                    files.push_back(U8String("BYPRODUCT:") + arr.get(c).as_varchar().view());
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("byproducts list is not an SDN array").do_throw();
            }
        }

        auto sources = mm.get("sources");
        if (sources.is_initialized()) {
            if (sources.get().is_array())
            {
                LDDArrayView arr = sources.get().as_array();
                for (size_t c = 0; c < arr.size(); c++) {
                    files.push_back(U8String("SOURCE:") + arr.get(c).as_varchar().view());
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("sources list is not an SDN array").do_throw();
            }
        }

        auto profiles = mm.get("active_profiles");
        if (profiles.is_initialized()) {
            if (profiles.get().is_array())
            {
                LDDArrayView arr = profiles.get().as_array();
                for (size_t c = 0; c < arr.size(); c++) {
                    files.push_back(U8String("PROFILE:") + arr.get(c).as_varchar().view());
                }
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("active_profiles list is not an SDN array").do_throw();
            }
        }
    }

    for (size_t c = 0; c < files.size(); c++)
    {
        U8String name = files[c];
        ss << name;
        if (c < files.size() - 1) {
            ss << ";";
        }
    }

    return ss.str();
}




int main(int argc, char** argv)
{
    InitMemoriaCoreExplicit();

    try {
        create_codegen_python_bindings();

        py::scoped_interpreter guard;
        po::options_description options;

        using StringOpts = std::vector<std::string>;

        options.add_options()
                ("sysroot", po::value<std::string>(), "Specify Clang sysroot (for header files)")
                ("include,I", po::value<StringOpts>(), "Add include directory (-I)")
                ("define,D", po::value<StringOpts>(), "Add preprocessor definition (-D)")
                ("enable", po::value<StringOpts>(), "Enable profile")
                ("disable", po::value<StringOpts>(), "Disable profile")
                ("config", po::value<std::string>(), "Config file")
                ("project-output", po::value<std::string>(), "Main output folder")
                ("components-output-base", po::value<std::string>(), "Output folder prefix for compoments")
                ("verbose,v", po::value<std::string>(), "Provide additional debug info")
                ("print-output-file-names", "Only print filenames that will be created during full run (for CMake integration)")
                ("reuse-output-file-names", "Reuse previously generated filenames list that will be created during full run (for CMake integration, faster build)")
                ;

        po::variables_map map;

        po::store(
            boost::program_options::parse_command_line(argc, argv, options),
            map
        );

        add_parser_clang_option("-std=c++17");

        bool verbose = map.count("verbose");

        if (map.count("include"))
        {
            for (const auto& inc: map["include"].as<StringOpts>()) {
                if (verbose) println("Include opt: {}", inc);
                add_parser_clang_option("-I" + inc);
            }
        }

        if (map.count("define"))
        {
            for (const auto& inc: map["define"].as<StringOpts>()) {
                if (verbose) println("Define opt: {}", inc);
                add_parser_clang_option("-D" + inc);
            }
        }

        if (map.count("sysroot"))
        {
            std::string sysroot = map["sysroot"].as<std::string>();
            add_parser_clang_option("-isysroot " + sysroot);
        }

        boost::program_options::notify(map);

        if (!map.count("config")) {
            println(std::cerr, "Error: --config option must be specified (input file)");
            exit(1);
        }

        if (!map.count("project-output")) {
            println(std::cerr, "Error: --project-output option must be specified (output folder)");
            exit(2);
        }

        if (!map.count("components-output-base")) {
            println(std::cerr, "Error: --components-output-base option must be specified (components output folder base)");
            exit(3);
        }

        auto config_file = map["config"].as<std::string>();
        auto project_output_folder  = map["project-output"].as<std::string>();
        auto components_output_base  = map["components-output-base"].as<std::string>();

        add_parser_clang_option("-I" + project_output_folder);

        std::string file_name = project_output_folder + "/generator_state.sdn";
        bool list_exists = std::filesystem::exists(file_name);

        if (map.count("print-output-file-names"))
        {
            if (map.count("reuse-output-file-names") && list_exists)
            {
                std::string text = load_text_file(file_name);
                LDDocument doc = LDDocument::parse(text);

                std::cout << build_generated_files_list(doc);
            }
            else {
                add_parser_clang_option("-Wno-everything");

                auto project = Project::create(config_file, project_output_folder, components_output_base);

                project->parse_configuration();

                if (map.count("enable"))
                {
                    for (const auto& opt: map["enable"].as<StringOpts>()) {
                        if (verbose) println("Enable profile opt: {}", opt);
                        project->add_enabled_profile(opt);
                    }
                }

                if (map.count("enable"))
                {
                    for (const auto& opt: map["enable"].as<StringOpts>()) {
                        if (verbose) println("Disable profile opt: {}", opt);
                        project->add_disabled_profile(opt);
                    }
                }
                LDDocument doc = project->dry_run();
                //LDDMapView map = doc.set_map();
                //LDDArrayView arr = map.set_array("generated_files");
                //auto names = project->build_file_names();

//                for (const auto& name: names) {
//                    arr.add_varchar(name);
//                }

                write_text_file(file_name, doc.to_pretty_string());

                std::cout << build_generated_files_list(doc);
            }
        }
        else if (map.count("reuse-output-file-names") && list_exists) {
            std::cout << "Reusing previously generated files" << std::endl;
        }
        else {
            println("About to create project");

            auto project = Project::create(config_file, project_output_folder, components_output_base);
            println("About to parse the Configuration");
            project->parse_configuration();

            if (map.count("enable"))
            {
                for (const auto& opt: map["enable"].as<StringOpts>()) {
                    if (verbose) println("Enable profile opt: {}", opt);
                    project->add_enabled_profile(opt);
                }
            }

            if (map.count("enable"))
            {
                for (const auto& opt: map["enable"].as<StringOpts>()) {
                    if (verbose) println("Disable profile opt: {}", opt);
                    project->add_disabled_profile(opt);
                }
            }

            println("Configuration has been parsed");
            project->generate_artifacts();
        }

        exit(0);
    }
    catch (const pybind11::error_already_set& err) {
        std::cout << err.what() << std::endl;
        exit(1);
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
        exit(1);
    }


    return 0;
}
