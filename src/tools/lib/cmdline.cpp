
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/tools/cmdline.hpp>
#include <memoria/core/tools/platform.hpp>
#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings/string.hpp>

namespace memoria {

void CmdLine::Process()
{
    String image_name = getImageName();

    if (image_name == "dump")
    {
        processDump();
    }
    else {
        processTests();
    }
}

void CmdLine::processDump()
{
    if (argc_ == 1) {
        throw Exception(MA_SRC, SBuf()<<"Dump file name must be specified");
    }
    else if (argc_ > 2) {
        throw Exception(MA_SRC, SBuf()<<"Incorrect number of dump parameters specified");
    }
    else {
        dump_file_ = argv_[1];
        dump_ = true;
    }
}


void CmdLine::processTests()
{
    bool cfg_cpecified = false;

    for (Int c = 1; c < argc_; c++)
    {
        String arg(argv_[c]);
        if (isStartsWith(arg, "-D"))
        {
            auto pos = arg.find_first_of("=");
            if (pos != String::npos)
            {
                cfg_cmdline_.AddProperty(arg.substr(2, pos - 2), arg.substr(pos + 1));
            }
            else {
                throw Exception(MEMORIA_SOURCE, SBuf()<<"Invalid property format: "<<arg);
            }
        }
        else if (arg == "--help")
        {
            help_ = true;
        }
        else if (arg == "--list")
        {
            list_ = true;
        }
        else if (arg == "--dump")
        {
            if (!list_)
            {
                if (c < argc_ - 1)
                {
                    if (!dump_)
                    {
                        dump_file_  = argv_[c + 1];
                        dump_       = true;
                        c += 1;
                    }
                    else {
                        throw Exception(MEMORIA_SOURCE, "Dump operation has been already specified");
                    }
                }
                else {
                    throw Exception(MEMORIA_SOURCE, "Incorrect --dump parameters number");
                }
            }
        }
        else if (arg == "--config")
        {
            if (!list_)
            {
                if (c < argc_ - 1)
                {
                    if (!cfg_cpecified)
                    {
                        c++;
                        Configurator::Parse(argv_[c], &cfg_file_);
                        cfg_cpecified = true;
                    }
                    else {
                        throw Exception(MEMORIA_SOURCE, "Config file has been already specified");
                    }
                }
                else {
                    throw Exception(MEMORIA_SOURCE, "Properties file is not specified for the --config option");
                }
            }
        }
        else if (arg == "--out")
        {
            if (c < argc_ - 1)
            {
                if (!out_folder_)
                {
                    out_folder_ = argv_[c + 1];
                    c += 1;
                }
                else {
                    throw Exception(MEMORIA_SOURCE, "Output folder has been already specified");
                }
            }
            else {
                throw Exception(MEMORIA_SOURCE, "Incorrect --out parameters number");
            }
        }
        else if (arg == "--count")
        {
            if (c < argc_ - 1)
            {
                count_ = FromString<Int>::convert(argv_[c + 1]);
                c += 1;
            }
            else {
                throw Exception(MEMORIA_SOURCE, "Incorrect --count parameters number");
            }
        }
        else if (arg == "--coverage")
        {
            if (c < argc_ - 1)
            {
                coverage_ = argv_[c + 1];
                c += 1;
            }
            else {
                throw Exception(MEMORIA_SOURCE, "Incorrect --coverage parameters number");
            }
        }
        else if (arg == "--coverage-size")
        {
            if (c < argc_ - 1)
            {
                coverage_size_ = FromString<String>::convert(argv_[c + 1]);
                c += 1;
            }
            else {
                throw Exception(MEMORIA_SOURCE, "Incorrect --coverage-size parameters number");
            }
        }
        else if (arg == "--soft-memlimit")
        {
            if (c < argc_ - 1)
            {
                soft_memlimit_ = FromString<String>::convert(argv_[c + 1]);
                c += 1;
            }
            else {
                throw Exception(MEMORIA_SOURCE, "Incorrect --soft-memlimit parameters number");
            }
        }
        else if (arg == "--hard-memlimit")
        {
            if (c < argc_ - 1)
            {
                hard_memlimit_ = FromString<String>::convert(argv_[c + 1]);
                c += 1;
            }
            else {
                throw Exception(MEMORIA_SOURCE, "Incorrect --hard-memlimit parameters number");
            }
        }
        else if (arg == "--replay" && (operations_ & REPLAY))
        {
            if (!list_)
            {
                if (c < argc_ - 1)
                {
                    if (!replay_)
                    {
                        replay_file_ = argv_[c + 1];
                        replay_ = true;
                        c += 1;
                    }
                    else {
                        throw Exception(MEMORIA_SOURCE, "Replay operation has been already specified");
                    }
                }
                else {
                    throw Exception(MEMORIA_SOURCE, "Incorrect --replay parameters number");
                }
            }
        }
        else {
            throw Exception(MEMORIA_SOURCE, SBuf()<<"Unknown command line option is specified: "<<arg);
        }
    }

    if ((!cfg_cpecified) && !list_)
    {
        File f0(cfg_file_name_);

        if (f0.isExists())
        {
            Configurator::Parse(cfg_file_name_, &cfg_file_);
        }
        else {
            String image_path = getImagePathPart(argv_[0]);
            String cfg_file_name = image_path + Platform::getFilePathSeparator()+cfg_file_name_;

            File f1(cfg_file_name);

            if (f1.isExists())
            {
                Configurator::Parse(cfg_file_name, &cfg_file_);
            }
        }
    }

    if (coverage_ != "") {
        cfg_cmdline_.AddProperty("coverage", coverage_);
    }

    if (coverage_size_ != "") {
        cfg_cmdline_.AddProperty("coverage_size", coverage_size_);
    }

    if (soft_memlimit_ != "") {
        cfg_cmdline_.AddProperty("soft_memlimit", soft_memlimit_);
    }

    if (hard_memlimit_ != "") {
        cfg_cmdline_.AddProperty("hard_memlimit", hard_memlimit_);
    }
}








String CmdLine::getImagePathPart(const char* str)
{
    File file(str);
    String abs_path = file.getAbsolutePath();

    if (file.getName() == abs_path)
    {
        return Platform::getPathSeparator();
    }
    else {
        auto pos = abs_path.find_last_of(Platform::getFilePathSeparator());

        if (pos != String::npos)
        {
            return abs_path.substr(0, pos);
        }
        else {
            return Platform::getFilePathSeparator();
        }
    }
}

String CmdLine::getImageName(const char* str)
{
    String st(str);

    auto pos = st.find_last_of(Platform::getFilePathSeparator());

    if (pos != String::npos)
    {
        return st.substr(pos + 1);
    }
    else {
        return st;
    }
}



}
