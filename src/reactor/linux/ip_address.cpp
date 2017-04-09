
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


#include <memoria/v1/reactor/linux/socket_impl.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdint.h>


namespace memoria {
namespace v1 {
namespace reactor {

IPAddress::IPAddress()
{
    boost::get<in_addr>(address_) = tools::make_zeroed<in_addr>();
    boost::get<in_addr>(address_).s_addr = INADDR_ANY;
}
    
IPAddress::IPAddress(const char* addr, bool v4)
{
    if (v4) 
    {
        inet_pton(AF_INET, addr, &boost::get<in_addr>(address_));
    }
    else {
        inet_pton(AF_INET6, addr, &boost::get<in6_addr>(address_));
    }
}

IPAddress::IPAddress(uint8_t oct3, uint8_t oct2, uint8_t oct1, uint8_t oct0)
{
    auto addr = tools::make_zeroed<in_addr>();
    
    uint32_t value = ((in_addr_t)oct3) << 24 | ((in_addr_t)oct2) << 16 | ((in_addr_t)oct1) << 8 | ((in_addr_t)oct0);
    
    addr.s_addr = ::htonl(value);
    
    boost::get<in_addr>(address_) = addr;
}

IPAddress::IPAddress(const uint8_t* octets, bool v4) 
{
    if (v4)
    {
        auto addr = tools::make_zeroed<in_addr>();
        
        for (int c = 0; c < 4; c++) {
            addr.s_addr |= ((in_addr_t)octets[c]) << (c * 8);
        }
        
        boost::get<in_addr>(address_) = addr;
    }
    else {
        auto addr = tools::make_zeroed<in6_addr>();
        
        for (int c = 0; c < 16; c++) {
            addr.__in6_u.__u6_addr8[c] = octets[c];
        }
        
        boost::get<in6_addr>(address_) = addr;
    }
}    
    

std::string IPAddress::to_string() const
{
    char str[INET6_ADDRSTRLEN];
    
    if (is_v4())
    {
        if (inet_ntop(AF_INET, &boost::get<in_addr>(address_), str, sizeof(str))) 
        {
            return std::string(str);
        }
        else {
            tools::rise_perror("Can't convert IPv4 address to std::string");
        }
    }
    else {
        if (inet_ntop(AF_INET6, &boost::get<in6_addr>(address_), str, sizeof(str))) 
        {
            return std::string(str);
        }
        else {
            tools::rise_perror("Can't convert IPv6 address to std::string");
        }
    }
}    

bool IPAddress::is_v4() const
{
    struct AddTypeChecker: boost::static_visitor<>
    {
        bool v4_{true};
        
        void operator()(const in_addr&) {}
        void operator()(const in6_addr&) {
            v4_ = false;
        }
    };
    
    AddTypeChecker visitor;
    boost::apply_visitor(visitor, address_);
    return visitor.v4_;
}
std::ostream& operator<<(std::ostream& out, const IPAddress& addr)
{
    out << addr.to_string();
    return out;
}



    
}}}
