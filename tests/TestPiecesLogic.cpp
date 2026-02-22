//
// Created by Matouš Kovář on 21.02.2026.
//
#include <iostream>
#include <cassert>
#include "../common/Board.h"
#include "../common/Pieces.h"

class TestPiecesLogic {
public:
    static void runAllTests() {
        std::cout << "--- Spoustim sanity testy ---\n";
        test_loadFromFile();
        test_placement_and_removal();
        test_evaluation_logic();
        std::cout << "--- Vsechny testy OK ---\n\n";
    }

private:
    static void test_loadFromFile() {
        Board board;
        board.loadFromFile("../data/mapb6_6c.txt");

        assert(board.getWidth() == 6);
        assert(board.getHeight() == 6);
        assert(board.getTrivialUpperBound() == 280);

        std::cout << "  [OK] test_loadFromFile proslo.\n";
    }

    static void test_placement_and_removal() {
        Board board;
        board.loadFromFile("../data/mapb6_6c.txt");

        assert(board.canPlacePiece(0, Pieces::VARIANTS[0]) == true);
        board.placePiece(0, Pieces::VARIANTS[0], 1);

        assert(board.getTheoreticalMaxPossibleCost() == 240);
        assert(board.canPlacePiece(0, Pieces::VARIANTS[0]) == false);
        assert(board.canPlacePiece(1, Pieces::VARIANTS[0]) == false);
        assert(board.canPlacePiece(2, Pieces::VARIANTS[0]) == false);


        assert(board.canPlacePiece(3, Pieces::VARIANTS[2]) == true);
        assert(board.canPlacePiece(2, Pieces::VARIANTS[4]) == false);
        assert(board.canPlacePiece(3, Pieces::VARIANTS[7]) == false);


        board.removePiece(0, Pieces::VARIANTS[0]);

        assert(board.canPlacePiece(2, Pieces::VARIANTS[0]) == true);
        assert(board.getTheoreticalMaxPossibleCost() == 280);
        std::cout << "  [OK] test_placement_and_removal proslo.\n";
    }
    static void test_evaluation_logic() {
        Board board;
        board.loadFromFile("../data/mapb6_6c.txt");

        assert(board.getCurrentCost() == 0);

        board.markAsEmpty(0);
        assert(board.getCurrentCost() == 10);

        board.placePiece(0, Pieces::VARIANTS[0], 1);

        board.printSolution();


        std::cout << "  [OK] test_evaluation_logic proslo.\n";
    }
};
