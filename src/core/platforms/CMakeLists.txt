if (UNIX)
    add_subdirectory(posix)
endif()

if (MINGW)
    add_subdirectory(mingw)
endif()

if (MSVC OR INTEL_COMPILER)
    add_subdirectory(msvc)
endif()