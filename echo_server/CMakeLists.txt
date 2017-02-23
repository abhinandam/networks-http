cmake_minimum_required(VERSION 3.6)
project(networks_http)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_SRC echo_server.c)
set(CLIENT_SRC echo_client.c)

add_executable(echo_server ${SERVER_SRC})
add_executable(echo_client ${CLIENT_SRC})