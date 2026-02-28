//
// Created by Matouš Kovář on 25.02.2026.
//

#include "OmpSolver.h"

std::vector<SearchState> OmpSolver::generateStartingBoards(const Board& originalBoard) {
    std::vector<SearchState> queue;
    int limit = n_threads * z;

    queue.push_back({originalBoard, 0, 1});

    // fronta je realizována jako vektor
    // head je aktuální vrchol fronty
    // ve fronte za indexem head uz jsou tedy jen listy
    int head = 0;

    while ((queue.size() - head) < limit && head < queue.size())
    {
        SearchState current_state = queue[head];
        head++;

        Board currentBoard = current_state.board;
        int start_idx = current_state.start_idx;
        int piece_id = current_state.start_piece_id;

        int cell = currentBoard.getNextFreeCell(start_idx);

        // deska je uz plna
        if (cell == -1) {
            queue.push_back(current_state);
            continue;
        }

        // prazdna deska do fronty
        currentBoard.markAsEmpty(cell);
        queue.push_back({currentBoard, cell + 1, piece_id});
        currentBoard.unmarkAsEmpty(cell);

        // dilky do fronty
        for (const auto& item : Pieces::VARIANTS) {
            if (currentBoard.canPlacePiece(cell, item)) {
                currentBoard.placePiece(cell, item, piece_id);
                queue.push_back({currentBoard, cell + 1, piece_id + 1});
                currentBoard.removePiece(cell, item);
            }
        }
    }

    std::vector<SearchState> startingBoards(queue.begin() + head, queue.end());

    return startingBoards;
}