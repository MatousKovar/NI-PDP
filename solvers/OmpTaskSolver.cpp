//
// Created by Matouš Kovář on 05.03.2026.
//

#include "OmpTaskSolver.h"
#include <omp.h>


void OmpTaskSolver::solveDFSSeq(Board &board, int start_idx, int piece_id, long long &local_calls) {
    local_calls++;

    if (best_cost == board.getTrivialUpperBound()) return;

    if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;

    // Nalezení dalšího volného políčka k rozhodnutí
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord
    if (cell == -1)
    {
        // poprve kontrola bez zamku aby se vlakna nezdrzovala, podruhe kontrola i prepis v kriticke sekci
        if (board.getCurrentCost() > best_cost)
        {
            #pragma omp critical
            {
                if (board.getCurrentCost() > best_cost)
                {
                    best_cost = board.getCurrentCost();
                    best_board = board;
                }
            }
        }
        return;
    }

    // ------------ZBYTEK KODU KLASICKY SEKVENCNI-----------------------
    // Branching
    int cell_val = board.getCellValue(cell);

    // Hodnota na volnem policku je kladna
    if (cell_val > 0)
    {
        board.markAsEmpty(cell);
        solveDFSSeq(board, cell + 1, piece_id, local_calls);
        board.unmarkAsEmpty(cell);

        if (best_cost == board.getTrivialUpperBound()) return;

        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFSSeq(board, cell + 1, piece_id, local_calls);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }
    }
    else
    {
        // Zaporna policka
        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFSSeq(board, cell + 1, piece_id, local_calls);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound()) return;

        board.markAsEmpty(cell);
        solveDFSSeq(board, cell + 1, piece_id, local_calls);
        board.unmarkAsEmpty(cell);
    }
}

void OmpTaskSolver::solveDFS(Board board, int start_idx, int piece_id, int & depth) {
    calls_counter += 1;
    if (best_cost == board.getTrivialUpperBound())
    {
        return;
    }

    // Ořezávání neperspektivních větví - pokud bych zakryl vsechny zaporne, tak stejne nedostanu lepsi reseni nez jsem uz nasel
    if (board.getTheoreticalMaxPossibleCost() <= best_cost)
        return;

    // Nalezení dalšího volného políčka k rozhodnutí - je volne a nema byt preskoceno
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord
    if (cell == -1)
    {
        if (board.getCurrentCost() > best_cost)
        {
            best_cost = board.getCurrentCost();
            best_board = board; // Uložíme kopii nejlepšího stavu
        }
        return;
    }

    // Branching
    int cell_val = board.getCellValue(cell);

    // Hodnota na volnem policku je kladna - chceme idealne nezakryt -> ten stav navstivime prvni
    if (cell_val > 0)
    {
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id,depth);
        board.unmarkAsEmpty(cell);

        // Pokud jsme vynecháním už dosáhli maxima, nemusíme zkoušet pokládat dílky
        if (best_cost == board.getTrivialUpperBound()) return;

        // Následně zkusíme všechny varianty dilku
        #pragma omp task if (depth < z)
        depth++;
        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id,depth);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }
    }
    else
    {
        // v pripade ze je policko se zapornou hodnotou, tak je nejlepsi moznost ze bude zakryto -> to zkousime prvni
        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id,depth);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound()) return;

        // Až jako poslední zoufalou možnost zkusíme záporné políčko úmyslně vynechat
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id, depth);
        board.unmarkAsEmpty(cell);
    }
}


double OmpTaskSolver::solve(Board initial_board)
{
    #pragma omp parallel
    #pragma omp single
    best_cost = -1;
    best_board = initial_board;
    calls_counter = 0;
    int depth = 0;

    auto start_time = std::chrono::high_resolution_clock::now();
    solveDFS(initial_board, 0, 1, depth);
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;
    return elapsed.count();
}
