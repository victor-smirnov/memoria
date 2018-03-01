
// Copyright 2018 Victor Smirnov
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


#include "posix_process_impl.hpp"


namespace memoria {
namespace v1 {
namespace reactor {


Process Process::create(const U16String& path, const U16String& args, const std::vector<U16String>& env)
{
    auto space = ICURegexPattern::compile(u"\\p{WSpace=yes}+");

    auto arg_tokens = space.split(args);

    return Process::create2(path, arg_tokens, env);
}


Process Process::create2(const U16String& path, const std::vector<U16String>& args, const std::vector<U16String>& env)
{
    std::vector<U8String> u8_args;

    for (const auto& arg: args)
    {
        u8_args.emplace_back(arg.to_u8());
    }

    auto args_char_array = allocate_system_zeroed<char*>((u8_args.size() + 1) * sizeof(char*));

    size_t cnt = 0;
    for (auto& arg: u8_args)
    {
        args_char_array.get()[cnt++] = arg.data();
    }

    std::vector<U8String> u8_env;

    for (const auto& env_entry: env)
    {
        u8_env.emplace_back(env_entry.to_u8());
    }

    auto env_char_array = allocate_system_zeroed<char*>((u8_env.size() + 1) * sizeof(char*));

    cnt = 0;
    for (auto& env_entry: u8_env)
    {
        env_char_array.get()[cnt++] = env_entry.data();
    }

    U8String u8_path = path.to_u8();

    return Process(MakeLocalShared<ProcessImpl>(u8_path.data(), args_char_array.get(), env_char_array.get()));
}




}}}
