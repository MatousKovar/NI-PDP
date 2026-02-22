cmake_minimum_required(VERSION 4.1)
project(NI_PDP)

set(CMAKE_CXX_STANDARD 14)

add_executable(NI_PDP main.cpp
        tests/TestPiecesLogic.cpp
        common/Pieces.h
        common/Board.cpp
        common/Board.h
        common/Pieces.cpp
        solvers/SequentialSolver.cpp
        solvers/SequentialSolver.h
)
