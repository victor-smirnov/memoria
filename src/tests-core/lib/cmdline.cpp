
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



#include <memoria/v1/tests/cmdline.hpp>
#include <memoria/v1/core/tools/platform.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>

#include <boost/filesystem.hpp>

namespace memoria {
namespace v1 {

namespace bf = boost::filesystem;

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
        if (bf::exists(cfg_file_name_))
        {
            Configurator::Parse(cfg_file_name_, &cfg_file_);
        }
        else {
            String image_path = getImagePathPart(argv_[0]);
            String cfg_file_name = image_path + Platform::getFilePathSeparator()+cfg_file_name_;

            if (bf::exists(cfg_file_name))
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
    return bf::absolute(str).parent_path().string();
}

String CmdLine::getImageName(const char* str)
{
    return bf::absolute(str).filename().string();
}



}}
