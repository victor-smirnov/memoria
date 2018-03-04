
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

#include <memoria/v1/core/memory/smart_ptrs.hpp>
#include <memoria/v1/core/regexp/icu_regexp.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/process.hpp>
#include <memoria/v1/reactor/application.hpp>

#include <memoria/v1/fiber/count_down_latch.hpp>

#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


namespace memoria {
namespace v1 {
namespace reactor {

namespace _ {
U16String get_image_name(const U16String& filename)
{
    size_t start = filename.find_last_of(u"/");

    if (start == U16String::NPOS) {
        start = 0;
    }

    return filename.substring(start);
}
}

class CharPtrList {
    std::vector<char*> list_;
public:
    CharPtrList() = default;
    CharPtrList(CharPtrList&&) = delete;
    CharPtrList(const CharPtrList&) = delete;


    ~CharPtrList() noexcept {
        delete_list();
    }

    void add(const char* ptr, size_t size)
    {
        auto mem = allocate_system<char>(size + 1).release();
        std::memmove(mem, ptr, size + 1);
        list_.push_back(mem);
    }

    void add(const U8String& str)
    {
        auto mem = allocate_system<char>(str.size() + 1).release();
        std::memmove(mem, str.data(), str.size() + 1);
        list_.push_back(mem);
    }

    void finish()
    {
        list_.push_back(nullptr);
    }

    size_t size() const
    {
        return list_.size();
    }

    char** list()
    {
        if (list_.size() > 0) {
            return list_.data();
        }
        else {
            return nullptr;
        }
    }

    void clear() noexcept
    {
        delete_list();
    }

private:
    void delete_list() noexcept
    {
        for (auto& ptr: list_) {
            free_system(ptr);
        }

        list_.clear();
    }
};




class ProcessBuilderImpl {
	filesystem::path exe_path_;

    CharPtrList args_;
    CharPtrList envp_;

    bool use_vfork_{false};

public:
	ProcessBuilderImpl(filesystem::path&& exe_path) :
        exe_path_(std::move(exe_path))
	{
        with_env(app().env().entries_list());

        auto name = _::get_image_name(exe_path_.filename().to_u16()).to_u8();
        args_.add(name.data(), name.size());
        args_.finish();
	}

	~ProcessBuilderImpl() noexcept
	{
	}

    bool is_use_vfork() const {
        return use_vfork_;
    }

	filesystem::path& exe_path() {
		return exe_path_;
	}

    char** args()
    {
        return args_.list();
    }

    char** envp()
    {
        return envp_.list();
    }

    void with_args(U16String&& args)
    {
        auto space = ICURegexPattern::compile(u"\\p{WSpace=yes}+");

        auto arg_tokens = space.split(args);

        return with_args(std::move(arg_tokens));
	}

	void with_args(std::vector<U16String>&& args)
	{
        args_.clear();

        for (auto& entry: args)
        {
            auto u8_entry = entry.to_u8();
            args_.add(u8_entry.data());
        }

        args_.finish();
	}


	void with_env(EnvironmentList&& entries) 
	{
        envp_.clear();

        for (auto& entry: entries)
        {
            auto u8_entry = entry.to_u8();
            envp_.add(u8_entry.data());
        }

       envp_.finish();
	}


	void with_env(EnvironmentMap&& entries) 
	{
        envp_.clear();

        for (auto& entry: entries)
        {
            auto u8_entry = entry.first.to_u8() + "=" + entry.second.to_u8();
            envp_.add(u8_entry.data());
        }

       envp_.finish();
	}

    void with_vfork(bool vfork) {
        use_vfork_ = vfork;
    }
};





class ProcessImpl {

    PipeOutputStream in_stream_;
    PipeInputStream  out_stream_;
    PipeInputStream  err_stream_;

    pid_t pid_;

    bool finished_{false};
    int32_t status_{};
    int32_t error_code_{};
    int32_t waitpid_ret_{};

    fibers::count_down_latch<int32_t> process_status_latch_{1};

public:
    ProcessImpl(ProcessBuilderImpl* builder)
    {
        auto pipe_out = open_pipe();
        auto pipe_in  = open_pipe();
        auto pipe_err = open_pipe();

        in_stream_  = pipe_in.output;
        out_stream_ = pipe_out.input;
        err_stream_ = pipe_err.input;

        int32_t stdout_child_fd = pipe_out.output.detach();
        int32_t stderr_child_fd = pipe_err.output.detach();
        int32_t stdin_child_fd  = pipe_in.input.detach();


        if (builder->is_use_vfork()) {
            pid_ = vfork();
        }
        else {
            pid_ = fork();
        }

        if (pid_ == -1)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't create process {}", builder->exe_path());
        }
        else if (pid_ == 0)
        {
            // child

            fcntl(pipe_in.output.handle(), F_SETFL, O_CLOEXEC);
            fcntl(pipe_err.input.handle(), F_SETFL, O_CLOEXEC);
            fcntl(pipe_out.input.handle(), F_SETFL, O_CLOEXEC);

            if (dup2(stdin_child_fd, 0) < 0)
            {
                std::cout << "Can't dup2 stdin: " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (dup2(stderr_child_fd, 2) < 0)
            {
                std::cout << "Can't dup2 stderr: " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (dup2(stdout_child_fd, 1) < 0)
            {
                std::cout << "Can't dup2 stdout: " << strerror(errno) << std::endl;
                exit(-1);
            }

            if (execve(builder->exe_path().std_string().data(), builder->args(), builder->envp()) < 0)
            {
                std::cout << "Can't execute process: " << builder->exe_path() << ", reason: " << strerror(errno) << std::endl;
                exit(-1);
            }
        }
        else {
            // parent
            close(stdout_child_fd);
            close(stderr_child_fd);
            close(stdin_child_fd);

            engine().run_in_thread_pool_special([&]() noexcept {
                wait_for_pid();
            }, [&](auto fiber_context){
                process_status_latch_.dec();
                finished_ = true;
            });
        }
    }

    virtual ~ProcessImpl() noexcept {}

    Process::Status join()
    {
        process_status_latch_.wait(0);
        return status();
    }

    void terminate()
    {
        ::kill(pid_, SIGTERM);
    }

    void kill()
    {
        ::kill(pid_, SIGKILL);
    }

    Process::Status status() const
    {
        if (!finished_)
        {
            return Process::Status::RUNNING;
        }
        else if (WIFEXITED(status_)) {
            return Process::Status::EXITED;
        }
        else if (WIFSIGNALED(status_))
        {
            int sign = WTERMSIG(status_);

            if (sign == SIGTERM) {
                return Process::Status::TERMINATED;
            }
            else {
                return Process::Status::CRASHED;
            }
        }

        return Process::Status::OTHER;
    }

    int32_t exit_code() const {
        return WEXITSTATUS(status_);
    }

    auto out_stream() {
        return out_stream_.ptr();
    }

    auto err_stream() {
        return err_stream_.ptr();
    }

    auto in_stream() {
        return in_stream_.ptr();
    }

private:
    void wait_for_pid()
    {
        do
        {
            waitpid_ret_ = ::waitpid(pid_, &status_, 0);
        }
        while (((waitpid_ret_ == -1) && (errno == EINTR)) || (waitpid_ret_ != -1 && !WIFEXITED(status_) && !WIFSIGNALED(status_)));

        error_code_ = errno;
    }
};


}}}
