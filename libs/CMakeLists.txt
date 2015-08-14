cmake_minimum_required(VERSION 2.8.11)

project(libs)

set(SRC
    sys.cpp
    sys.h
)
add_library(libs ${SRC})