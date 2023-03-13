#include <unistd.h>


int main(int argc, char** argv) {

    sleep(5);

    if (argc == 1) {
        int *addr = nullptr;
        *addr = 1234;
    }

    return 99;
}
