cmake_minimum_required(VERSION 3.2)
project(boken CXX)

include_directories(src)
include_directories(SYSTEM external)
include_directories(SYSTEM external/boost/assert/include)
include_directories(SYSTEM external/boost/config/include)
include_directories(SYSTEM external/boost/integer/include)
include_directories(SYSTEM external/boost/mpl/include)
include_directories(SYSTEM external/boost/predef/include)
include_directories(SYSTEM external/boost/random/include)
include_directories(SYSTEM external/boost/static_assert/include)
include_directories(SYSTEM external/boost/type_traits/include)
include_directories(SYSTEM external/boost/utility/include)
include_directories(SYSTEM external/pcg/include)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        "-Weverything"
        "-Wno-c++98-compat"
        "-Wno-c++98-c++11-compat-pedantic")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        "-Wall"
        "-Wextra"
        "-Wodr"
        "-Wredundant-decls"
        "-Wcast-align"
        "-Wmissing-declarations"
        "-Wmissing-include-dirs"
        "-Wswitch-enum"
        "-Wswitch-default"
        "-Winvalid-pch"
        "-Wredundant-decls"
        "-Wformat=2"
        "-Wmissing-format-attribute"
        "-Wformat-nonliteral")
endif()

set(SOURCES
    src/bsp_generator.cpp
    src/catch.cpp
    src/data.cpp
    src/level.cpp
    src/main.cpp
    src/random.cpp
    src/system_sdl.cpp
    src/text.cpp)

set(SOURCES_TEST
    src/test/bsp_generator.t.cpp
    src/test/math.t.cpp
    src/test/spatial_map.t.cpp)

set_source_files_properties(${SOURCES_TEST} PROPERTIES COMPILE_FLAGS "-Wno-exit-time-destructors")

add_executable(boken ${SOURCES} ${SOURCES_TEST})
set_property(TARGET boken PROPERTY CXX_STANDARD 14)

target_link_libraries(boken SDL2)

# Include file configuration checks
include(CheckIncludeFileCXX)

# Check for std::(exprimental)::string_view
set(CMAKE_REQUIRED_FLAGS "-std=c++14")
CHECK_INCLUDE_FILE_CXX("experimental/string_view" HAVE_STD_EXP_STRING_VIEW)

if(HAVE_STD_EXP_STRING_VIEW)
    target_compile_definitions(boken PRIVATE BK_USE_STD_EXP_STRING_VIEW=1)
else()
    target_compile_definitions(boken PRIVATE BK_USE_BOOST_STRING_VIEW=1)
endif()
