cmake_minimum_required(VERSION 3.12)
project(FTPServer C)

set(CMAKE_C_STANDARD 11)

add_executable(FTPServer main.c global.h network.c network.h utils.c utils.h)