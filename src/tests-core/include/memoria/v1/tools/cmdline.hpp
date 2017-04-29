
// Copyright 2012 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/tools/configuration.hpp>

#include <vector>
#include <ostream>

namespace memoria {
namespace v1 {

using namespace std;

class CmdLine {
    int             argc_;
    const char**    argv_;
    const char**    envp_;

    bool            help_;
    bool            list_;
    bool            dump_;

    String          image_name_;
    String          cfg_file_name_;

    String          dump_file_;

    Configurator    cfg_file_;
    Configurator    cfg_cmdline_;

    int             operations_;

    String          replay_file_;
    bool            replay_;

    const char*     out_folder_;

    Int             count_;

    String          coverage_;
    String          coverage_size_;

    String          soft_memlimit_;
    String          hard_memlimit_;

public:

    enum {NONE = 0, REPLAY = 1};

    CmdLine(int argc, const char** argv, const char** envp, StringRef cfg_file_name, int operations = 0):
        argc_(argc),
        argv_(argv),
        envp_(envp),
        help_(false),
        list_(false),
        dump_(false),
        image_name_(getImageName(argv[0])),
        cfg_file_name_(cfg_file_name),
        cfg_file_(),
        cfg_cmdline_(&cfg_file_),
        operations_(operations),
        replay_(false),
        out_folder_(NULL),
        count_(1)
    {
        Process();
    };

    StringRef getConfigFileName() const
    {
        return cfg_file_name_;
    }

    StringRef getImageName() const
    {
        return image_name_;
    }

    StringRef getDumpFileName() const
    {
        return dump_file_;
    }

    const char* getOutFolder() const
    {
        return out_folder_;
    }

    bool IsHelp() const
    {
        return help_;
    }

    bool IsList() const {
        return list_;
    }

    bool IsDump() const {
        return dump_;
    }

    bool IsReplay() const {
        return replay_;
    }

    Int getCount() const {
        return count_;
    }

    Configurator& getConfigurator()
    {
        return cfg_cmdline_;
    }

    StringRef getReplayFile()
    {
        return replay_file_;
    }

    StringRef getCoverage() const
    {
        return coverage_;
    }

    StringRef getCoverageSize() const {
        return coverage_size_;
    }

    StringRef getSoftMemLimit() const
    {
        return soft_memlimit_;
    }

    StringRef getHardMemLimit() const {
        return hard_memlimit_;
    }

    void Process();
protected:
    static String getImagePathPart(const char* str);
    static String getImageName(const char* str);

    void processTests();
    void processDump();
};


}}