
#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/application.hpp"
#include "memoria/v1/filesystem/path.hpp"
#include "memoria/v1/filesystem/operations.hpp"
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <memoria/v1/core/memory/malloc.hpp>

#include <iostream>
#include <thread>
#include <vector>

#include <fcntl.h>

using namespace memoria::v1::reactor;


int main(int argc, char** argv) {
    return Application::run(argc, argv, [&]{
        ShutdownOnScopeExit hh;

        engine().coutln(u"Hello, {} World!!!", u8"cruel");

        return 0;
    });
}


