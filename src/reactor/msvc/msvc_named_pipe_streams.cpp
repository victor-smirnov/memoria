
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

#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/pipe_streams.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>

#include <Winbase.h>

namespace memoria {
namespace v1 {
namespace reactor {

class PipeInputStreamImpl: public IPipeInputStream {
	filesystem::path path_;
	
	HANDLE handle_;

    bool op_closed_{false};
    bool data_closed_{false};
	bool connected_{false};

public:
    PipeInputStreamImpl(const char16_t* name, size_t buffer_size = 4096): path_(name)
	{
		handle_ = CreateNamedPipeW(
			U16String(name).to_uwstring().to_std_string().data(), // pipe name 
			PIPE_ACCESS_DUPLEX |		// read/write access 
			FILE_FLAG_OVERLAPPED,		// overlapped mode 
			PIPE_TYPE_BYTE |
			PIPE_REJECT_REMOTE_CLIENTS |
			PIPE_WAIT,					// blocking mode 
			PIPE_UNLIMITED_INSTANCES,	// number of instances 
			buffer_size,				// output buffer size 
			buffer_size,				// input buffer size 
			0,							// client time-out 
			nullptr						// default security attributes 
		);

		if (handle_ != INVALID_HANDLE_VALUE) 
		{
			if (CreateIoCompletionPort(handle_, engine().io_poller().completion_port(), (u_long)0, 0) == INVALID_HANDLE_VALUE)
			{
				MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't open pipe for reading: {}", path_);
			}
		}
		else {
			MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't open pipe for reading: {}", path_);
		}
    }

    virtual ~PipeInputStreamImpl() noexcept {
        close();
    }

	void connect()
	{
		if (!connected_) 
		{
			Reactor& r = engine();

			AIOMessage message(r.cpu());

			auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
			overlapped.msg_ = &message;
			while (true)
			{
				bool connection_result = ::ConnectNamedPipe(handle_, &overlapped);

				DWORD error_code = GetLastError();

				if (connection_result || error_code == ERROR_IO_PENDING)
				{
					message.wait_for();

					if (overlapped.status_) {
						connected_ = true;
						return;
					}
					else {
						MMA1_THROW(SystemException(overlapped.error_code_)) << fmt::format_ex(u"Error connecting to pipe {}", path_);
					}
				}
				else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY || error_code == ERROR_PIPE_LISTENING) {
					message.wait_for(); // jsut sleep and wait for required resources to appear
				}
				else {
					std::cout << GetErrorMessage(error_code) << " " << fmt::format8(u"Error connecting to pipe {}", path_) << std::endl;
					MMA1_THROW(SystemException(error_code)) << fmt::format_ex(u"Error connecting to pipe {}", path_);
				}
			}
		}
	}

    virtual IOHandle hande() const {return (IOHandle)handle_;}

	size_t read(uint8_t* buffer, size_t size)
	{
		connect();

		Reactor& r = engine();

		AIOMessage message(r.cpu());

		auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
		overlapped.msg_ = &message;

		while (true) {
			bool read_result = ::ReadFile(handle_, buffer, (DWORD)size, nullptr, &overlapped);

			DWORD error_code = GetLastError();

			if (read_result || error_code == ERROR_IO_PENDING)
			{
				message.wait_for();

				if (overlapped.status_) {
					return overlapped.size_;
				}
				else if (overlapped.error_code_ == ERROR_HANDLE_EOF) {
					return 0;
				}
				else {
					MMA1_THROW(SystemException(overlapped.error_code_)) << fmt::format_ex(u"Error reading from file {}", path_);
				}
			}
			else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY || error_code == ERROR_PIPE_LISTENING) {
				message.wait_for(); // jsut sleep and wait for required resources to appear
			}
			else {
				MMA1_THROW(SystemException(error_code)) << fmt::format_ex(u"Error starting read from file {}", path_);
			}
		}
	}

    virtual void close()
    {
		if (!op_closed_)
		{
			op_closed_ = true;

			if (!::CloseHandle(handle_)) {
				MMA1_THROW(SystemException());
			}
		}
    }

    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
    }
};

class PipeOutputStreamImpl: public IPipeOutputStream {
	filesystem::path path_;

    HANDLE handle_;

