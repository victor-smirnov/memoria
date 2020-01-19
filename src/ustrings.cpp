
#include <memoria/core/strings/string.hpp>
#include <memoria/core/regexp/icu_regexp.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <iostream>

using namespace memoria::v1;

using MUUID = memoria::UUID;

int main(int argc, char** argv)
{
    try {
        U8String str8(u8"klmn -- Hello World --");

        U16String str16(u"abcd");

        U32String str32(u"xyz_");

        UWString strw(L"[@Hello World@]");

        std::cout << str8 << std::endl;
        std::cout << str32 + str8.to_u32() << std::endl;
        std::cout << strw << std::endl;

        std::wcout << strw << std::endl;

        std::cout << MUUID::make_random() << std::endl;
        std::cout << MUUID::make_random() << std::endl;
    }
    catch (MemoriaThrowable& e) {
        e.dump(std::cout);
    }

    return 0;
}
