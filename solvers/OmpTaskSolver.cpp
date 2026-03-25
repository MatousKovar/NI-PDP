//
// Created by Matouš Kovář on 05.03.2026.
//

#include "OmpTaskSolver.h"
#include <omp.h>
#include <chrono>

/**
 * Klasicke sekvencni, jedine co, tak kdyz pristupuju k atributum oznacenym jako SHARED, tak musim pres kritisckou sekci
 * @param board - predava se referenci protoze tady uz se bezi sekvencne, nemusi se resit preisovani
 */
void OmpTaskSolver::solveDFSSeq(Board &board, int start_idx, int piece_id) {
    if (best_cost == board.getTrivialUpperBound()) return;
    if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;

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
                solveDFSSeq(board, cell + 1, piece_id + 1);
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
                solveDFSSeq(board, cell + 1, piece_id + 1);
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
 * pri spousteni tasku si posilaji promenne best_cost a best_board jako sdilene, protoze task si dela neco jako kopii kontextu standardne
 * @param board predavano hodnotou, musi byt kopie pro kazdy task
 * @param depth maximalni povolena uroven zanoreni, nez prechazim k sekvencnimu reseni, idealne mensi cislo - 2, nebo 3, tak aby vetsina vypoctu byla v sekvencnim reseni
 */
void OmpTaskSolver::solveDFS(Board board, int start_idx, int piece_id, int depth)
{
    if (best_cost == board.getTrivialUpperBound()) return;
    if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;

    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - kontrola a vraceni v rekurzi
    if (cell == -1)
    {
        if (board.getCurrentCost() > best_cost)
        {
            #pragma omp critical // < ------------------------------ KRITICKA SEKCE - novinka oproti sekvencnimu, pristup ke sdilene promenne
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

    //VETVENI PODLE HODNOTY POLICKA
    if (cell_val > 0)
    {
        board.markAsEmpty(cell);

        if (depth < z) // prepinani mezi sekvencnim a vicevlaknovym vetvenim, z by mela byt mala konstanta - napr 2, at se vytvori dost tasku, ktere budou vypocetne narocne, ne naopak
        {
            #pragma omp task shared(best_cost, best_board) // predavame globalni promenne do tasku
            solveDFS(board, cell + 1, piece_id, depth + 1);
        }
        else
            solveDFSSeq(board, cell + 1, piece_id);
        board.unmarkAsEmpty(cell);

        if (best_cost == board.getTrivialUpperBound()) return;


        for (int i = 0; i < 12; ++i) // pokladani dilku
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                if (depth < z)
                {
                    #pragma omp task shared(best_cost, best_board)
                    solveDFS(board, cell + 1, piece_id + 1, depth + 1); // Vytvoříme úkol
                }
                else
                    solveDFSSeq(board, cell + 1, piece_id + 1); // Jdeme do sekvence

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
                    solveDFS(board, cell + 1, piece_id + 1, depth + 1);
                }
                else
                    solveDFSSeq(board, cell + 1, piece_id + 1);

                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound())
            return;

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

    // ceka se na dokonceni vsech mnou vytvorenych tasku
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
