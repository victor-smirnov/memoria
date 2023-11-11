# Overview
Memoria is an advanced data engineering Framework written in modern C++. It consists from the following parts:

*Memoria is still in development and is not meant to be used in practice. Functionality declared below may be incomplete, experimental or currently missing. All processes are optimized for development, not for production.*

1. **Hermes** - arbitrarily structured object graphs allocated in relocatable contiguous memory segments, suitable for memory mapping and inter-process communication, with focus on data storage purposes. Hermes objects and containers are string-externalizable and may look like "Json with types". GRPC-style services and related tooling (IDL) is provided.
2. Extensible and customizable set of **Data Containers**, internally based on B+Trees crafted from reusable building blocks by the metaprogramming framework. The spectrum of possible containers is from plain dynamic arrays and sets, via row-wise/column-wise tables, multitude of graphs, to compressed spatial indexes and beyond. Everything that maps well to B+Trees can be a first-class citizen of data containers framework. Containers and Hermes data objects are deeply integrated with each other.
3. Pluggable **Storage Engines** based on Copy-on-Write principles. Storage is completely separated from containers via simple but efficient contracts. Out of the box, OLTP-optimized and HTAP-optimized storage, as well as In-Memory storage options, are provided, supporting advanced features like serializable transactions and Git-like branching.
4. **DSL execution engine**. Lightweight embeddable VM with Hermes-backed code model (classes, byte-code, resources) natively supporting Hermes data types. Direct Interpreter and AOT compilation to optimized C++.
5. **Runtime Environments**. Single thread per CPU core, non-migrating fibers, high-performance IO on top of io-uring and hardware accelerators.
6. **Development Automation** tools. Clang-based build tools to extract metadata directly from C++ sources and generate boilerplate code.

The purpose of the project is to integrate all the aspects and components described above into a single vertical framework, starting from *bare silicon* up to networking and human-level interfaces. The framework may eventually grow up into a fully-featured metaprogramming platform.

Memoria does recognize and welcome generative AI, but without hype and fanaticism. The project will be actively exploring AI technics for code and data structure generation, as well as *turning itself into a dataset* for generative AI training. So future users may not even know that they are using algorithms, data structures programming patterns, idioms and philosophy originated in the Memoria Framework.

# OS & Platforms

Only modern *Linux* is currently supported. Some modules like Hermes and VM will be fully supported on other platforms, but certain storage options may not be avaialble or have reduced functionality (due to the limited OS support). 

# Build and Run

Memoria relies on third-party libraries that either may not be available on supported developenment platfroms or have outdated versions there. Vcpkg package manager is currently being used for dependency management. Memoria itself is avaialble via [custom Vcpkg registry](https://github.com/victor-smirnov/memoria-vcpkg-registry). Conan recipies and source packages for Linux distributions (via CPack) may be provided in the future.

See the [Dockerfile](https://github.com/victor-smirnov/memoria/blob/master/docker/Dockerfile) on how to configure development environment on Ubuntu 22.04. Standard development environment will be the latest Ubuntu LTS. 

Standard compiler is the *latest stable Clang*. Certain versions of GCC may crash while building Memoria or fail in another way (GCC 13.1 is known to work). Standard library is platform-specific: libstdc++ on Linux. 

# IDE Instructions

See [QT Creator](https://memoria-framework.dev/docs/overview/qt_creator_instructions/) configuration instructions.