	bool op_closed_{ false };
	bool data_closed_{ false };
	bool connected_{ false };

public:
    PipeOutputStreamImpl(const char16_t* name, size_t buffer_size = 4096): path_(name)
    {
		handle_ = CreateNamedPipeW(
			U16String(name).to_uwstring().to_std_string().data(), // pipe name 
			PIPE_ACCESS_DUPLEX |		// read/write access 
			FILE_FLAG_OVERLAPPED,		// overlapped mode 
			PIPE_TYPE_BYTE |
			PIPE_REJECT_REMOTE_CLIENTS |
			PIPE_WAIT,					// blocking mode 
			PIPE_UNLIMITED_INSTANCES,	// number of instances 
			buffer_size,				// output buffer size 
			buffer_size,				// input buffer size 
			0,							// client time-out 
			nullptr						// default security attributes 
		);

		if (handle_ != INVALID_HANDLE_VALUE) 
		{
			if (CreateIoCompletionPort(handle_, engine().io_poller().completion_port(), (u_long)0, 0) == INVALID_HANDLE_VALUE)
			{
				MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't open pipe for writing: {}", path_);
			}
		}
		else {
			MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't open pipe for writing: {}", name);
		}
    }

	void connect() 
	{
		if (!connected_) 
		{
			Reactor& r = engine();

			AIOMessage message(r.cpu());

			auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
			overlapped.msg_ = &message;
			while (true)
			{
				bool read_result = ::ConnectNamedPipe(handle_, &overlapped);

				DWORD error_code = GetLastError();

				if (read_result || error_code == ERROR_IO_PENDING)
				{
					message.wait_for();

					if (overlapped.status_) {
						connected_ = true;
						return;
					}
					else {
						MMA1_THROW(SystemException(overlapped.error_code_)) << fmt::format_ex(u"Error connecting to pipe {}", path_);
					}
				}
				else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY || error_code == ERROR_PIPE_LISTENING) {
					message.wait_for(); // jsut sleep and wait for required resources to appear
				}
				else {
					std::cout << GetErrorMessage(error_code) << " " << fmt::format8(u"Error connecting to pipe {}", path_) << std::endl;
					MMA1_THROW(SystemException(error_code)) << fmt::format_ex(u"Error connecting to pipe {}", path_);
				}
			}
		}
	}

    virtual ~PipeOutputStreamImpl() noexcept {
        close();
    }

    virtual IOHandle hande() const {return (IOHandle)handle_;}

    virtual size_t write(const uint8_t* data, size_t size)
    {
        size_t total{};
        while (total < size) {
            total += write_(data + total, size - total);
        }
        return total;
    }

	size_t write_(const uint8_t* buffer, size_t size)
	{
		connect();

		Reactor& r = engine();

		AIOMessage message(r.cpu());

		auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
		overlapped.msg_ = &message;
		while (true)
		{
			bool read_result = ::WriteFile(handle_, buffer, (DWORD)size, nullptr, &overlapped);

			DWORD error_code = GetLastError();

			if (read_result || error_code == ERROR_IO_PENDING)
			{
				message.wait_for();

				if (overlapped.status_) {
					return overlapped.size_;
				}
				else {
					MMA1_THROW(SystemException(overlapped.error_code_)) << fmt::format_ex(u"Error writing to file {}", path_);
				}
			}
			else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY || error_code == ERROR_PIPE_LISTENING) {
				message.wait_for(); // jsut sleep and wait for required resources to appear
			}
			else {
				std::cout << GetErrorMessage(error_code) << " " << fmt::format8(u"Error starting write to file {}", path_) << std::endl;
				MMA1_THROW(SystemException(error_code)) << fmt::format_ex(u"Error starting write to file {}", path_);
			}
		}
	}


    virtual void close()
    {
        if (!op_closed_)
        {
            op_closed_ = true;

			if (!::CloseHandle(handle_)) {
				MMA1_THROW(SystemException());
			}
        }
    }

    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
    }

    virtual void flush() {}
};


PipeInputStream open_input_pipe(const char16_t* name) {
    return PipeInputStream(MakeLocalShared<PipeInputStreamImpl>(name));
}

PipeOutputStream open_output_pipe(const char16_t* name){
    return PipeOutputStream(MakeLocalShared<PipeOutputStreamImpl>(name));
}


}}}
