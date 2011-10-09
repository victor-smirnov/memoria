if (BUILD_UNIX)
    add_subdirectory(posix)
endif()

if (MINGW)
    add_subdirectory(mingw)
endif()

if (BUILD_MSVC)
    add_subdirectory(msvc)
endif()