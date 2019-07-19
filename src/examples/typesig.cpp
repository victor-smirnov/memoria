#include <memoria/v1/api/datatypes/type_signature.hpp>

using namespace memoria::v1;

int main() {

    //std::string text = "Real type <a b c d, b(1, 2, bar)>(1, 2, foo, '4', 5.1234e6)   ";
    std::string text = "_Real _1type";

    TypeSignature::parse(text);

    return 0;
}
