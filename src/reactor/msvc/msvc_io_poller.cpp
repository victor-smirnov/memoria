
// Copyright 2017 Victor Smirnov
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


#include <memoria/v1/reactor/msvc/msvc_io_poller.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/tools/ptr_cast.hpp>
#include <memoria/v1/tools/perror.hpp>



#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <exception>
#include <algorithm>


namespace memoria {
namespace v1 {
namespace reactor {


void assert_ok(int result, const char* msg)
{
    if (result < 0)
    {
        std::cout << msg << " -- " << strerror(errno) << std::endl;
        std::terminate();
    }
}

IOPoller::IOPoller(IOBuffer& buffer):buffer_(buffer)
{
	// Create a handle for the completion port
	completion_port_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (u_long)0, 0);
	if (completion_port_ == NULL) 
	{
		DumpErrorMessage("CreateIoCompletionPort failed with error: ", GetLastError());
		std::terminate();
	}
}

IOPoller::~IOPoller() 
{
	CloseHandle(completion_port_);
}

#ifdef min
#undef min
#endif


void IOPoller::poll() 
{
	auto room_size = buffer_.capacity();
	if (room_size > 0)
	{
		size_t batch_size = std::min(room_size, (size_t)10);

		for (size_t c = 0; c < batch_size; c++)
		{
			DWORD num_bytes{};
			ULONG_PTR completion_key{};
			OVERLAPPED* overlapped {};

			auto status = GetQueuedCompletionStatus(completion_port_, &num_bytes, &completion_key, &overlapped, 0);
			
			auto error_code = GetLastError();

			if ((error_code == WAIT_TIMEOUT || error_code == ERROR_ABANDONED_WAIT_0) && !overlapped) {
				break; // do not wait for more events in this poll() invocation
			}
			else if (overlapped) 
			{
				auto ovlMsg = tools::ptr_cast<OVERLAPPEDMsg>(overlapped);

				ovlMsg->size_			= num_bytes;
				ovlMsg->completion_key_ = completion_key;
				ovlMsg->status_			= status;
				ovlMsg->error_code_		= error_code;

				ovlMsg->msg_->process();

				buffer_.push_front(ovlMsg->msg_);
			}
			else {
				DumpErrorMessage("GetQueuedCompletionStatus failed with error: ", error_code);
				std::terminate();
			}
		}
	}
}



std::string GetErrorMessage(DWORD error_code) 
{	
	void* cstr;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		error_code,
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
		(LPTSTR)&cstr,
		0,
		NULL
	);

	std::string str((char*)cstr);
	LocalFree(cstr);
	
	return str;
}


void DumpErrorMessage(DWORD error_code) {
	std::cout << "Error: " << GetErrorMessage(error_code) << std::endl;
}

void DumpErrorMessage(const std::string& prefix, DWORD error_code) {
	std::cout << prefix << " -- " << GetErrorMessage(error_code) << std::endl;
}

void DumpErrorAndTerminate(const std::string& prefix, DWORD error_code) {
	DumpErrorMessage(prefix, error_code);
	std::terminate();
}

[[noreturn]] void rise_win_error(tools::SBuf msg, DWORD error_code) 
{
	tools::SBuf msg2;
	msg2 << msg.str();

	msg2 << " -- " << GetErrorMessage(error_code);
	throw std::runtime_error(msg2);
}

}}}
