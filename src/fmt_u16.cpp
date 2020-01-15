
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <tuple>
#include <iostream>
#include <functional>

namespace mm = memoria::v1;

void f2() {
    MMA1_THROW(mm::SystemException()) << mm::WhatInfo(mm::format_u8("Hello {} world!", "cruel"));
}

void f1() {
    f2();
}

int main() 
{
    try {
        f1();
    }
    catch (const boost::exception& ex)
    {
        std::cout << boost::diagnostic_information(ex) << "\n";
    }

    return 0;
}
