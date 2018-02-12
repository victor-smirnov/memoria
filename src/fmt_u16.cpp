
#include <memoria/v1/core/tools/strings/format.hpp>

//#include <memoria/v1/fmt/ostream.hpp>
#include <memoria/v1/fmt/format.hpp>

#include <tuple>
#include <iostream>
#include <functional>

namespace mm = memoria::v1;

namespace _ {




}


int main() 
{
    std::cout << mm::fmt::format(u"AA {dddfffffff} BB {sss}", 12345) << std::endl;

    return 0;
}
