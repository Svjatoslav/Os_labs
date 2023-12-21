cmake_minimum_required (VERSION 3.22)

project ("lab3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Werror)

find_library(LIBRT rt)
find_package(Threads)

add_library(opt_node opt_node.h)
set_target_properties(opt_node PROPERTIES LINKER_LANGUAGE CXX)
add_library(opt_set opt_set.h)
set_target_properties(opt_set PROPERTIES LINKER_LANGUAGE CXX)
add_library(test test.h)
set_target_properties(test PROPERTIES LINKER_LANGUAGE CXX)

add_executable (lab3 ${CMAKE_SOURCE_DIR}/main.cpp)
target_link_libraries(lab3 PUBLIC pthread rt opt_node opt_set test)
