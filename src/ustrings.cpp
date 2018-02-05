
#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/tools/regexp/icu_regexp.hpp>

#include <iostream>

using namespace memoria::v1;

int main(int argc, char** argv) {

    U8String str8(u8"klmn -- Всем привет --");

    U16String str16(u"abcd");

    U32String str32(u"xyz_");

    UWString strw(L"[@Всем Привет@]");

    std::cout << str8 << std::endl;
    std::cout << str32 + str8.to_u32() << std::endl;
    std::cout << strw << std::endl;

    std::wcout << strw << std::endl;

    return 0;
}
