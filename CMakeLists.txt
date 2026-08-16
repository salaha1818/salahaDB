cmake_minimum_required(VERSION 3.16)

project(SalahaDB VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SalahaDB
    src/main.cpp
    src/Parser.cpp
    src/Database.cpp
    src/StorageEngine.cpp
    src/Serializer.cpp
    src/Page.cpp
    src/PageManager.cpp
    src/BPlusTree.cpp
)

target_include_directories(SalahaDB PRIVATE include)