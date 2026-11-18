cmake_minimum_required(VERSION 3.16)
project(recparse C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler and linker flags are supplied by the validator through
# -DCMAKE_C_FLAGS / -DCMAKE_EXE_LINKER_FLAGS so that the release and the
# sanitized build are driven entirely from configuration (R-SCOPE-08).
add_executable(recparse
    src/main.c
    src/record.c
)

target_include_directories(recparse PRIVATE src)
