//
// Created by Matouš Kovář on 05.03.2026.
//

#ifndef NI_PDP_OMPTASKSOLVER_H
#define NI_PDP_OMPTASKSOLVER_H
#include "../common/Board.h";
#include "../common/Pieces.h";

class OmpTaskSolver {
private:
    int best_cost;
    Board best_board;


    // Samotná rekurzivní BB-DFS funkce
    void solveDFS(Board& board, int start_idx, int piece_id);

public:
    int calls_counter;
    int getBestCost() const { return best_cost; }
    Board getBestBoard() const { return best_board; }
    OmpTaskSolver() : best_cost(-1), calls_counter(0) {}

    // Hlavní metoda, kterou zavoláte z mainu
    double solve(Board initial_board);
};


#endif //NI_PDP_OMPTASKSOLVER_H