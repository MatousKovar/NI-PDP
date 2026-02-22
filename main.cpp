#include <iostream>
#include <exception>
#include "common/Board.h"
#include "tests/TestPiecesLogic.cpp"
#include "solvers/SequentialSolver.h"
#include "solvers/SequentialSolver.h"

int main() {
    std::cout << "=== NI-PDP: SQX (Skladani quatromin s maximem) ===\n";

    try {
        // 1. Otestujeme logiku (lze zakomentovat při měření velkých map)
        TestPiecesLogic tests;
        tests.runAllTests();

        // 2. Načtení mapy
        Board board;
        board.loadFromFile("../data/mapb6_6c.txt");

        std::cout << "Mapa uspesne nactena!\n";
        std::cout << "Rozmery: " << board.getWidth() << "x" << board.getHeight() << "\n";
        std::cout << "Trivialni horni mez: " << board.getTrivialUpperBound() << "\n\n";

        // 3. Spuštění sekvenčního solveru
        SequentialSolver seq_solver;
        seq_solver.solve(board);

    } catch (const std::exception& e) {
        std::cerr << "Kriticka chyba: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
