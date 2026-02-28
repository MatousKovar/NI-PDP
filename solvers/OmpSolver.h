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
    int global_best;
    int n_threads;
    int z;
    int calls_counter = 0;
    int best_cost;
    Board best_board;

    OmpSolver(int n_threads, int z = 40) : n_threads(0), global_best(0), z(z) {} ;

    double solve(const Board& initialBoard);
private:


    void solveDFS(Board& board, int start_idx, int piece_id);
    std::vector<SearchState> generateStartingBoards(const Board& original_board) const;
};

#endif //NI_PDP_OMPSOLVER_H