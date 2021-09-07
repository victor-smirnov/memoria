
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

#include <codegen.hpp>

#include <boost/program_options.hpp>
#include <pybind11/embed.h>

#include <fstream>
#include <filesystem>

using namespace memoria;
using namespace memoria::codegen;

namespace po = boost::program_options;
namespace py = pybind11;

int main(int argc, char** argv)
{
    InitMemoriaCoreExplicit();
    create_codegen_python_bindings();

    py::scoped_interpreter guard;


    po::options_description options;

    using StringOpts = std::vector<std::string>;

    options.add_options()
        ("sysroot", po::value<std::string>(), "Specify Clang sysroot (for header files)")
        ("include,I", po::value<StringOpts>(), "Add include directory (-I)")
        ("define,D", po::value<StringOpts>(), "Add preprocessor definition (-D)")
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

    std::string file_name = project_output_folder + "/generated-files-list.txt";
    bool list_exists = std::filesystem::exists(file_name);

    if (map.count("print-output-file-names"))
    {
        if (map.count("reuse-output-file-names") && list_exists)
        {
            std::string text = load_text_file(file_name);
            std::cout << text;
        }
        else {
            add_parser_clang_option("-Wno-everything");

            auto project = Project::create(config_file, project_output_folder, components_output_base);

            project->parse_configuration();

            std::stringstream ss;

            auto names = project->build_file_names();

            size_t cnt = 0;
            for (const auto& name: names) {
                std::cout << name;
                ss << name;
                if (cnt < names.size() - 1) {
                    std::cout << ";";
                    ss << ";";
                }
                cnt++;
            }

            write_text_file(file_name, ss.str());
        }
    }
    else if (map.count("reuse-output-file-names") && list_exists) {
        std::cout << "Reusing previously generated files" << std::endl;
    }
    else {
        auto project = Project::create(config_file, project_output_folder, components_output_base);
        println("About to parse the Configuration");
        project->parse_configuration();
        println("Configuration has been parsed");
        project->generate_artifacts();
    }

    exit(0);

    return 0;
}
