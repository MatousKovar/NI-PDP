//
// Created by Matouš Kovář on 22.02.2026.
//

#ifndef NI_PDP_SEQUENTIALSOLVER_H
#define NI_PDP_SEQUENTIALSOLVER_H


#include "../common/Board.h"
#include "../common/Pieces.h"
#include <iostream>
#include <chrono>

class SequentialSolver {
private:
    int best_cost;
    Board best_board;


    // Samotná rekurzivní BB-DFS funkce
    void solveDFS(Board& board, int start_idx, int piece_id);

public:
    int calls_counter;
    int getBestCost() const { return best_cost; }
    Board getBestBoard() const { return best_board; }
    SequentialSolver() : best_cost(-1) {}

    // Hlavní metoda, kterou zavoláte z mainu
    double solve(Board initial_board);

    friend class OmpSolver;
};
#endif //NI_PDP_SEQUENTIALSOLVER_H