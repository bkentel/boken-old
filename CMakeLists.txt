cmake_minimum_required(VERSION 3.2)
project(boken CXX)

include_directories(src)
include_directories(external)
include_directories(external/boost/predef/include)
include_directories(external/pcg/include)

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

add_executable(boken ${SOURCES} ${SOURCES_TEST})

set_property(TARGET boken PROPERTY CXX_STANDARD 14)

include(CheckIncludeFileCXX)

set(CMAKE_REQUIRED_FLAGS "-std=c++14")
CHECK_INCLUDE_FILE_CXX("experimental/string_view" HAVE_STD_EXP_STRING_VIEW)

if (HAVE_STD_EXP_STRING_VIEW)
    target_compile_definitions(boken PRIVATE BK_USE_STD_EXP_STRING_VIEW=1)
else()
    target_compile_definitions(boken PRIVATE BK_USE_BOOST_STRING_VIEW=1)
endif()
