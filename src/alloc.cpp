
#include <stdlib.h>

#include <iostream>
#include <thread>
#include <vector>

#include <memoria/v1/jemalloc/jemalloc.hpp>

extern "C" {
    void* mma1_malloc(size_t size) noexcept;
}

int main(int argc, char **argv) 
{    
    void* ptr = mma1_malloc(100);

    std::cout << ptr << std::endl;

    return 0;
}
