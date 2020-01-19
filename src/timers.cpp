
#include "memoria/fiber/all.hpp"
#include "memoria/reactor/application.hpp"
#include "memoria/filesystem/path.hpp"
#include "memoria/filesystem/operations.hpp"
#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/time.hpp>

#include <iostream>
#include <thread>
#include <vector>

namespace m  = memoria::v1;

namespace df  = memoria::fibers;
namespace dr  = memoria::reactor;
namespace mt  = memoria::tools;
namespace fs  = memoria::filesystem;

using namespace dr;


volatile size_t counter{};


int main(int argc, char **argv) 
{    
    Application app(argc, argv);
    
    app.start_engines();

    int cnt_ = 0;

    app.run([&](){
        try {
            using Time = std::chrono::high_resolution_clock;

            auto t0 = Time::now();
            engine().sleep_for(std::chrono::seconds(1));
            auto t1 = Time::now();

            auto fs = t1 - t0;

            std::cout << "Sleep time: " << fs.count() << std::endl;
			


            dr::Timer tt = dr::Timer::schedule(std::chrono::seconds(4), std::chrono::seconds(1), 5, [&]{
                std::cout << "In the periodic timer: " << (++cnt_) << std::endl;
            });

            engine().sleep_for(std::chrono::seconds(7));
            std::cout << "Done!" << std::endl;
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        dr::app().shutdown();
    });

    return 0;
}
