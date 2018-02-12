
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
namespace rr  = memoria::v1::reactor;
namespace mt  = memoria::v1::tools;
namespace fs  = memoria::v1::filesystem;

using namespace rr;

struct SomeClass: enable_shared_from_this<SomeClass> {
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
    }
};

template <typename T, typename... Args>
rr::weak_ptr<T> make_weak(int cpu, Args&&... args)
{
    return rr::weak_ptr<T>(rr::make_shared<T>(cpu, std::forward<Args>(args)...));
}

int main(int argc, char **argv) 
{    
    //const char* argv0[] = {"shared_ptrs", "--threads 2", nullptr};

    Application app(argc, argv);

    app.run([]{
        std::cout << "Hello from SharedPtrs!" << std::endl;
        
        try {
            rr::local_weak_ptr<SomeClass> ptr = make_weak<SomeClass>(1, 555);

            //rr::weak_ptr<SomeClass> pp = ptr;

            rr::local_weak_ptr<SomeClass> lpp = ptr;

            ptr.reset();

            std::cout << "After reset" << std::endl;

        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        rr::app().shutdown();
    });

    return 0;
}
