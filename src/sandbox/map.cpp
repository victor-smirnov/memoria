

#include <memoria/memoria.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

typedef SmallInMemAllocator                                                     Allocator;
typedef SCtrTF<Map1>::Type                                                      MapCtr;
typedef SCtrTF<Vector<Int>>::Type                                               VectorCtr;
typedef SCtrTF<VectorMap<BigInt, Byte>>::Type                                   VectorMapCtr;



int main(void) {

    MEMORIA_INIT(SmallProfile<>);

    Allocator allocator;

    MapCtr map(&allocator, 1, true);

    for (int c = 0; c < 10; c++)
    {
        map[c] = c + 1;
    }

    for (const auto& iter: map)
    {
        cout<<"Map: "<<iter.key()<<" "<<iter.value()<<endl;
    }

    cout<<endl;

    VectorCtr mvector(&allocator, 2, true);

    for (Int c = 0x100; c < 0x120; c++)
    {
        mvector<<c;
    }

    for (auto& value: mvector)
    {
        cout<<"Vector "<<value<<endl;
    }
    cout<<endl;

    Int cnt = 12345;
    for (auto& value: mvector)
    {
        value = cnt++;
    }

    for (auto& value: mvector)
    {
        cout<<"Vector "<<value<<endl;
    }
    cout<<endl;

    for (int c = 0; c < mvector.size(); c++)
    {
        mvector[c] = c;
    }

    for (int c = 0; c < mvector.size(); c++)
    {
        cout<<"Vector "<<mvector[c]<<endl;
    }
    cout<<endl;

    std::vector<Int> vec = mvector[7].subVector(10);

    for (auto& value: vec)
    {
        cout<<"Std::Vector "<<value<<endl;
    }

    return 0;
}

