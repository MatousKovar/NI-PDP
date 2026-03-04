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
    const int TAG_WORK = 1; // master posila slave pokyn at resi
    const int TAG_END = 2; // master posila pokyn k ukonceni
    const int TAG_RESULT = 3; // slave posila masterovi vysledky

    void solveDFS(Board &board, int start_idx, int piece_id, long long &local_calls, int global_best);
    std::vector<SearchState> generateStartingBoards(const Board& original_board) const;

    static void packState(const SearchState &state, int *buffer);

    static SearchState unpackState(const int *buffer, const Board &original_board);
};


#endif //NI_PDP_MPISOLVER_H