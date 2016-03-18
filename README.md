## Overview

Memoria is a C++14 framework providing general purpose dynamic data structures on top of key-value, object, file or block storage. Out of the box the following data structures are provided: map, vector, multimap, table, wide table and some others. Besides basic data structures there are also advanced ones: searchable sequences and wavelet trees. Many others can be designed using building blocks provided by the framework.

**The project is not yet ready so nothing is guaranteed to work**

## Download and Build 

Primary development platform is Linux with GCC 4.9+ or Clang 3.5+. Memoria uses CMake 2.8.5+ as a build system and provides some build scripts to simplify the build process. Just run the following commands in Linux shell:

```
#!console
$ git clone https://vsmirnov@bitbucket.org/vsmirnov/memoria.git
$ ./memoria/mkbuild/setup.sh
$ cd memoria-build/unix
$ ./mkbuild.sh
$ ./build.sh
```

When the build stops try:

```
#!console
$ cd bin
$ ./tests
```

To get available test options and configuration parameters:

```
#!console
$ ./tests --help
```

Usually tests take several minutes to complete.

## IDE Integration

See [instructions](https://bitbucket.org/vsmirnov/memoria/wiki/Eclipse_IDE_instructions) to setup the project in Eclipse IDE.