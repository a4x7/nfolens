cmake_minimum_required(VERSION 4.2.0)
project(nfolens VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG main
)
FetchContent_MakeAvailable(ftxui)
add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE -static -static-libstdc++ -static-libgcc ftxui::ftxui)
