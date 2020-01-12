
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


#include <memoria/v1/reactor/msvc/msvc_socket_impl.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#include "msvc_socket.hpp"

#include <boost/assert.hpp>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>

#include <memory>


#include <mswsock.h>

namespace memoria {
namespace v1 {
namespace reactor {

ServerSocketConnectionImpl::ServerSocketConnectionImpl(SocketConnectionData&& data):
	SocketConnectionImpl(data.take_fd()),
	ip_address_(data.ip_address()),
	ip_port_(data.ip_port())
{
	if (fd_ >= 0) {
		if (!CreateIoCompletionPort((HANDLE)fd_, engine().io_poller().completion_port(), (u_long)0, 0))
		{
			closesocket(fd_);
			MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("CreateIoCompletionPort associate failed with error");
		}
	}
	else {
		MMA1_THROW(SystemException(WSAGetLastError())) << format_ex(
			"Error starting read operation from socket connection for {}:{}:{}",
			ip_address_, ip_port_, fd_
		);
	}
}

ServerSocketConnectionImpl::~ServerSocketConnectionImpl() noexcept 
{
	::closesocket(fd_);
}


size_t ServerSocketConnectionImpl::read(uint8_t* data, size_t size) 
{
	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;
	
	while (!conn_closed_) 
	{
		bool read_result = ::ReadFile((HANDLE)fd_, data, (DWORD)size, nullptr, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();

			if (overlapped.status_) {
				conn_closed_ = overlapped.size_ == 0;
				return overlapped.size_;
			}
			else if (overlapped.error_code_ == ERROR_NETNAME_DELETED) {
				conn_closed_ = true;
				return overlapped.size_;
			}
			else {
				MMA1_THROW(SystemException(overlapped.error_code_)) << format_ex(
					"Error reading from socket connection for {}:{}:{}",
					ip_address_, ip_port_, fd_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else if (error_code == ERROR_NETNAME_DELETED) {
			conn_closed_ = true;
			return 0;
		}
		else {
			MMA1_THROW(SystemException(error_code)) << format_ex(
				"Error starting read operation from socket connection for {}:{}:{}",
				ip_address_, ip_port_, fd_
			);
		}
	}

	return 0;
}

size_t ServerSocketConnectionImpl::write_(const uint8_t* data, size_t size) 
{
	DWORD number_of_bytes_read{};

	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;

	while (!conn_closed_) 
	{
		bool read_result = ::WriteFile((HANDLE)fd_, data, (DWORD)size, &number_of_bytes_read, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();
			
			if (overlapped.status_) {
				conn_closed_ = overlapped.size_ == 0;
				return overlapped.size_;
			}
			else if (overlapped.error_code_ == ERROR_NETNAME_DELETED) {
				conn_closed_ = true;
				return overlapped.size_;
			}
			else {
				MMA1_THROW(SystemException(overlapped.error_code_)) << format_ex(
					"Error writing to socket connection for {}:{}:{}",
					ip_address_, ip_port_, fd_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else if (error_code == ERROR_NETNAME_DELETED) {
			conn_closed_ = true;
			return 0;
		}
		else {
			MMA1_THROW(SystemException(error_code)) << format_ex(
				"Error starting write operation to socket connection for {}:{}:{}",
				ip_address_, ip_port_, fd_
			);
		}
	}

	return 0;
}


void ServerSocketConnectionImpl::close()
{
	if (!op_closed_) {
		op_closed_ = true;
		::closesocket(fd_);
	}
}



    
ServerSocketImpl::ServerSocketImpl(const IPAddress& ip_address, uint16_t ip_port): 
	SocketImpl(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
    ip_address_(ip_address), 
	ip_port_(ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()}
{
	if (fd_ == INVALID_SOCKET)
	{
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("Create of ListenSocket socket failed with error");
	}
	
	Reactor& r = engine();

    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");
    
	auto cport = r.io_poller().completion_port();

	if (!CreateIoCompletionPort((HANDLE)fd_, cport, (u_long)0, 0)) 
	{
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("CreateIoCompletionPort associate failed with errorr");
	}
    
    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = ::htons(ip_port_);
    
    int bres = ::bind(fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));
    
    if (bres == SOCKET_ERROR)
	{
		::closesocket(fd_);
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("Bind failed with error");
    }
}



    
ServerSocketImpl::~ServerSocketImpl() noexcept 
{    
	closesocket(fd_);
}

void ServerSocketImpl::close()
{
	if (!closed_) {
		closed_ = true;
		::closesocket(fd_);
	}
}


void ServerSocketImpl::listen() 
{
    int res = ::listen(fd_, 100);
    if (res != 0) 
    {
		MMA1_THROW(SystemException(WSAGetLastError())) << format_ex(
			"Can't start listening on socket for {}:{}",
			ip_address_, ip_port_
		);
    }
}




SocketConnectionData ServerSocketImpl::accept()
{
    Reactor& r = engine();
    
    sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);
    
	// Create an accepting socket
	SOCKET accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (accept_socket == INVALID_SOCKET)
	{
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("Create accept socket failed with error");
	}

	AIOMessage message(r.cpu());

	auto overlap = tools::make_zeroed<OVERLAPPEDMsg>();

	overlap.msg_ = &message;

	constexpr size_t ADDRLEN1 = sizeof(sockaddr_in) + 16;
	constexpr size_t ADDRLEN2 = sizeof(sockaddr_in) + 16;
	std::array<char, ADDRLEN1 + ADDRLEN2> o_buf;
	
	DWORD bytes{};
	bool accept_status = sockets->lpfn_accept_ex(
		fd_, accept_socket, o_buf.data(),
		0,
		ADDRLEN1, ADDRLEN2,
		&bytes,
		&overlap
	);

	DWORD error_code = GetLastError();

	if (accept_status || error_code == ERROR_IO_PENDING) 
	{
		message.wait_for();
		if (overlap.status_) {
			return SocketConnectionData(accept_socket, ip_address_, ip_port_);
		}
		else {
			MMA1_THROW(SystemException(overlap.error_code_)) << WhatCInfo("AcceptEx failed with error");
		}
	}
	else {
		closesocket(accept_socket);
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("AcceptEx failed with error");
	}
}






ClientSocketImpl::ClientSocketImpl(const IPAddress& ip_address, uint16_t ip_port) :
	ClientSocketConnectionImpl(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
	ip_address_(ip_address),
	ip_port_(ip_port)
{
	if (fd_ >= 0) 
	{
		Reactor& r = engine();

		BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");

		auto cport = r.io_poller().completion_port();

		if (!CreateIoCompletionPort((HANDLE)fd_, cport, (u_long)0, 0))
		{
			MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("CreateIoCompletionPort associate failed with error");
		}

		struct sockaddr_in bind_addr {};
		
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_addr.s_addr = INADDR_ANY;
		bind_addr.sin_port = 0;
		
		int bres = ::bind(fd_, tools::ptr_cast<sockaddr>(&bind_addr), sizeof(bind_addr));

		if (bres == SOCKET_ERROR)
		{
			::closesocket(fd_);
			MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("Bind failed with error");
		}

		connect();
	}
	else {
		MMA1_THROW(SystemException()) << format_ex(
			"Error starting read operation from socket connection for {}:{}:{}",
			ip_address_, ip_port_, fd_
		);
	}
}

void ClientSocketImpl::connect() 
{
	sock_address_.sin_family = AF_INET;
	sock_address_.sin_addr.s_addr = ip_address_.to_in_addr().s_addr;
	sock_address_.sin_port = ::htons(ip_port_);

	AIOMessage message(engine().cpu());
	auto overlap = tools::make_zeroed<OVERLAPPEDMsg>();
	overlap.msg_ = &message;

	DWORD bytes{};
	bool connect_status = sockets->lpfn_connect_ex(
		fd_, (SOCKADDR*)&sock_address_, sizeof(sock_address_),
		0,
		NULL, 0,
		&overlap
	);

	DWORD error_code = GetLastError();

	if (connect_status || error_code == ERROR_IO_PENDING)
	{
		message.wait_for();

		if (!overlap.status_) {
			MMA1_THROW(SystemException(overlap.error_code_)) << WhatCInfo("ConnectEx failed with error");
		}
	}
	else {
		closesocket(fd_);
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("ConnectEx failed with error");
	}
}

ClientSocketImpl::~ClientSocketImpl() noexcept
{
	::closesocket(fd_);
}


size_t ClientSocketImpl::read(uint8_t* data, size_t size)
{
	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;

	while (!conn_closed_)
	{
		bool read_result = ::ReadFile((HANDLE)fd_, data, (DWORD)size, nullptr, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();

			if (overlapped.status_) {
				conn_closed_ = overlapped.size_ == 0;
				return overlapped.size_;
			}
			else if (overlapped.error_code_ == ERROR_NETNAME_DELETED) {
				conn_closed_ = true;
				return overlapped.size_;
			}
			else {
				MMA1_THROW(SystemException(overlapped.error_code_)) << format_ex(
					"Error reading from socket connection for {}:{}:{}",
					ip_address_, ip_port_, fd_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else if (error_code == ERROR_NETNAME_DELETED) {
			conn_closed_ = true;
			return 0;
		}
		else {
			MMA1_THROW(SystemException(error_code)) << format_ex(
				"Error starting read operation from socket connection for {}:{}:{}",
				ip_address_, ip_port_, fd_
			);
		}
	}

	return 0;

}

size_t ClientSocketImpl::write_(const uint8_t* data, size_t size)
{
	DWORD number_of_bytes_read{};

	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;

	while (!conn_closed_)
	{
		bool read_result = ::WriteFile((HANDLE)fd_, data, (DWORD)size, &number_of_bytes_read, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();

			if (overlapped.status_) {
				conn_closed_ = overlapped.size_ == 0;
				return overlapped.size_;
			}
			else {
				MMA1_THROW(SystemException(overlapped.error_code_)) << format_ex(
					"Error writing to socket connection for {}:{}:{}",
					ip_address_, ip_port_, fd_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else if (error_code == ERROR_NETNAME_DELETED) {
			conn_closed_ = true;
			return 0;
		}
		else {
			MMA1_THROW(SystemException(error_code)) << format_ex(
				"Error starting write operation to socket connection for {}:{}:{}",
				ip_address_, ip_port_, fd_
			);
		}
	}

	return 0;
}


void ClientSocketImpl::close()
{
	if (!op_closed_) {
		op_closed_ = true;
		::closesocket(fd_);
	}
}

AsyncSockets* sockets = nullptr;


AsyncSockets::AsyncSockets()
{
	
	constexpr size_t ADDRLEN1 = sizeof(sockaddr_in) + 16;
	constexpr size_t ADDRLEN2 = sizeof(sockaddr_in) + 16;
	std::array<char, ADDRLEN1 + ADDRLEN2> o_buf;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == INVALID_SOCKET) {
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("Socket creation failed with error");
	}
		
	DWORD bytes1{};

	GUID guid_accept_ex = WSAID_ACCEPTEX;

	if (WSAIoctl(
		fd,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid_accept_ex, sizeof(guid_accept_ex),
		&lpfn_accept_ex, sizeof(guid_accept_ex),
		&bytes1, NULL, NULL
	) == SOCKET_ERROR) 
	{
		closesocket(fd);
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("WSAIoctl failed with error");
	}

	GUID guid_connect_ex = WSAID_CONNECTEX;
	DWORD bytes2{};
	if (WSAIoctl(
		fd,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid_connect_ex, sizeof(guid_connect_ex),
		&lpfn_connect_ex, sizeof(guid_connect_ex),
		&bytes2, NULL, NULL
	) == SOCKET_ERROR)
	{
		closesocket(fd);
		MMA1_THROW(SystemException(WSAGetLastError())) << WhatCInfo("WSAIoctl failed with error");
	}

	closesocket(fd);
}

void InitSockets() 
{
    WSADATA wsaData = { 0 };

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        wprintf(L"WSAStartup failed: %d\n", iResult);
        std::terminate();
    }

    sockets = new AsyncSockets();
}

void DestroySockets() {
    delete sockets;

    WSACleanup();
}
    
}}}
