//
// Created by Matouš Kovář on 04.03.2026.
//

#ifndef NI_PDP_MPISOLVER_H
#define NI_PDP_MPISOLVER_H
#include <mpi.h>
#include "../common/Board.h"
#include "../common/Pieces.h"
#include "OmpSolver.h"


class MpiSolver {
public:
    int global_best;
    int n_threads;
    int z;
    int calls_counter = 0;
    int world_rank;
    int world_size;
    volatile int best_cost;
    Board best_board;

    MpiSolver(int n_threads, int z, int world_rank, int world_size) : n_threads(n_threads), z(z), world_rank(world_rank), world_size(world_size) {};
    double solve(const Board& initialBoard);

private:
    void solveDFS(Board &board, int start_idx, int piece_id, long long &local_calls, int global_best);
    std::vector<SearchState> generateStartingBoards(const Board& original_board) const;
};


#endif //NI_PDP_MPISOLVER_H