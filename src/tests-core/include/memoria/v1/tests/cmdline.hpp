
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

#include <memoria/v1/tests/configuration.hpp>

#include <memoria/v1/core/types/types.hpp>

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

    U16String       image_name_;
    U16String       cfg_file_name_;

    U16String       dump_file_;

    Configurator    cfg_file_;
    Configurator    cfg_cmdline_;

    int             operations_;

    U16String       replay_file_;
    bool            replay_;

    U16String       out_folder_;

    int32_t         count_;

    U16String       coverage_;
    U16String       coverage_size_;

    U16String       soft_memlimit_;
    U16String       hard_memlimit_;

public:

    enum {NONE = 0, REPLAY = 1};

    CmdLine(int argc, const char** argv, const char** envp, U16StringRef cfg_file_name, int operations = 0):
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
        count_(1)
    {
        Process();
    };

    U16StringRef getConfigFileName() const
    {
        return cfg_file_name_;
    }

    U16StringRef getImageName() const
    {
        return image_name_;
    }

    U16StringRef getDumpFileName() const
    {
        return dump_file_;
    }

    U16StringRef& getOutFolder() const
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

    int32_t getCount() const {
        return count_;
    }

    Configurator& getConfigurator()
    {
        return cfg_cmdline_;
    }

    U16StringRef getReplayFile()
    {
        return replay_file_;
    }

    U16StringRef getCoverage() const
    {
        return coverage_;
    }

    U16StringRef getCoverageSize() const {
        return coverage_size_;
    }

    U16StringRef getSoftMemLimit() const
    {
        return soft_memlimit_;
    }

    U16StringRef getHardMemLimit() const {
        return hard_memlimit_;
    }

    void Process();
protected:
    static U16String getImagePathPart(const char* str);
    static U16String getImageName(const char* str);

    void processTests();
    void processDump();
};


}}
