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

#include <memoria/v1/core/types.hpp>

#ifdef MMA1_POSIX
#include "posix/posix_process_impl.hpp"
#elif defined (MMA1_WINDOWS)
#include "msvc/msvc_process_impl.hpp"
#endif


namespace memoria {
namespace v1 {
namespace reactor {


Process::Status Process::join() {
    return ptr_->join();
}

void Process::terminate() {
    return ptr_->terminate();
}

void Process::kill() {
    return ptr_->kill();
}

int32_t Process::exit_code() const {
    return ptr_->exit_code();
}

Process::Status Process::status() const {
    return ptr_->status();
}

PipeInputStream Process::out_stream() {
    return PipeInputStream(ptr_->out_stream());
}

PipeInputStream Process::err_stream() {
    return PipeInputStream(ptr_->err_stream());
}

PipeOutputStream Process::in_stream() {
    return PipeOutputStream(ptr_->in_stream());
}

std::ostream& operator<<(std::ostream& out, Process::Status status)
{
    switch (status)
    {
        case Process::Status::RUNNING: out << "RUNNING"; break;
        case Process::Status::EXITED: out << "EXITED"; break;
        case Process::Status::TERMINATED: out << "TERMINATED"; break;
        case Process::Status::CRASHED: out << "CRASHED"; break;
        case Process::Status::OTHER: out << "OTHER"; break;
        default: out << "UNKNOWN"; break;
    }

    return out;
}


ProcessBuilder ProcessBuilder::create(filesystem::path exe_path) {
	return ProcessBuilder(MakeLocalShared<ProcessBuilderImpl>(std::move(exe_path)));
}

ProcessBuilder& ProcessBuilder::with_args(U16String args) 
{
	ptr_->with_args(std::move(args));
	return *this;
}

ProcessBuilder& ProcessBuilder::with_args(std::vector<U16String> args) 
{
	ptr_->with_args(std::move(args));
	return *this;
}

ProcessBuilder& ProcessBuilder::with_env(EnvironmentList entries) 
{
	ptr_->with_env(std::move(entries));
	return *this;
}

ProcessBuilder& ProcessBuilder::with_env(EnvironmentMap entries) 
{
	ptr_->with_env(std::move(entries));
	return *this;
}

ProcessBuilder& ProcessBuilder::with_vfork(bool use_vfork) {
    ptr_->with_vfork(use_vfork);
    return *this;
}

Process ProcessBuilder::run() {
	return Process(MakeLocalShared<ProcessImpl>(ptr_.get()));
}

}}}
