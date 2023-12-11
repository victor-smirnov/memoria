
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
#include <memoria/memoria_core.hpp>
#include <memoria/core/tools/time.hpp>
#include <memoria/core/tools/result.hpp>

#include "compiler.hpp"
#include "codegen.hpp"
#include "inja_generators.hpp"

//#include "mbt_config.hpp"

#include <boost/program_options.hpp>

#include <fstream>
#include <filesystem>
#include <regex>

#include <pty.h>


extern "C" {
int openpty(int *amaster, int *aslave, char *name,
            const struct termios *termp,
            const struct winsize *winp) {return 0;}
pid_t forkpty(int *amaster, char *name,
              const struct termios *termp,
              const struct winsize *winp) {return pid_t{};}
}


using namespace memoria;
using namespace memoria::codegen;
using namespace memoria::hermes;

namespace po = boost::program_options;

std::vector<U8String> repack_string_vector(const std::vector<std::string>& vv) {
  std::vector<U8String> res;

  for (const auto& item: vv) {
    res.emplace_back(item);
  }

  return res;
}

std::vector<U8String> Split(const U8String& str, const U8String& regex)
{
  std::regex re(regex.to_std_string());
  return {
    std::sregex_token_iterator(
        str.to_std_string().begin(),
        str.to_std_string().end(), re, -1
    ),
    std::sregex_token_iterator()
  };
}

int main(int argc, char** argv)
{
  InitMemoriaCoreExplicit();

  try {
    register_inja_generator_factories();

    po::options_description options;

    using StringOpts = std::vector<std::string>;

    options.add_options()
        ("std", po::value<std::string>(), "Specify C++ Standard")
        ("sysroot", po::value<std::string>(), "Specify Clang sysroot (for header files)")
        ("include,I", po::value<StringOpts>(), "Add include directory (-I)")
        ("define,D", po::value<StringOpts>(), "Add preprocessor definition (-D)")
        ("enable", po::value<StringOpts>(), "Enable profile")
        ("disable", po::value<StringOpts>(), "Disable profile")
        ("config", po::value<StringOpts>(), "Config file. Multiple config files will be merged together.")
        ("project-output", po::value<std::string>(), "Main output folder")
        ("components-output-base", po::value<std::string>(), "Output folder prefix for compoments")
        ("verbose,v", po::value<std::string>(), "Provide additional debug info")
        ("dry-run", "Only print resources that will be created during full run (for CMake integration)")
        ("reuse-config", "Reuse previously generated resources (for CMake integration, faster build)")
        ;

    po::variables_map map;

    po::store(
          boost::program_options::parse_command_line(argc, argv, options),
          map
    );

    if (map.count("std")) {
        add_parser_clang_option(U8String("-std=c++") + map["std"].as<std::string>());
    }
    else {
        add_parser_clang_option("-std=c++17");
    }

    bool verbose = map.count("verbose");
    auto cfg = get_compiler_config();
    for (const auto& inc: cfg->includes()){
        if (verbose) println("Config Include opt: {}", inc);
        add_parser_clang_option(U8String("-I") + inc);
    }

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
      println(std::cerr, "Error: at least one --config option must be specified (input file)");
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

    auto config_files = repack_string_vector(map["config"].as<StringOpts>());
    auto project_output_folder = map["project-output"].as<std::string>();
    auto components_output_base = map["components-output-base"].as<std::string>();

    add_parser_clang_option("-I" + project_output_folder);

    std::string file_name = project_output_folder + "/generator_state.sdn";
    bool list_exists = std::filesystem::exists(file_name);

    if (map.count("dry-run"))
    {
      if (map.count("reuse-config") && list_exists)
      {
        std::string text = load_text_file(file_name);
        auto doc = HermesCtrView::parse_document(text);
        std::cout << build_output_list(doc);
      }
      else {
        add_parser_clang_option("-Wno-everything");

        auto project = Project::create(config_files, project_output_folder, components_output_base);

        project->parse_configuration();

        if (map.count("enable"))
        {
          for (const auto& opt: map["enable"].as<StringOpts>()) {
            if (verbose) println("Enable profile opt: {}", opt);
            project->add_enabled_profile(opt);
          }
        }

        if (map.count("disable"))
        {
          for (const auto& opt: map["disable"].as<StringOpts>()) {
            if (verbose) println("Disable profile opt: {}", opt);
            project->add_disabled_profile(opt);
          }
        }

        auto doc = project->dry_run();
        write_text_file(file_name, doc.root().value().to_pretty_string());

        std::cout << build_output_list(doc);
      }
    }
    else if (map.count("reuse-config") && list_exists) {
      std::cout << "Reusing previously generated files" << std::endl;
    }
    else {
      auto project = Project::create(config_files, project_output_folder, components_output_base);

      project->parse_configuration();
      println("Configuration has been parsed");

      if (map.count("enable"))
      {
        for (const auto& opt: map["enable"].as<StringOpts>()) {
          if (verbose) println("Enable profile opt: {}", opt);
          project->add_enabled_profile(opt);
        }
      }

      if (map.count("disable"))
      {
        for (const auto& opt: map["disable"].as<StringOpts>()) {
          if (verbose) println("Disable profile opt: {}", opt);
          project->add_disabled_profile(opt);
        }
      }

      println("Generating reusable configuration");
      auto doc = project->dry_run();
      write_text_file(file_name, doc.to_pretty_string());

      println("Generating artifacts");
      project->generate_artifacts();
    }

    exit(0);
  }
  catch (const std::exception& ex) {
    std::cout << ex.what() << std::endl;
    exit(1);
  }


  return 0;
}
