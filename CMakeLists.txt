cmake_minimum_required(VERSION 3.2)
project(boken CXX)

#
# Whether to warn about paddings added to structures
#
set(NO_WARN_PADDING 1)

#
# Wmissing-braces is buggy (issued for valid code).
# Wcovered-switch-default conflicts with GCC; warnings are still issues for unhandled enumerator values.
#
set(CLANG_WARNINGS
    "-Weverything"
    "-Wno-c++98-compat"
    "-Wno-c++98-c++11-compat-pedantic"
    "-Wno-covered-switch-default"
    "-Wno-missing-braces"
    "-Wno-missing-prototypes"
    "-Wno-date-time")

set(GCC_WARNINGS
    "-Wall"
    "-Wextra"
    "-Wcast-align"
    "-Wcast-qual"
    "-Wconversion"
#    "-Wduplicated-cond"
    "-Wfloat-equal"
    "-Wformat-nonliteral"
    "-Wformat=2"
    "-Winvalid-pch"
    "-Wmissing-declarations"
    "-Wmissing-format-attribute"
    "-Wmissing-include-dirs"
    "-Wodr"
    "-Wpointer-arith"
    "-Wredundant-decls"
    "-Wsuggest-final-methods"
    "-Wsuggest-final-types"
    "-Wsuggest-override"
    "-Wswitch-default"
    "-Wswitch-enum"
    "-Wunsafe-loop-optimizations"
    "-Wuseless-cast"
    "-Wzero-as-null-pointer-constant")

set(SOURCES
    src/bsp_generator.cpp
    src/catch.cpp
    src/command.cpp
    src/data.cpp
    src/entity.cpp
    src/inventory.cpp
    src/item.cpp
    src/level.cpp
    src/main.cpp
    src/random.cpp
    src/render.cpp
    src/serialize.cpp
    src/system_sdl.cpp
    src/text.cpp
    src/tile.cpp
    src/unicode.cpp
    src/utility.cpp
    src/world.cpp)

set(SOURCES_EXTERNAL
    external/bkassert/assert.cpp)

set(SOURCES_TEST
    src/test/bsp_generator.t.cpp
    src/test/circular_buffer.t.cpp
    src/test/entity.t.cpp
    src/test/hash.t.cpp
    src/test/level.t.cpp
    src/test/math.t.cpp
    src/test/math_types.t.cpp
    src/test/random.t.cpp
    src/test/rect.t.cpp
    src/test/serialize.t.cpp
    src/test/spatial_map.t.cpp
    src/test/types.t.cpp
    src/test/unicode.t.cpp
    src/test/utility.t.cpp)

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

#
# The unit test library Catch generated exit time destructors; disable warnings about this for
# unit tests only
#
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_source_files_properties(${SOURCES_TEST} PROPERTIES COMPILE_FLAGS "-Wno-exit-time-destructors")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set_source_files_properties(${SOURCES_TEST} PROPERTIES COMPILE_FLAGS "-Wno-parentheses")
endif()

add_executable(boken ${SOURCES} ${SOURCES_EXTERNAL} ${SOURCES_TEST})
set_property(TARGET boken PROPERTY CXX_STANDARD 14)

target_link_libraries(boken SDL2)

target_compile_options(boken PUBLIC $<$<CXX_COMPILER_ID:Clang>:${CLANG_WARNINGS}>)
target_compile_options(boken PUBLIC $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>)

if (${NO_WARN_PADDING})
    target_compile_options(boken PUBLIC $<$<CXX_COMPILER_ID:Clang>:-Wno-padded>)
endif()

# Include file configuration checks
include(CheckIncludeFileCXX)

# Check for std::(exprimental)::string_view
set(CMAKE_REQUIRED_FLAGS "-std=c++14")
CHECK_INCLUDE_FILE_CXX("experimental/string_view" HAVE_STD_EXP_STRING_VIEW)

if (HAVE_STD_EXP_STRING_VIEW)
    target_compile_definitions(boken PRIVATE BK_USE_STD_EXP_STRING_VIEW=1)
else()
    target_compile_definitions(boken PRIVATE BK_USE_BOOST_STRING_VIEW=1)
endif()
