
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
#include <memoria/v1/tools/perror.hpp>

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

StreamSocketConnection::StreamSocketConnection(SOCKET connection_fd, const std::shared_ptr<StreamSocket>& socket): 
	    socket_(socket),
        connection_fd_(connection_fd)
{}

StreamSocketConnection::~StreamSocketConnection() noexcept 
{
	::closesocket(connection_fd_);
}


ssize_t StreamSocketConnection::read(char* data, size_t size) 
{
	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;
	
	while (true) 
	{
		bool read_result = ::ReadFile((HANDLE)connection_fd_, data, (DWORD)size, nullptr, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();

			if (overlapped.status_) {
				return overlapped.size_;
			}
			else {
				rise_win_error(
					tools::SBuf() << "Error reading from socket connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_, 
					overlapped.error_code_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			rise_win_error(
				tools::SBuf() << "Error starting read operation from socket connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_, 
				error_code
			);
		}
	}

}

ssize_t StreamSocketConnection::write(const char* data, size_t size) 
{
	DWORD number_of_bytes_read{};

	Reactor& r = engine();

	AIOMessage message(r.cpu());
	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;

	while (true) 
	{
		bool read_result = ::WriteFile((HANDLE)connection_fd_, data, (DWORD)size, &number_of_bytes_read, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();
			
			if (overlapped.status_) {
				return overlapped.size_;
			}
			else {
				rise_win_error(
					tools::SBuf() << "Error writing to socket connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_, 
					overlapped.error_code_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			rise_win_error(
				tools::SBuf() << "Error starting write operation to socket connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_, 
				error_code
			);
		}
	}
}






    
StreamServerSocket::StreamServerSocket(const IPAddress& ip_address, uint16_t ip_port): 
    StreamSocket(ip_address, ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()}
{
	Reactor& r = engine();

    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (socket_fd_ == INVALID_SOCKET)
    {
		rise_win_error(tools::SBuf() << "Create of ListenSocket socket failed with error", WSAGetLastError());
    }

	auto cport = r.io_poller().completion_port();

	if (!CreateIoCompletionPort((HANDLE)socket_fd_, cport, (u_long)0, 0)) 
	{
		rise_win_error(tools::SBuf() << "CreateIoCompletionPort associate failed with error", WSAGetLastError());
	}
    
    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = ::htons(ip_port_);
    
    int bres = ::bind(socket_fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));
    
    if (bres == SOCKET_ERROR)
	{
		::closesocket(socket_fd_);
		rise_win_error(tools::SBuf() << "Bind failed with error", WSAGetLastError() );
    }
}



    
StreamServerSocket::~StreamServerSocket() noexcept 
{    
	closesocket(socket_fd_);
}    



void StreamServerSocket::listen() 
{
    int res = ::listen(socket_fd_, 100);
    if (res != 0) 
    {
		rise_win_error(tools::SBuf() << "Can't start listening on socket for " << ip_address_ << ":" << ip_port_, WSAGetLastError());
    }
}




std::unique_ptr<StreamSocketConnection> StreamServerSocket::accept()
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
		socket_fd_, 
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid_accept_ex, sizeof(guid_accept_ex),
		&lpfn_accept_ex, sizeof(guid_accept_ex),
		&bytes, NULL, NULL
	);

	if (result == SOCKET_ERROR) {
		closesocket(socket_fd_);
		rise_win_error(tools::SBuf() << "WSAIoctl failed with error", WSAGetLastError());
	}

    
	// Create an accepting socket
	SOCKET accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (accept_socket == INVALID_SOCKET)
	{
		rise_win_error(tools::SBuf() << "Create accept socket failed with error", WSAGetLastError());
	}

	AIOMessage message(r.cpu());

	auto overlap = tools::make_zeroed<OVERLAPPEDMsg>();

	overlap.msg_ = &message;

	bool accept_status = lpfn_accept_ex(
		socket_fd_, accept_socket, o_buf.data(),
		0,
		ADDRLEN1, ADDRLEN2,
		&bytes,
		&overlap);

	DWORD error_code = GetLastError();

	if (accept_status || error_code == ERROR_IO_PENDING) 
	{
		message.wait_for();

		if (!CreateIoCompletionPort((HANDLE)accept_socket, r.io_poller().completion_port(), (u_long)0, 0))
		{
			closesocket(accept_socket);
			rise_win_error(tools::SBuf() << "CreateIoCompletionPort associate failed with error", WSAGetLastError());
		}

		return std::make_unique<StreamSocketConnection>(accept_socket, this->shared_from_this());
	}
	else {
		closesocket(accept_socket);
		rise_win_error(tools::SBuf() << "AcceptEx failed with error", WSAGetLastError());
	}
}


    
}}}
