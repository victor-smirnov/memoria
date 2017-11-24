
#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/application.hpp"
#include "memoria/v1/filesystem/path.hpp"
#include "memoria/v1/filesystem/operations.hpp"
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <iostream>
#include <thread>
#include <vector>

namespace m  = memoria::v1;

namespace df  = memoria::v1::fibers;
namespace dr  = memoria::v1::reactor;
namespace mt  = memoria::v1::tools;
namespace fs  = memoria::v1::filesystem;

using namespace dr;

volatile size_t counter{};


int main(int argc, char **argv) 
{    
    Application app(argc, argv);
    
    app.run([](){
        std::cout << "Hello from Files!" << std::endl;
        
        try {
            auto ff = open_dma_file("sone_file.bin", FileFlags::CREATE | FileFlags::RDWR);
            auto dma_buf = allocate_dma_buffer(16384);
            ff.write(dma_buf.get(), 0, 16384);

            ff.close();
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        dr::app().shutdown();
    });

    return 0;
}
