
#include <memoria/v1/core/types.hpp>

#include <memoria/v1/fiber/all.hpp>
#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/filesystem/path.hpp>
#include <memoria/v1/filesystem/operations.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>


//#include <boost/smart_ptr.hpp>
#include <memoria/v1/reactor/smart_ptr.hpp>




#include <iostream>
#include <thread>
#include <vector>

namespace rr = memoria::v1::reactor;



struct SomeClass: rr::enable_shared_from_this<SomeClass> {
    int value_{};


    SomeClass(int v): value_(v) {
        std::cout << "Constructing SomeClass@" << this << " at " << rr::engine().cpu() << ", value = " << value_ << std::endl;
    }

//    SomeClass() {
//        std::cout << "Constructing SomeClass@" << this << " at " << rr::engine().cpu() << std::endl;
//    }

    ~SomeClass() {
        std::cout << "Destructing SomeClass@" << this << " at " << rr::engine().cpu() << std::endl;
    }

    auto get_ref()  {
        return shared_from_this();
    }
};



int main(int argc, char **argv) 
{
    rr::Application app(argc, argv);

    app.run([]{
        std::cout << "Hello from SharedPtrs!" << std::endl;
        
        try {
            auto ptr = rr::allocate_shared<SomeClass>(1, std::allocator<SomeClass>(), 555);

            std::cout << ptr->value_ << std::endl;
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

        rr::app().shutdown();
    });

    return 0;
}
