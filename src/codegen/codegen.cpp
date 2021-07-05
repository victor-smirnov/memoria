
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
        ("output", po::value<std::string>(), "Output folder")
        ("verbose,v", po::value<std::string>(), "Provide additional debug info")
        ("print-output-file-names", "Only print filenames that will be created during full run (for CMake integration)")        
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

    if (!map.count("output")) {
        println(std::cerr, "Error: --output option must be specified (output folder)");
        exit(2);
    }

    auto config_file = map["config"].as<std::string>();
    auto out_folder  = map["output"].as<std::string>();

    add_parser_clang_option("-I" + out_folder);

    if (map.count("print-output-file-names"))
    {
        add_parser_clang_option("-Wno-everything");

        auto project = Project::create(config_file, out_folder);
        project->parse_configuration();

        auto names = project->build_file_names();        
        size_t cnt = 0;
        for (const auto& name: names) {
            std::cout << name;
            if (cnt < names.size() - 1) {
                std::cout << ";";
            }
            cnt++;
        }
    }
    else {
        auto project = Project::create(config_file, out_folder);
        project->parse_configuration();
        project->generate_artifacts();
    }

    exit(0);

    return 0;
}
