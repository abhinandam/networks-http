cmake_minimum_required(VERSION 3.6)
project(networks_http)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_SRC http_server.c responses.h)

add_executable(http_server ${SERVER_SRC})