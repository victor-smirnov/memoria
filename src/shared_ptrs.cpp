
#include <memoria/v1/core/types.hpp>

#include <memoria/v1/fiber/all.hpp>
#include <memoria/v1/reactor/application.hpp>
#include <memoria/v1/filesystem/path.hpp>
#include <memoria/v1/filesystem/operations.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <memoria/v1/reactor/smart_ptr.hpp>

#include <iostream>
#include <thread>
#include <vector>

using namespace memoria::v1::reactor;



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

struct ArrayValClass {
    ArrayValClass() {
        std::cout << "ArrValCtr: " << engine().cpu() << std::endl;
    }

    ~ArrayValClass() {
        std::cout << "ArrValDtr: " << engine().cpu() << std::endl;
    }
};


int main(int argc, char **argv) 
{
    return Application::run(argc, argv, []{
        ShutdownOnScopeExit hh;

        int32_t target_cpu = engine().cpu_num() - 1;

        std::cout << "Hello from SharedPtrs!" << std::endl;
        
        ArrayValClass* av0 = new ArrayValClass();
        shared_ptr<ArrayValClass> pp0(target_cpu, av0, [](ArrayValClass* vv){
            delete vv;
        });

        ArrayValClass* av1 = new ArrayValClass();
        shared_ptr<ArrayValClass> pp1(target_cpu, av1, [](ArrayValClass* vv){
            delete vv;
        }, std::allocator<ArrayValClass>());

        shared_ptr<ArrayValClass> pp3(target_cpu, std::nullptr_t(), [](ArrayValClass* vv){
            delete vv;
        });

        allocate_shared_at<ArrayValClass[]>(target_cpu, std::allocator<ArrayValClass>(), 3);

        auto ptr = allocate_shared_at<SomeClass>(target_cpu, std::allocator<SomeClass>(), 555);

        std::cout << ptr->value_ << std::endl;

        return 0;
    });
}
