
#include <stdlib.h>

#include <iostream>
#include <thread>
#include <vector>

#include <memoria/v1/core/memory/jemalloc.hpp>
#include <memoria/v1/core/memory/malloc.hpp>

int main(int argc, char **argv)
{
    auto ptr = memoria::v1::allocate_system_zeroed<void>(100);

    //void* ptr = mma1_malloc
    std::cout << ptr.get() << std::endl;

    memoria::v1::free_system(ptr.release());

    return 0;
}
