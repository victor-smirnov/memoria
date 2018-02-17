
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
#include <memoria/v1/core/strings/string.hpp>

#include <boost/filesystem.hpp>

namespace memoria {
namespace v1 {

namespace bf = boost::filesystem;

void CmdLine::Process()
{
    U16String image_name = getImageName();

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
        MMA1_THROW(Exception()) << WhatCInfo("Dump file name must be specified");
    }
    else if (argc_ > 2) {
        MMA1_THROW(Exception()) << WhatCInfo("Incorrect number of dump parameters specified");
    }
    else {
        dump_file_ = argv_[1];
        dump_ = true;
    }
}


void CmdLine::processTests()
{
    bool cfg_cpecified = false;

    for (int32_t c = 1; c < argc_; c++)
    {
        U16String arg(argv_[c]);
        if (arg.starts_with(u"-D"))
        {
            auto pos = arg.find_first_of(u"=");
            if (pos != U16String::NPOS)
            {
                cfg_cmdline_.AddProperty(arg.substring(2, pos - 2), arg.substring(pos + 1));
            }
            else {
                MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid property format: {}", arg));
            }
        }
        else if (arg == u"--help")
        {
            help_ = true;
        }
        else if (arg == u"--list")
        {
            list_ = true;
        }
        else if (arg == u"--dump")
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
                        MMA1_THROW(Exception()) << WhatCInfo("Dump operation has been already specified");
                    }
                }
                else {
                    MMA1_THROW(Exception()) << WhatCInfo("Incorrect --dump parameters number");
                }
            }
        }
        else if (arg == u"--config")
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
                        MMA1_THROW(Exception()) << WhatCInfo("Config file has been already specified");
                    }
                }
                else {
                    MMA1_THROW(Exception()) << WhatCInfo("Properties file is not specified for the --config option");
                }
            }
        }
        else if (arg == u"--out")
        {
            if (c < argc_ - 1)
            {
                if (out_folder_.is_empty())
                {
                    out_folder_ = U16String(argv_[c + 1]);
                    c += 1;
                }
                else {
                    MMA1_THROW(Exception()) << WhatCInfo("Output folder has been already specified");
                }
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Incorrect --out parameters number");
            }
        }
        else if (arg == u"--count")
        {
            if (c < argc_ - 1)
            {
                count_ = FromString<int32_t>::convert(U16String(argv_[c + 1]));
                c += 1;
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Incorrect --count parameters number");
            }
        }
        else if (arg == u"--coverage")
        {
            if (c < argc_ - 1)
            {
                coverage_ = argv_[c + 1];
                c += 1;
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Incorrect --coverage parameters number");
            }
        }
        else if (arg == u"--coverage-size")
        {
            if (c < argc_ - 1)
            {
                coverage_size_ = U16String(argv_[c + 1]);
                c += 1;
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Incorrect --coverage-size parameters number");
            }
        }
        else if (arg == u"--soft-memlimit")
        {
            if (c < argc_ - 1)
            {
                soft_memlimit_ = U16String(argv_[c + 1]);
                c += 1;
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Incorrect --soft-memlimit parameters number");
            }
        }
        else if (arg == u"--hard-memlimit")
        {
            if (c < argc_ - 1)
            {
                hard_memlimit_ = U16String(argv_[c + 1]);
                c += 1;
            }
            else {
                MMA1_THROW(Exception()) << WhatCInfo("Incorrect --hard-memlimit parameters number");
            }
        }
        else if (arg == u"--replay" && (operations_ & REPLAY))
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
                        MMA1_THROW(Exception()) << WhatCInfo("Replay operation has been already specified");
                    }
                }
                else {
                    MMA1_THROW(Exception()) << WhatCInfo("Incorrect --replay parameters number");
                }
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Unknown command line option is specified: {}", arg));
        }
    }

    if ((!cfg_cpecified) && !list_)
    {
        if (bf::exists(cfg_file_name_.to_u8().to_std_string()))
        {
            Configurator::Parse(cfg_file_name_, &cfg_file_);
        }
        else {
            U16String image_path = getImagePathPart(argv_[0]);
            U16String cfg_file_name = image_path + Platform::getFilePathSeparator() + cfg_file_name_;

            if (bf::exists(cfg_file_name.to_u8().to_std_string()))
            {
                Configurator::Parse(cfg_file_name, &cfg_file_);
            }
        }
    }

    if (coverage_ != u"") {
        cfg_cmdline_.AddProperty(u"coverage", coverage_);
    }

    if (coverage_size_ != u"") {
        cfg_cmdline_.AddProperty(u"coverage_size", coverage_size_);
    }

    if (soft_memlimit_ != u"") {
        cfg_cmdline_.AddProperty(u"soft_memlimit", soft_memlimit_);
    }

    if (hard_memlimit_ != u"") {
        cfg_cmdline_.AddProperty(u"hard_memlimit", hard_memlimit_);
    }
}




U16String CmdLine::getImagePathPart(const char* str)
{
    return U8String(bf::absolute(str).parent_path().string()).to_u16();
}

U16String CmdLine::getImageName(const char* str)
{
    return U8String(bf::absolute(str).filename().string()).to_u16();
}



}}
