
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

	}
	else {
		rise_win_error(
			SBuf() << "Error starting read operation from socket connection for " << ip_address_ << ":" << ip_port_ << ":" << fd_,
			0
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
	
	while (true) 
	{
		bool read_result = ::ReadFile((HANDLE)fd_, data, (DWORD)size, nullptr, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();

			if (overlapped.status_) {
				return overlapped.size_;
			}
			else {
				rise_win_error(
					SBuf() << "Error reading from socket connection for " << ip_address_ << ":" << ip_port_ << ":" << fd_, 
					overlapped.error_code_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			rise_win_error(
				SBuf() << "Error starting read operation from socket connection for " << ip_address_ << ":" << ip_port_ << ":" << fd_, 
				error_code
			);
		}
	}

}

size_t ServerSocketConnectionImpl::write(const uint8_t* data, size_t size) 
{
	DWORD number_of_bytes_read{};

	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;

	while (true) 
	{
		bool read_result = ::WriteFile((HANDLE)fd_, data, (DWORD)size, &number_of_bytes_read, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();
			
			if (overlapped.status_) {
				return overlapped.size_;
			}
			else {
				rise_win_error(
					SBuf() << "Error writing to socket connection for " << ip_address_ << ":" << ip_port_ << ":" << fd_, 
					overlapped.error_code_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			rise_win_error(
				SBuf() << "Error starting write operation to socket connection for " << ip_address_ << ":" << ip_port_ << ":" << fd_, 
				error_code
			);
		}
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
		rise_win_error(SBuf() << "Create of ListenSocket socket failed with error", WSAGetLastError());
	}
	
	Reactor& r = engine();

    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");
    
	auto cport = r.io_poller().completion_port();

	if (!CreateIoCompletionPort((HANDLE)fd_, cport, (u_long)0, 0)) 
	{
		rise_win_error(SBuf() << "CreateIoCompletionPort associate failed with error", WSAGetLastError());
	}
    
    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = ::htons(ip_port_);
    
    int bres = ::bind(fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));
    
    if (bres == SOCKET_ERROR)
	{
		::closesocket(fd_);
		rise_win_error(SBuf() << "Bind failed with error", WSAGetLastError() );
    }
}



    
ServerSocketImpl::~ServerSocketImpl() noexcept 
{    
	closesocket(fd_);
}



void ServerSocketImpl::listen() 
{
    int res = ::listen(fd_, 100);
    if (res != 0) 
    {
		rise_win_error(SBuf() << "Can't start listening on socket for " << ip_address_ << ":" << ip_port_, WSAGetLastError());
    }
}




SocketConnectionData ServerSocketImpl::accept()
{
    Reactor& r = engine();
    
    sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);

	LPFN_ACCEPTEX lpfn_accept_ex{};
	GUID guid_accept_ex = WSAID_ACCEPTEX;

	constexpr size_t ADDRLEN1 = sizeof(sockaddr_in) + 16;
	constexpr size_t ADDRLEN2 = sizeof(sockaddr_in) + 16;
	std::array<char, ADDRLEN1 + ADDRLEN2> o_buf;

	DWORD bytes{};

	int result = WSAIoctl(
		fd_, 
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid_accept_ex, sizeof(guid_accept_ex),
		&lpfn_accept_ex, sizeof(guid_accept_ex),
		&bytes, NULL, NULL
	);

	if (result == SOCKET_ERROR) {
		closesocket(fd_);
		rise_win_error(SBuf() << "WSAIoctl failed with error", WSAGetLastError());
	}

    
	// Create an accepting socket
	SOCKET accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (accept_socket == INVALID_SOCKET)
	{
		rise_win_error(SBuf() << "Create accept socket failed with error", WSAGetLastError());
	}

	AIOMessage message(r.cpu());

	auto overlap = tools::make_zeroed<OVERLAPPEDMsg>();

	overlap.msg_ = &message;

	bool accept_status = lpfn_accept_ex(
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

		if (!CreateIoCompletionPort((HANDLE)accept_socket, r.io_poller().completion_port(), (u_long)0, 0))
		{
			closesocket(accept_socket);
			rise_win_error(SBuf() << "CreateIoCompletionPort associate failed with error", WSAGetLastError());
		}

		//return std::make_shared<ServerSocketConnectionImpl>(accept_socket);
		return SocketConnectionData(accept_socket, ip_address_, ip_port_);
	}
	else {
		closesocket(accept_socket);
		rise_win_error(SBuf() << "AcceptEx failed with error", WSAGetLastError());
	}
}


    
}}}
