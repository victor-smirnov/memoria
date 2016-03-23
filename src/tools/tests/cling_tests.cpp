
int main(int argc, const char** argv, const char** envp);

void cling_tests() {

    const char* argv[] = {"tests"};

    main(1, argv, nullptr);
}}