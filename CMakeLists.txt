cmake_minimum_required(VERSION 4.1)
project(NI_PDP)

set(CMAKE_CXX_STANDARD 14)

# 1. HACK: Nápověda pro Apple Silicon (M1/M2/M3), kde Homebrew instaluje knihovny
if (APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
endif()

# 2. HACK: Řekneme CMake, ať hledá OpenMP POUZE pro C++ (COMPONENTS CXX)
find_package(OpenMP COMPONENTS CXX REQUIRED)

# Náš spustitelný soubor
add_executable(NI_PDP main.cpp
        tests/TestPiecesLogic.cpp
        common/Pieces.h
        common/Board.cpp
        common/Board.h
        common/Pieces.cpp
        solvers/SequentialSolver.cpp
        solvers/SequentialSolver.h
        solvers/OmpSolver.cpp
        solvers/OmpSolver.h
)

# Připojení nalezené knihovny k projektu
if(OpenMP_CXX_FOUND)
    target_link_libraries(NI_PDP PUBLIC OpenMP::OpenMP_CXX)
endif()