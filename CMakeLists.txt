cmake_minimum_required(VERSION 3.16)
project(FreeKassaTest LANGUAGES CXX)

#C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#BOOST
set(Boost_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS TRUE)

set(BUILD_SHARED_LIBS OFF)

find_package(Boost 1.75 REQUIRED COMPONENTS system thread json)
find_package(OpenSSL REQUIRED)

#executable
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/*.hpp
    src/*.h
)
add_executable(FreeKassaTest 
    ${SRC_FILES}
)

#Includes
target_include_directories(FreeKassaTest PRIVATE
    ${Boost_INCLUDE_DIRS}
)

#Linking
target_link_libraries(FreeKassaTest PRIVATE
    Boost::system
    Boost::thread
    Boost::json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Создаем целевую директорию, если ее нет
add_custom_command(
    TARGET FreeKassaTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/status
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/status
        ${CMAKE_CURRENT_BINARY_DIR}/status
    COMMENT "Copying status directory"
)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
