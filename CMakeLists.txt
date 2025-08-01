cmake_minimum_required(VERSION 3.10.0)
project(TinyWebServer VERSION 0.1.0 LANGUAGES C)

# --- Global Settings ---
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Define the root directory for binaries
set(BINARY_ROOT_DIR ${CMAKE_SOURCE_DIR})

# --- Include Directories ---
include_directories(include)


# --- Main Web Server Target ---
set(SERVER_SRC_FILES
    src/server/tiny.c
    src/server/csapp.c
    src/server/util.c
)
add_executable(tiny ${SERVER_SRC_FILES})

# Set the output location for the 'webserv' executable to the 'bin/' directory
set_target_properties(tiny PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BINARY_ROOT_DIR}/bin"
)


# --- CGI Program Target ---
set(CGI_ADD_SRC_FILES
    src/cgi/add.c
)
add_executable(add ${CGI_ADD_SRC_FILES})

# Set the output location for the 'add' executable to the 'cgi-bin/' directory
set_target_properties(add PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${BINARY_ROOT_DIR}/cgi-bin"
)

# --- Informational Messages ---
message(STATUS "Main server executable will be placed in: ${BINARY_ROOT_DIR}/bin")
message(STATUS "CGI executable will be placed in: ${BINARY_ROOT_DIR}/cgi-bin")
