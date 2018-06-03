mkdir memoria-clang6-msvc2017
cd memoria-clang6-msvc2017


cmake -G "Visual Studio 15 2017 Win64" -T v141_clang60 -DBoost_COMPILER=-vc141 -DCMAKE_BUILD_TYPE=Debug -DBUILD_SANDBOX=OFF -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DICU_DEBUG=OFF ../memoria

