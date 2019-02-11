## Overview

Memoria is a C++14 framework providing general purpose dynamic data structures on top of in-memory, file or block device storage. Out of the box the following data structures are provided: map, vector, multimap, table, wide table and some others. Besides basic data structures there are also advanced ones: searchable sequences and wavelet trees. Many others can be designed using building blocks provided by the framework.

**The project is not yet ready so nothing is guaranteed to work**

## Download and Build 

Primary development platform is Linux with Clang 6.0+. Memoria uses CMake 3.6+ as a build system and provides some build scripts to simplify the build process. Memoria is using Vcpkg library manager, but does not require it if the environment already contains all required libraries. 

First, download and build Memoria-specific version of Vcpkg:

```
#!console
# Assuming current folder is /home/guest/cxx
$ git clone https://github.com/victor-smirnov/vcpkg-memoria.git
$ cd vcpkg-memoria
$ git checkout memoria-libs
$ ./bootstrap-vcpkg.sh
$ ./vcpkg install boost icu 
```

After libraries are built, download and build Memoria with tests:

```
#!console
# Assuming current folder is /home/guest/cxx
$ git clone https://vsmirnov@bitbucket.org/vsmirnov/memoria.git
$ ./memoria/mkbuild/setup-vcpkg.sh
$ cd memoria-build
$ make -j6
```

When the build is finished, try:

```
#!console
$ cd memoria-build/src/tests-core/tests
$ ./tests2
```

To get available test options and configuration parameters:

```
#!console
$ ./tests --help
```

Usually tests take several minutes to complete.

