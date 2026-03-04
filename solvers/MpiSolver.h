//
// Created by Matouš Kovář on 04.03.2026.
//

#ifndef NI_PDP_MPISOLVER_H
#define NI_PDP_MPISOLVER_H
#include <mpi.h>
#include "../common/Board.h"
#include "../common/Pieces.h"

struct SearchState {
    Board board;
    int start_idx;
    int start_piece_id;
};

class MpiSolver {
public:
    int global_best;
    int n_threads;
    int z;
    int calls_counter = 0;
    volatile int best_cost;
    Board best_board;


private:
    void solveDFS(Board &board, int start_idx, int piece_id, long long &local_calls, int global_best);
    std::vector<SearchState> generateStartingBoards(const Board& original_board) const;
};


#endif //NI_PDP_MPISOLVER_H