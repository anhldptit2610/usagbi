cmake_minimum_required(VERSION 3.10)

project(usagbi)

set(CMAKE_CXX_STANDARD 20)

find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_subdirectory(core)
add_subdirectory(utils)
add_subdirectory(usagbi)
add_subdirectory(tests)


