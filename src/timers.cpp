
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
        std::cout << "Hello from Timers!" << std::endl;
        
        try {
            using Time = std::chrono::high_resolution_clock;

            auto t0 = Time::now();
            engine().sleep_for(std::chrono::seconds(1));
            auto t1 = Time::now();

            auto fs = t1 - t0;

            std::cout << "Speep time: " << fs.count() << std::endl;
			
            int cnt_ = 0;

            dr::Timer tt = dr::Timer::schedule(std::chrono::seconds(1), 5, [&]{
				std::cout << "In the periodic timer: " << (++cnt_) << std::endl;
            });

            engine().sleep_for(std::chrono::seconds(8));
            std::cout << "Done!" << std::endl;
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        dr::app().shutdown();
    });

    return 0;
}
