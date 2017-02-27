cmake_minimum_required(VERSION 3.6)
project(lisod)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_SRC http_server.c responses.h request_handler.h request_handler.c)

add_executable(lisod ${SERVER_SRC})