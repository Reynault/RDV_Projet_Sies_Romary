cmake_minimum_required(VERSION 3.10)
project(RDV_Projet_Sies_Romary)

set(CMAKE_C_STANDARD 11)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB SOURCES
        "${SRC_DIR}/*.h"
        "${SRC_DIR}/*.cpp"
        )

add_executable(RDV_Projet_Sies_Romary
        src/rasterizing.cpp)
