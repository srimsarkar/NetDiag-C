cmake_minimum_required(VERSION 3.10)
project(NetDiag-C C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L")

# Include header directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files (excluding main.c for library/tests)
set(NETDIAG_CORE_SRCS
    src/utils.c
    src/logger.c
    src/network.c
    src/transceiver.c
    src/device.c
    src/diagnostics.c
    src/monitoring.c
)

# Core Static Library
add_library(netdiag_core STATIC ${NETDIAG_CORE_SRCS})

# Main Executable
add_executable(netdiag src/main.c)
target_link_libraries(netdiag PRIVATE netdiag_core)

# Diagnostic Test Executable
add_executable(test_diagnostics tests/test_diagnostics.c)
target_link_libraries(test_diagnostics PRIVATE netdiag_core)

enable_testing()
add_test(NAME DiagnosticUnitTest COMMAND test_diagnostics)
