mkdir memoria-msvc2019
cd memoria-msvc2019
                                  
cmake -G "Visual Studio 16 2019" -A x64 -T "ClangCl"^
	-DCMAKE_TOOLCHAIN_FILE:INTERNAL=c:\projects\vcpkg-memoria\scripts\buildsystems\vcpkg.cmake^
	-DCMAKE_BUILD_TYPE=Debug^
	-DBUILD_SANDBOX=OFF^
	-DBUILD_TESTS=ON^
	-DBUILD_CONTAINERS=ON^
	-DBUILD_MEMORY_STORE=OFF^
	-DBUILD_TESTS_PACKED=OFF^
	-DBUILD_EXAMPLES=ON^
	-DICU_DEBUG=OFF^
	../memoria

