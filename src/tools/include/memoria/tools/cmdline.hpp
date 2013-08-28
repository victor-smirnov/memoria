
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_CMDLINE_HPP
#define _MEMORIA_TOOLS_CMDLINE_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/file.hpp>

#include <memoria/tools/configuration.hpp>

#include <vector>
#include <ostream>

namespace memoria {

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

    void Process();
protected:
    static String getImagePathPart(const char* str);
    static String getImageName(const char* str);

    void processTests();
    void processDump();
};


}
#endif
