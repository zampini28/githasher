cmake_minimum_required(VERSION 3.27)
project(GitHasher CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(LibArchive REQUIRED)

add_executable(GitHasher src/main.cpp)

target_link_libraries(GitHasher PRIVATE
    Drogon::Drogon
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    fmt::fmt
    LibArchive::LibArchive
)

add_custom_command(TARGET GitHasher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/static ${CMAKE_BINARY_DIR}/static
)
