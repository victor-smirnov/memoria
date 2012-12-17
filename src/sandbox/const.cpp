

#include <memoria/memoria.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

class AClass {
    Int* array_;
public:
    AClass() {
        array_ = new Int[10];
    }

    AClass(Int* array): array_(array) {}

    void constMethod() const {
        array_[0] = 10;
    }

    void nonConstMethod() {
        array_[1] = 20;
    }

    ~AClass() {
        delete[] array_;
    }

};


void doSomethin(const AClass& aClass)
{
    aClass.constMethod();
}

int main(void) {


    return 0;
}

