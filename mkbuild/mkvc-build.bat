mkdir memoria-msvc2017
cd memoria-msvc2017

cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SANDBOX=OFF -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF -DICU_DEBUG=OFF ../memoria

