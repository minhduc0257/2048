cmake_minimum_required(VERSION 3.10)

# set the project name
project(2048 VERSION 0.0.1 LANGUAGES C CXX)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

set(ASSETS
    Lato-Bold.ttf
    Montserrat-Regular.ttf
    RobotoMono-Regular.ttf
    pause-loop.wav
    sectionfail.wav
    soft-hitsoft.wav
    color.txt
    color2.txt)

set(CMAKE_CXX_STANDARD 17)

# add the executable
add_executable(2048 2048.cpp)
add_library(2048utils INTERFACE lib2048utils.hpp)
add_library(2048ui INTERFACE lib2048ui.hpp)
add_library(2048core lib2048core.cpp lib2048core.hpp)
target_link_libraries(2048core sfml-graphics)
add_compile_options(-fsanitize=undefined,address -g)

foreach(file_i ${ASSETS})
    add_custom_command(
        TARGET 2048 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/${file_i}
                $<TARGET_FILE_DIR:2048>/${file_i})
endforeach(file_i)

target_link_libraries(2048 2048utils 2048core sfml-audio sfml-graphics sfml-window sfml-system)