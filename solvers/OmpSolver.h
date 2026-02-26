//
// Created by Matouš Kovář on 25.02.2026.
//

#ifndef NI_PDP_OMPSOLVER_H
#define NI_PDP_OMPSOLVER_H
#include <omp.h>
#include "../common/Board.h"
#include "../solvers/SequentialSolver.h"

struct SearchState {
    Board board;
    int start_idx;
    int start_piece_id;
};

class OmpSolver {
    public:
    OmpSolver(const Board & originalBoard, int n_threads) : n_threads(0), global_best(0), originalBoard(originalBoard) {} ;

    double solve(Board initialBoard);
private:
    int global_best;
    int n_threads;
    Board originalBoard;

    std::vector<SearchState> generateStartingBoards(int z = 40);
};

#endif //NI_PDP_OMPSOLVER_H