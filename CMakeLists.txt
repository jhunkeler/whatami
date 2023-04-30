cmake_minimum_required(VERSION 3.2)
project(whatami C)

set(CMAKE_C_STANDARD 99)

add_executable(whatami main.c util.c darwin.c linux.c common.h x86.c x86.h win.c win.h)
