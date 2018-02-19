
#include <memoria/v1/core/tools/terminal.hpp>

#include <unistd.h>

namespace memoria {
namespace v1 {
namespace tools {

bool IsATTY(int fd) {
    return isatty(fd);
}

}}}
