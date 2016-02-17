cmake_minimum_required(VERSION 3.2)
project(boken CXX)

include_directories(src)

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
