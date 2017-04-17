
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
    
    auto vv = app.run([](){
        std::cout << "Hello from Intellectio!" << std::endl;
        
        try {
            int64_t t0 = m::getTimeInMillis();
            
            for (size_t c = 0; c < 1000000; c++) 
            {
                engine().run_at(1, [&]{
                    counter++;
                });
            }
            
            int64_t t1 = m::getTimeInMillis();

            std::cout << "Time: " << m::FormatTime(t1 - t0) << ", counter = " << counter << std::endl;
            
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        dr::app().shutdown();
        return 5678;
    });

    std::cout << "vv = " << vv << std::endl;
    
    return 0;
}
