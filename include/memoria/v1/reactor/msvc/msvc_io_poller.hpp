
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

#pragma once

#include "msvc_smp.hpp"
#include "../message/fiber_io_message.hpp"
#include "../ring_buffer.hpp"
#include "msvc_io_messages.hpp"

#include "../../core/tools/strings/string_buffer.hpp"

#include <memory>
#include <thread>
#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")



namespace memoria {
namespace v1 {
namespace reactor {

std::string GetErrorMessage(DWORD error_code);
void DumpErrorMessage(DWORD error_code);
void DumpErrorMessage(const std::string& prefix, DWORD error_code);
void DumpErrorAndTerminate(const std::string& prefix, DWORD error_code);
[[noreturn]] void rise_win_error(SBuf msg, DWORD error_code);


using IOBuffer = RingBuffer<Message*>;

struct OVERLAPPEDMsg;





struct OVERLAPPEDMsg : OVERLAPPED {

	enum {READ, WRITE};

	AIOMessage* msg_;

	DWORD size_;
	ULONG_PTR completion_key_;
	DWORD error_code_;
	bool status_;
	int operation_;

	void* data_;
	int64_t requested_size_;

	void configure(int operation, void* data, int64_t offset, int64_t size) 
	{
		operation_ = operation;

		data_ = data;
		requested_size_ = size;

		this->Offset = (DWORD)offset;
		this->OffsetHigh = (DWORD)(offset >> 32);
	}

	void configure(int operation){
		operation_ = operation;
	}
};



class IOPoller {
    
    static constexpr int BATCH_SIZE = 512;
	
	HANDLE completion_port_{};
    
	IOBuffer& buffer_;

public:
    IOPoller(IOBuffer& buffer);
    
    ~IOPoller();
    
    void poll();
    
    
	HANDLE completion_port() { return completion_port_; }
};



}}}
