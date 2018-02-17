
#include <stdlib.h>

#include <iostream>
#include <thread>
#include <vector>

#include <memoria/v1/core/memory/jemalloc.hpp>

#include <malloc.h>

int main(int argc, char **argv)
{
    void* ptr = mma1_malloc(100);

    std::cout << ptr << std::endl;

    return 0;
}
