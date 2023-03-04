
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

//#define WIN32_LEAN_AND_MEAN

#include "../../core/tools/bzero_struct.hpp"
#include "../message/fiber_io_message.hpp"

#include <boost/mpl/size_t.hpp>
#include <boost/variant.hpp>

#include <fmt/format.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <cstring>
#include <string>


#include <iostream>
#include <memory>


namespace memoria {
namespace reactor {    

using ssize_t = std::int64_t;
    
class IPAddress {
    
    using AddressT = boost::variant<in_addr, in6_addr>;
    
    AddressT address_{};
public:
    IPAddress(const char* addr, bool v4 = true);
    IPAddress();
    
    IPAddress(uint8_t oct3, uint8_t oct2, uint8_t oct1, uint8_t oct0);
    
    IPAddress(const uint8_t* octets, bool v4 = true);
    
    bool is_v4() const; 
    
    std::string to_string() const;
    
    AddressT& address() {return address_;}
    const AddressT& address() const {return address_;}
    
    in_addr& to_in_addr() {return boost::get<in_addr>(address_);}
    const in_addr& to_in_addr() const {return boost::get<in_addr>(address_);}
    
    in6_addr& to_in6_addr() {return boost::get<in6_addr>(address_);}
    const in6_addr& to_in6_addr() const {return boost::get<in6_addr>(address_);}
};

std::ostream& operator<<(std::ostream& out, const IPAddress& addr);



class SocketConnectionData {
    SOCKET fd_;
    IPAddress ip_address_;
    uint16_t ip_port_;

    friend class SocketConnectionImpl;
    friend class ServerSocketConnectionImpl;
    friend class ServerSocketImpl;

    SocketConnectionData(SOCKET fd, IPAddress ip_address, uint16_t ip_port):
        fd_(fd), ip_address_(ip_address), ip_port_(ip_port)
    {}

public:
	SocketConnectionData(SocketConnectionData&& other) :
		fd_(other.take_fd())
	{}

    ~SocketConnectionData() noexcept {
        if (fd_ != INVALID_SOCKET)
        {
            std::cout << "Untaken SocketConnectionData detected. Aborting." << std::endl;
            std::terminate();
        }
    }



    const IPAddress& ip_address() const {
        return ip_address_;
    }

    uint16_t ip_port() const {return ip_port_;}

private:
    SOCKET take_fd()
    {
        SOCKET fd = fd_;
        fd_ = -1;
        return fd;
    }
};



    
}}

namespace fmt {

template <>
struct formatter<memoria::reactor::IPAddress> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::reactor::IPAddress& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", d.to_string());
    }
};

}

