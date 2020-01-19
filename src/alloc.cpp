
#include <stdlib.h>

#include <iostream>
#include <thread>
#include <vector>

#include <memoria/core/memory/jemalloc.hpp>
#include <memoria/core/memory/malloc.hpp>

int main(int argc, char **argv)
{
    auto ptr = memoria::allocate_system_zeroed<void>(100);

    //void* ptr = mma1_malloc
    std::cout << ptr.get() << std::endl;

    memoria::free_system(ptr.release());

    return 0;
}
