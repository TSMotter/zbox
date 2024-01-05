cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zbox)

set(SRC_DIR "src")
#set(INCLUDE_DIRS
#    "${SRC_DIR}/lib"
#    "${SRC_DIR}/external"
#)

# nanopb stuff
#list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
#include(nanopb)

# List 'regular' source files
set(SRC_FILES
    "${SRC_DIR}/main.cpp"
)

#target_include_directories(app PRIVATE ${INCLUDE_DIRS})
#zephyr_nanopb_sources(app proto/protocol.proto)
target_sources(app PRIVATE ${SRC_FILES})