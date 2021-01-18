## Overview

Memoria is a structured storage engines construction framework usilizing advanced metaprogramming techniques over modern C++. It consistes from the following subsystems:

1. SQL/XSD-compatible and extensible memory-mapped core data types like INT or VARCHAR(N), including composite types like arrays, sets, maps and documents.
2. LinkedData -- memory-mapped JSON-like documents and graphs over core data types with string-based external representation (String Data Notation, SDN).
3. Parametrised Data Containers -- B-Tree based complex data structures on top of core datatypes and LinkedData built with metaprogramming from reusable building blocks.
	* Basic parametrised data containers: Map<>, Set<>, Vector<>, Tensor<>
	* Complex data containers: Multimap<>, Multiset<>, Table<>
	* Advanced data structures: LOUDS tree, Wavelet Tree, etc ...
	* Various combinations of the above ...
4. Optional Copy-on-Write based concurrency control for multithreaded operation over Data Containers.
5. Optional snapshot-based [data versioning](https://en.wikipedia.org/wiki/Persistent_data_structure) for Data Containers.
6. Pluggable storage engines, including:
	* In-memory persistent store (the fastest and most flexible option).
	* Immutable memory-mapped files for snapshots. Convenient option to store large amounts of versioned read-only structured data with zero serialization.
	* Fast and simple Single Writer Multiple Reader (SWMR) on-disk Store for hybrid transactional/analytical data processing.
	* Other options are possible ...
7. Asynchronous IO stack with integrated Boost Fibers engine for high performance disk and network access. 
9. Hardware Acceleration Engine (this subsystem is WIP and hasn't been published yet):
	* RISC-V ISA extensions for common Memoria-specific data transformation like symbol sequences with large alphabets, direct support for variable length integers and other data types, etc.
	* Memoria-specific hardware memory protection (tagged memory) to ensure data integrity under various adversarial scenarios. 
	* [RISC-V CGRA](https://wavecomp.ai/wp-content/uploads/2018/12/WP_CGRA.pdf) for high performance massive data processing with Memoria.
	* [RocketChip](https://bar.eecs.berkeley.edu/projects/rocket_chip.html)-based RISC-V prototyping and development environment (Arty A7-100T, Alveo U50).


**Note (1)** that this project is currently in the transitional state. It hit the limits of [C++ template metaprogramming](https://bitbucket.org/vsmirnov/memoria/wiki/WhyC++), so 
new project [Jenny](https://github.com/victor-smirnov/jenny) was started to overcome the limitations of C++ TMP. Memoria is going to become one of the Jenny's core metaprogramming 
engine. In the upcoming time ongoing development will be focused of this transition, so no practice-specific functionality is actually planned. Only PoCs and DEMOs. 

**Note (2).** There are currently two main Memoria repositories, on the [Github](https://github.com/victor-smirnov/memoria) and on the [Bitbucket](https://bitbucket.org/vsmirnov/memoria/wiki/Home). Both are mirrors of each other, except that the repository on Bitbucket has additional documentation on the Wiki. Please use BB for additional details about Memoria.

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

See also [QtCreator IDE Instructions](https://bitbucket.org/vsmirnov/memoria/wiki/QtCreator%20IDE%20Instructions) for Linux and MacOS X.
