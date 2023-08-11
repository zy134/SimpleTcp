cmake_minimum_required(VERSION 2.8)

project(SimpleTcp CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)

if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    message(STATUS "building for: debugging")
    # Set c++ version.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -pthread -fno-rtti")
    # Enable stack trace and not strip simbols.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -fno-omit-frame-pointer")
    # Add conversion check.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wshadow -Wold-style-cast \
        -Wwrite-strings -Wpointer-arith -Werror")
    # Add address-santizer for debug-build.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined")
    # Add stack protector for debug-build
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector")
    # For g++, add rdynamic to enable stack trace.
    if (CMAKE_COMPILER_IS_GNUCXX)
        message(STATUS "Current compiler is g++")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic")
    endif()
    # For clang, enable thread-safety check.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(STATUS "Current compiler is clang++, enable thread-safety check.")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wthread-safety -D_LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS=true")
    endif()

    # Set definition.
    add_compile_definitions(
        DEFAULT_LOG_PATH="${PROJECT_SOURCE_DIR}/logs"
        DEBUG_BUILD=1
        DEFAULT_LOG_LEVEL=1
    )
else()
    message(STATUS "building for: release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -pthread -fno-rtti")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -funroll-loops -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wshadow -Wold-style-cast \
        -Wwrite-strings -Wpointer-arith -Werror")

    add_compile_definitions(
        DEFAULT_LOG_PATH="${PROJECT_SOURCE_DIR}/logs"
        DEBUG_BUILD=0
        DEFAULT_LOG_LEVEL=2
    )
endif()

# build static library.
add_subdirectory(./src/base base)
add_subdirectory(./src/net net)
add_subdirectory(./src/tcp tcp)
add_subdirectory(./src/http http)

# build examples.
add_subdirectory(./example example)

# build test.
add_subdirectory(./test test)
