cmake_minimum_required(VERSION 3.7)
project(netkit-ntalk)

set(BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(SBIN_DIR "${CMAKE_INSTALL_PREFIX}/sbin")
set(MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

find_library(NCURSES ncurses REQUIRED)

option(USE_LIBWRAP
       "talkd: filter clients with libwrap (hosts.allow/hosts.deny)" ON)

add_subdirectory(talk)
add_subdirectory(talkd)
