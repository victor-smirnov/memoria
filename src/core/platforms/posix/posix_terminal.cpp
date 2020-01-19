
#include <memoria/core/tools/terminal.hpp>

#include <unistd.h>

namespace memoria {
namespace tools {

bool IsATTY(int fd) {
    return isatty(fd);
}

}}
