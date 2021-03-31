cmake_minimum_required(VERSION 3.10)

# set the project name
project(2048 VERSION 0.0.1 LANGUAGES C CXX)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

set(CMAKE_CXX_STANDARD 11)

# add the executable
add_executable(2048 2048.cpp)
add_library(2048utils lib2048utils.hpp)
set_target_properties(2048utils PROPERTIES LINKER_LANGUAGE CXX)
add_library(2048core lib2048core.cpp lib2048core.hpp)
add_compile_options(-fsanitize=undefined,address -g)

target_link_libraries(2048 2048utils 2048core sfml-audio sfml-graphics sfml-window sfml-system)