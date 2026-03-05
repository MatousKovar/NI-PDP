//
// Created by Matouš Kovář on 05.03.2026.
//

#ifndef NI_PDP_OMPTASKSOLVER_H
#define NI_PDP_OMPTASKSOLVER_H
#include "../common/Board.h"
#include "../common/Pieces.h"

class OmpTaskSolver {
private:

    int z;



    void solveDFSSeq(Board &board, int start_idx, int piece_id, long long &local_calls);

    // Samotná rekurzivní BB-DFS funkce
    void solveDFS(Board board, int start_idx, int piece_id, int depth);

public:
    Board best_board;
    int best_cost;
    int calls_counter;
    int getBestCost() const { return best_cost; }
    Board getBestBoard() const { return best_board; }
    OmpTaskSolver(int z) : best_cost(-1), calls_counter(0), z(z) {}

    // Hlavní metoda, kterou zavoláte z mainu
    double solve(Board initial_board);
};


#endif //NI_PDP_OMPTASKSOLVER_H