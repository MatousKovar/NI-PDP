//
// Created by Matouš Kovář on 22.02.2026.
//

#include "SequentialSolver.h"

void SequentialSolver::solveDFS(Board &board, int start_idx, int piece_id) {
    // 1. Předčasné ukončení (našli jsme absolutní utopii)
    if (best_cost == board.getTrivialUpperBound()) {
        return;
    }

    // 2. Ořezávání neperspektivních větví (Branch and Bound)
    if (board.getTheoreticalMaxPossibleCost() <= best_cost) {
        return;
    }

    // 3. Nalezení dalšího volného políčka k rozhodnutí
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord
    if (cell == -1) {
        if (board.getCurrentCost() > best_cost) {
            best_cost = board.getCurrentCost();
            best_board = board; // Uložíme kopii nejlepšího stavu
        }
        return;
    }

    // --- 4. VĚTVENÍ S HEURISTIKOU ---
    int cell_val = board.getCellValue(cell);

    if (cell_val > 0) {
        // HEURISTIKA A: Kladné políčko chceme primárně vynechat (zvedá to skóre)
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);

        // Pokud jsme vynecháním už dosáhli maxima, nemusíme zkoušet pokládat dílky
        if (best_cost == board.getTrivialUpperBound()) return;

        // Následně zkusíme všechny varianty quatromin
        for (int i = 0; i < 12; ++i) {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id + 1);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }
    } else {
        // HEURISTIKA B: Záporné políčko chceme primárně zakrýt (aby nestrhlo skóre)
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

void SequentialSolver::solve(Board initial_board) {
    std::cout << "--- Spoustim SequentialSolver (BB-DFS) ---\n";

    best_cost = -1;
    best_board = initial_board; // Pojistka

    // Měření času přesně podle doporučení
    auto start_time = std::chrono::high_resolution_clock::now();

    // Spuštění rekurze (hledáme od indexu 0, první dílek má ID 1)
    solveDFS(initial_board, 0, 1);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "Vypocet dokoncen!\n";
    std::cout << "Cas behu sekvencniho algoritmu: " << elapsed.count() << " s\n\n";

    std::cout << "Nejlepsi nalezene reseni:\n";
    if (best_cost != -1) {
        best_board.printSolution();
    } else {
        std::cout << "Zadne platne reseni nebylo nalezeno.\n";
    }
}
