
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


#include <memoria/v1/fiber/count_down_latch.hpp>

#include <boost/winapi/process.hpp>
#include <boost/winapi/synchronization.hpp>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/handle_info.hpp>


#include <WinNT.h>


#ifndef STATUS_FAIL_FAST_EXCEPTION
#define STATUS_FAIL_FAST_EXCEPTION ((DWORD)0xC0000602)
#endif

namespace memoria {
namespace v1 {
namespace reactor {

class ProcessImpl {

	using Handle = ::boost::winapi::HANDLE_;

    PipeOutputStream in_stream_;
    PipeInputStream  out_stream_;
    PipeInputStream  err_stream_;

	

    bool finished_{false};
    DWORD error_code_{};
	DWORD exit_code_{};
    
	::boost::winapi::STARTUPINFOW_ startup_info_{
		sizeof(::boost::winapi::STARTUPINFOW_),
		nullptr, nullptr, nullptr,
		0,0,0,0,0,0,0,0,0,0,
		nullptr,
		::boost::winapi::INVALID_HANDLE_VALUE_,
		::boost::winapi::INVALID_HANDLE_VALUE_,
		::boost::winapi::INVALID_HANDLE_VALUE_
	};

	::boost::winapi::PROCESS_INFORMATION_ proc_info_{ nullptr, nullptr, 0,0 };

    fibers::count_down_latch<int32_t> process_status_latch_{1};

	UWString cmd_line_;

public:
	ProcessImpl(const wchar_t* path, UWString cmd_line) : 
		cmd_line_(std::move(cmd_line))
    {
        auto pipe_out = open_pipe();
        auto pipe_in  = open_pipe();
        auto pipe_err = open_pipe();

        in_stream_  = pipe_in.output;
        out_stream_ = pipe_out.input;
        err_stream_ = pipe_err.input;

		startup_info_.dwFlags = ::boost::winapi::STARTF_USESTDHANDLES_;

        startup_info_.hStdOutput = pipe_out.output.detach();
		startup_info_.hStdError  = pipe_err.output.detach();
		startup_info_.hStdInput	 = pipe_in.input.detach();

		boost::winapi::SetHandleInformation(startup_info_.hStdOutput,
			boost::winapi::HANDLE_FLAG_INHERIT_,
			boost::winapi::HANDLE_FLAG_INHERIT_);

		boost::winapi::SetHandleInformation(startup_info_.hStdError,
			boost::winapi::HANDLE_FLAG_INHERIT_,
			boost::winapi::HANDLE_FLAG_INHERIT_);

		boost::winapi::SetHandleInformation(startup_info_.hStdInput,
			boost::winapi::HANDLE_FLAG_INHERIT_,
			boost::winapi::HANDLE_FLAG_INHERIT_);


		int proc_err_code = ::boost::winapi::create_process(
			path,                                       //       LPCSTR_ lpApplicationName,
			cmd_line_.data(),							//       LPSTR_ lpCommandLine,
			nullptr,									//       LPSECURITY_ATTRIBUTES_ lpProcessAttributes,
			nullptr,									//       LPSECURITY_ATTRIBUTES_ lpThreadAttributes,
			TRUE,										//       INT_ bInheritHandles,
			0,											//       DWORD_ dwCreationFlags,
			nullptr,									//		 LPVOID_ lpEnvironment,
			nullptr,                                    //       LPCSTR_ lpCurrentDirectory,
			&startup_info_,								//       LPSTARTUPINFOA_ lpStartupInfo,
			&proc_info_);                               //       LPPROCESS_INFORMATION_ lpProcessInformation)

		if (!proc_err_code)
		{
			MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't start process {}", UWString(path).to_u8());
		}

        engine().run_in_thread_pool_special([&]() noexcept {
                return wait_for_pid();
        }, 
		[&](DWORD error_code, auto fiber_context) {
                process_status_latch_.dec();
				do_process_finished();
        });
    }

    virtual ~ProcessImpl() noexcept 
	{
		
	}

	Handle handle() const {
		return proc_info_.hProcess;
	}

    Process::Status join()
    {
        process_status_latch_.wait(0);
        return status();
    }

    void terminate()
    {
		if (!TerminateProcess(handle(), STATUS_CONTROL_C_EXIT)) 
		{
			DumpErrorMessage(GetLastError());
		}
    }

    void kill()
    {
		if (!TerminateProcess(handle(), STATUS_FAIL_FAST_EXCEPTION)) {
			DumpErrorMessage(GetLastError());
		}
    }

	Process::Status status() const
	{
		if (!finished_)
		{
			return Process::Status::RUNNING;
		}
		else if (exit_code_ < 256) {
			return Process::Status::EXITED;
		}
		else if (exit_code_ == STATUS_CONTROL_C_EXIT) {
			return Process::Status::TERMINATED;
		}		
		else {
			return Process::Status::CRASHED;
		}
    }

    int32_t exit_code() const {
        return exit_code_;
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
    DWORD wait_for_pid() noexcept
    {
		if (::boost::winapi::WaitForSingleObject(handle(), ::boost::winapi::infinite) == ::boost::winapi::wait_failed) {
			return GetLastError();
		}
		return 0;
    }

	void do_process_finished() noexcept
	{
		if (!::boost::winapi::GetExitCodeProcess(handle(), &exit_code_)) 
		{
			error_code_ = GetLastError();
		}
		
		::boost::winapi::CloseHandle(proc_info_.hProcess);
		::boost::winapi::CloseHandle(proc_info_.hThread);
		
		proc_info_.hProcess = ::boost::winapi::INVALID_HANDLE_VALUE_;
		proc_info_.hThread = ::boost::winapi::INVALID_HANDLE_VALUE_;

		CloseHandle(startup_info_.hStdOutput);
		CloseHandle(startup_info_.hStdInput);
		CloseHandle(startup_info_.hStdError);

		finished_ = true;
	}
};


}}}
