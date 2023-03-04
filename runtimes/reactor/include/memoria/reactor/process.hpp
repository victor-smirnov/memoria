
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


#pragma once

#include <memoria/core/types.hpp>
#include <memoria/core/tools/pimpl_base.hpp>

#include <memoria/reactor/pipe_streams.hpp>
#include <memoria/reactor/pipe_streams_reader.hpp>

#include <boost/filesystem/path.hpp>

#include <ostream>
#include <map>

namespace memoria {
namespace reactor {


boost::filesystem::path get_program_path();
boost::filesystem::path get_image_name();

using EnvironmentMap  = std::map<U8String, U8String>;
using EnvironmentList = std::vector<U8String>;


class ProcessImpl;

class Process: public PimplBase<ProcessImpl> {
    using Base = PimplBase<ProcessImpl>;
public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Process)

    enum class Status {RUNNING, EXITED, TERMINATED, CRASHED, OTHER};

    Status join();
    void terminate();
    void kill();

    int32_t exit_code() const;
    Status status() const;

    PipeInputStream out_stream();
    PipeInputStream err_stream();
    PipeOutputStream in_stream();
};


std::ostream& operator<<(std::ostream& out, Process::Status status) ;


class ProcessBuilderImpl;

class ProcessBuilder : public PimplBase<ProcessBuilderImpl> {
	using Base = PimplBase<ProcessBuilderImpl>;
public:
	MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(ProcessBuilder)


    static ProcessBuilder create(boost::filesystem::path exe_path);
	
    ProcessBuilder& with_args(U8String args);
    ProcessBuilder& with_args(std::vector<U8String> args);

    ProcessBuilder& with_env(EnvironmentList entries = EnvironmentList());
	ProcessBuilder& with_env(EnvironmentMap entries);

    ProcessBuilder& with_vfork(bool use_vfork);

	Process run();
};



}}
