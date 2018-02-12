
#include <memoria/v1/core/tools/strings/format.hpp>

#include <tuple>
#include <iostream>
#include <functional>

namespace mm = memoria::v1;

int main() 
{
    std::cout << mm::fmt::format(u"AA {dddfffffff} BB {sss}", 12345) << std::endl;

    return 0;
}
