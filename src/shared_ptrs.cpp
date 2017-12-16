
#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/application.hpp"
#include "memoria/v1/filesystem/path.hpp"
#include "memoria/v1/filesystem/operations.hpp"
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/fmt/format.hpp>





#include <iostream>
#include <thread>
#include <vector>

namespace m  = memoria::v1;

namespace df  = memoria::v1::fibers;
namespace rr  = memoria::v1::reactor;
namespace mt  = memoria::v1::tools;
namespace fs  = memoria::v1::filesystem;

using namespace rr;

struct SomeClass: enable_local_shared_from_this<SomeClass> {
    int value_{};


    SomeClass(int v): value_(v) {
        std::cout << "Constructing SomeClass@" << this << " at " << engine().cpu() << ", value = " << value_ << std::endl;
    }

    SomeClass() {
        std::cout << "Constructing SomeClass@" << this << " at " << engine().cpu() << std::endl;
    }

    ~SomeClass() {
        std::cout << "Destructing SomeClass@" << this << " at " << engine().cpu() << std::endl;
    }

    auto get_ref()  {
        return shared_from_this();
        //return shared_ptr<SomeClass>(this);
    }
};


int main(int argc, char **argv) 
{    
    //const char* argv0[] = {"shared_ptrs", "--threads 2", nullptr};

    Application app(argc, argv);

    app.run([](){
        std::cout << "Hello from SharedPtrs!" << std::endl;
        
        try {
            auto ptr = rr::make_local_shared<SomeClass>(555);

            std::cout << ptr->value_ << std::endl;

            auto ptr2 = ptr->get_ref();

            local_weak_ptr<SomeClass> weakp = ptr;

            ptr.reset();

            std::cout << "After shared ptr reset" << std::endl;

            auto pppp = weakp.lock();
            weakp.reset();

            std::cout << "VV: " << pppp->value_ << std::endl;
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        rr::app().shutdown();
    });

    return 0;
}
