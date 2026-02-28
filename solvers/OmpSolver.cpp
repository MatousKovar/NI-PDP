//
// Created by Matouš Kovář on 25.02.2026.
//

#include "OmpSolver.h"

double OmpSolver::solve(const Board &initialBoard) {
    std::vector<SearchState> queue = generateStartingBoards(initialBoard);
}

//modifikovana verze z sekvencniho
void OmpSolver::solveDFS(Board &board, int start_idx, int piece_id) {
#pragma omp atomic
    calls_counter++;

    //nejlepsi reseni je absolutne nejlepsi - trivialni mez
    if (best_cost == board.getTrivialUpperBound())
        return;

    // Ořezávání neperspektivních větví - pokud bych zakryl vsechny zaporne, tak stejne nedostanu lepsi reseni nez jsem uz nasel
    if (board.getTheoreticalMaxPossibleCost() <= best_cost)
    {
        return;
    }

    // Nalezení dalšího volného políčka k rozhodnutí - je volne a nema byt preskoceno
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord
    if (cell == -1)
    {
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
            return;
        }
    }

    int cell_val = board.getCellValue(cell);

    // Hodnota na volnem policku je kladna - chceme idealne nezakryt -> ten stav navstivime prvni
    if (cell_val > 0)
    {
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);

        // Pokud jsme vynecháním už dosáhli maxima, nemusíme zkoušet pokládat dílky
        if (best_cost == board.getTrivialUpperBound()) return;

        // Následně zkusíme všechny varianty dilku
        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id);
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
                solveDFS(board, cell + 1, piece_id);
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



std::vector<SearchState> OmpSolver::generateStartingBoards(const Board &originalBoard) const {
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
        if (cell == -1)
        {
            queue.push_back(current_state);
            continue;
        }

        // prazdna deska do fronty
        currentBoard.markAsEmpty(cell);
        queue.push_back({currentBoard, cell + 1, piece_id});
        currentBoard.unmarkAsEmpty(cell);

        // dilky do fronty
        for (const auto &item: Pieces::VARIANTS)
        {
            if (currentBoard.canPlacePiece(cell, item))
            {
                currentBoard.placePiece(cell, item, piece_id);
                queue.push_back({currentBoard, cell + 1, piece_id + 1});
                currentBoard.removePiece(cell, item);
            }
        }
    }

    std::vector<SearchState> startingBoards(queue.begin() + head, queue.end());

    return startingBoards;
}
