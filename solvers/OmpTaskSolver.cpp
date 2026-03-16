//
// Created by Matouš Kovář on 05.03.2026.
//

#include "OmpTaskSolver.h"
#include <omp.h>
#include <chrono>


void OmpTaskSolver::solveDFSSeq(Board &board, int start_idx, int piece_id) {
    // base conditions
    if (best_cost == board.getTrivialUpperBound()) return;
    if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;



    // Nalezení dalšího volného políčka k rozhodnutí
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord a navrat
    if (cell == -1)
    {
        // poprve kontrola bez zamku aby se vlakna nezdrzovala, podruhe kontrola i prepis v kriticke sekci
        if (board.getCurrentCost() > best_cost)
        {
            #pragma omp critical // <-----------------------------ZDE ZMENA
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
        solveDFSSeq(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);

        if (best_cost == board.getTrivialUpperBound()) return;

        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFSSeq(board, cell + 1, piece_id);
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
                solveDFSSeq(board, cell + 1, piece_id);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound()) return;

        board.markAsEmpty(cell);
        solveDFSSeq(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);
    }
}


/**
 * pri spousteni tasku si posilaji promenne best_cost a best_board jako sdilene, protoze task si dela kopie standardne
 * @param board predavano hodnotou!!! ne reference
 * @param start_idx startovaci index prohledavani
 * @param piece_id startovaci id prvniho mozneho polozeneho dilku v behu
 * @param depth uroven zanoreni
 */
void OmpTaskSolver::solveDFS(Board board, int start_idx, int piece_id, int depth)
{
    // base conditions
    if (best_cost == board.getTrivialUpperBound()) return;
    if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;

    // Nalezení dalšího volného políčka k rozhodnutí
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - kontrola a vraceni v rekurzi
    if (cell == -1)
    {
        if (board.getCurrentCost() > best_cost)
        {
            #pragma omp critical // < ------------------------------ KRITICKA SEKCE - novinka oproti sekvencnimu
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

    int cell_val = board.getCellValue(cell);

    if (cell_val > 0) // policko obsahuje kladnou hodnotu
    {
        board.markAsEmpty(cell);

        if (depth < z) // prepinani mezi sekvencnim a vicevlaknovym vetvenim, z by mela byt mala konstanta
        {
            #pragma omp task shared(best_cost, best_board) // predavame globalni promenne do tasku
            solveDFS(board, cell + 1, piece_id, depth + 1);
        }
        else
            solveDFSSeq(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);

        if (best_cost == board.getTrivialUpperBound())
        {
            #pragma omp taskwait // Kdyz koncime predcasne, musime pockat nez se ukonci vsechny tasky v tomto vlaknu - sdileny zasobnik
            return;
        }


        for (int i = 0; i < 12; ++i) // pokladani dilku
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);

                if (depth < z)
                {
                    #pragma omp task shared(best_cost, best_board)
                    solveDFS(board, cell + 1, piece_id, depth + 1); // Vytvoříme úkol
                }
                else
                    solveDFSSeq(board, cell + 1, piece_id); // Jdeme do sekvence

                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }
    }
    else
    {
        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);

                if (depth < z)
                {
                    #pragma omp task shared(best_cost, best_board)
                    solveDFS(board, cell + 1, piece_id, depth + 1);
                }
                else
                    solveDFSSeq(board, cell + 1, piece_id);

                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound())
        {
            #pragma omp taskwait
            return;
        }

        board.markAsEmpty(cell);
        if (depth < z)
        {
            #pragma omp task shared(best_cost, best_board)
            solveDFS(board, cell + 1, piece_id, depth + 1);
        }
        else
            solveDFSSeq(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);
    }

    // master musi pockat na dokonceni ostatnich tasku, ktere vytvoril
    #pragma omp taskwait
}


double OmpTaskSolver::solve(const Board& initial_board) {
    best_cost = -1;
    best_board = initial_board;
    calls_counter = 0;
    int depth = 0;

    auto start_time = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    {
        #pragma omp single
        {
            solveDFS(initial_board, 0, 1, depth);
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;
    return elapsed.count();
}
