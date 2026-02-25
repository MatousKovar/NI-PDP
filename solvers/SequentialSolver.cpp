//
// Created by Matouš Kovář on 22.02.2026.
//

#include "SequentialSolver.h"

void SequentialSolver::solveDFS(Board &board, int start_idx, int piece_id) {
    if (best_cost == board.getTrivialUpperBound()) {
        return;
    }

    // Ořezávání neperspektivních větví - pokud bych zakryl vsechny zaporne, tak stejne nedostanu lepsi reseni nez jsem uz nasel
    if (board.getTheoreticalMaxPossibleCost() <= best_cost) {
        return;
    }

    // Nalezení dalšího volného políčka k rozhodnutí - je volne a nema byt preskoceno
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord
    if (cell == -1) {
        if (board.getCurrentCost() > best_cost) {
            best_cost = board.getCurrentCost();
            best_board = board; // Uložíme kopii nejlepšího stavu
        }
        return;
    }

    // Branching
    int cell_val = board.getCellValue(cell);

    // Hodnota na volnem policku je kladna - chceme idealne nezakryt -> ten stav navstivime prvni
    if (cell_val > 0) {
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);

        // Pokud jsme vynecháním už dosáhli maxima, nemusíme zkoušet pokládat dílky
        if (best_cost == board.getTrivialUpperBound()) return;

        // Následně zkusíme všechny varianty dilku
        for (int i = 0; i < 12; ++i) {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id + 1);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }
    } else {
        // v pripade ze je policko se zapornou hodnotou, tak je nejlepsi moznost ze bude zakryto -> to zkousime prvni
        for (int i = 0; i < 12; ++i) {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id + 1);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound()) return;

        // Až jako poslední zoufalou možnost zkusíme záporné políčko úmyslně vynechat
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);
    }
}

double SequentialSolver::solve(Board initial_board) {
    best_cost = -1;
    best_board = initial_board;

    auto start_time = std::chrono::high_resolution_clock::now();
    solveDFS(initial_board, 0, 1);
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;
    return elapsed.count();
}
